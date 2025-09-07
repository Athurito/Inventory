// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Components/Inv_InventoryComponent.h"

#include "Items/Components/Inv_ItemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Widgets/Inventory/InventoryBase/Inv_InventoryBase.h"
#include "Items/Inv_InventoryItem.h"
#include "Items/Fragments/Inv_ItemFragment.h"


UInv_InventoryComponent::UInv_InventoryComponent() : InventoryList(this)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
	bIsInventoryMenuOpen = false;
}

void UInv_InventoryComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, InventoryList);
}

void UInv_InventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	ConstructInventory();
}

void UInv_InventoryComponent::TryAddItem(UInv_ItemComponent* ItemComponent)
{
	FInv_SlotAvailabilityResult Result = InventoryMenu->HasRoomForItem(ItemComponent);

	UInv_InventoryItem* FoundItem = InventoryList.FindFirstItemByType(ItemComponent->GetItemManifest().GetItemType());

	Result.Item = FoundItem;
	
	if (Result.TotalRoomToFill == 0)
	{
		NoRoomInInventory.Broadcast();
		return;
	}

	if (Result.Item.IsValid() && Result.bStackable)
	{
		OnStackChange.Broadcast(Result);
		Server_AddStacksToItem(ItemComponent, Result.TotalRoomToFill, Result.Remainder);
	}
	else if (Result.TotalRoomToFill > 0)
	{
		Server_AddNewItem(ItemComponent, Result.bStackable ? Result.TotalRoomToFill : 0, Result.Remainder);
	}
}

void UInv_InventoryComponent::Server_AddNewItem_Implementation(UInv_ItemComponent* ItemComponent, int32 StackCount, int32 Remainder)
{
	UInv_InventoryItem* NewItem = InventoryList.AddEntry(ItemComponent);
	NewItem->SetTotalStackCount(StackCount);
	//If there is no other client
	if (GetOwner()->GetNetMode() == NM_ListenServer || GetOwner()->GetNetMode() == NM_Standalone)
	{
		OnItemAdded.Broadcast(NewItem);
	}
	
	
	if (Remainder == 0)
	{
		ItemComponent->PickedUp();
	}
	else if (FInv_StackableFragment* StackableFragment = ItemComponent->GetItemManifestMutable().GetFragmentOfTypeMutable<FInv_StackableFragment>())
	{
		StackableFragment->SetStackCount(Remainder);
	}
}

void UInv_InventoryComponent::Server_AddStacksToItem_Implementation(UInv_ItemComponent* ItemComponent, int32 StackCount, int32 Remainder)
{
	const FGameplayTag& ItemType = IsValid(ItemComponent) ? ItemComponent->GetItemManifestMutable().GetItemType() : FGameplayTag::EmptyTag;
	UInv_InventoryItem* Item = InventoryList.FindFirstItemByType(ItemType);

	if (!IsValid(Item)) return;

	Item->SetTotalStackCount(Item->GetTotalStackCount() + StackCount);

	if (Remainder == 0)
	{
		ItemComponent->PickedUp();
	}
	else if (FInv_StackableFragment* StackableFragment = ItemComponent->GetItemManifest().GetFragmentOfTypeMutable<FInv_StackableFragment>())
	{
		StackableFragment->SetStackCount(Remainder);
	}
}

void UInv_InventoryComponent::Server_DropItem_Implementation(UInv_InventoryItem* Item, int32 StackCount)
{
	const int32 NewStackCount = Item->GetTotalStackCount() - StackCount;
	if (NewStackCount <= 0)
	{
		InventoryList.RemoveEntry(Item);
	}
	else
	{
		Item->SetTotalStackCount(NewStackCount);
	}
	SpawnDroppedItem(Item, StackCount);
}

void UInv_InventoryComponent::SpawnDroppedItem(UInv_InventoryItem* Item, int32 StackCount)
{
	const APawn* OwningPawn = OwningController->GetPawn();
	FVector RotatedForward = OwningPawn->GetActorForwardVector();
	RotatedForward = RotatedForward.RotateAngleAxis(FMath::FRandRange(DropSpawnAngleMin, DropSpawnAngleMax), FVector::UpVector);

	FVector SpawnLocation = OwningPawn->GetActorLocation() + RotatedForward * FMath::FRandRange(DropSpawnDistanceMin, DropSpawnDistanceMax);

	SpawnLocation.Z -= RelativeSpawnElevation;
	const FRotator SpawnRotation = OwningPawn->GetActorRotation();

	FInv_ItemManifest& ItemManifest = Item->GetItemManifestMutable();
	if (FInv_StackableFragment* StackableFragment = ItemManifest.GetFragmentOfTypeMutable<FInv_StackableFragment>())
	{
		StackableFragment->SetStackCount(StackCount);
	}
	
	ItemManifest.SpawnPickupActor(this, SpawnLocation, SpawnRotation);
}


void UInv_InventoryComponent::Server_ConsumeItem_Implementation(UInv_InventoryItem* Item)
{
	const int32 NewStackCount = Item->GetTotalStackCount() - 1;

	if (NewStackCount <= 0)
	{
		InventoryList.RemoveEntry(Item);
	}
	else
	{
		Item->SetTotalStackCount(NewStackCount);
	}

	if (FInv_ConsumableFragment* ConsumableFragment = Item->GetItemManifestMutable().GetFragmentOfTypeMutable<FInv_ConsumableFragment>())
	{
		ConsumableFragment->OnConsume(OwningController.Get());
	}
}

void UInv_InventoryComponent::Server_TransferItemToInventory_Implementation(UInv_InventoryComponent* TargetInventory, UInv_InventoryItem* Item, int32 StackCount)
{
	if (!IsValid(TargetInventory) || !IsValid(Item)) return;
	if (TargetInventory == this) return;

	// Clamp amount
	const bool bIsStackable = Item->IsStackable();
	int32 TransferAmount = FMath::Max(1, StackCount);
	if (bIsStackable)
	{
		TransferAmount = FMath::Clamp(TransferAmount, 1, Item->GetTotalStackCount());
	}
	else
	{
		TransferAmount = 1; // non-stackable items move one at a time
	}

	// Try merge into an existing stack on target
	const FGameplayTag& ItemType = Item->GetItemManifest().GetItemType();
	UInv_InventoryItem* TargetExisting = TargetInventory->InventoryList.FindFirstItemByType(ItemType);
	bool bMerged = false;
	if (IsValid(TargetExisting) && bIsStackable)
	{
		TargetExisting->SetTotalStackCount(TargetExisting->GetTotalStackCount() + TransferAmount);
		bMerged = true;
	}

	if (!bMerged)
	{
		// Create a new item instance for the target owner with the transfer amount
		FInv_ItemManifest ManifestCopy = Item->GetItemManifest();
		if (FInv_StackableFragment* Stackable = ManifestCopy.GetFragmentOfTypeMutable<FInv_StackableFragment>())
		{
			Stackable->SetStackCount(TransferAmount);
		}
		UObject* TargetOuter = TargetInventory->GetOwner();
		UInv_InventoryItem* NewItem = ManifestCopy.Manifest(TargetOuter);
		TargetInventory->AddRepSubObject(NewItem);
		TargetInventory->InventoryList.AddEntry(NewItem);

		// If listen server or standalone, immediately update UI like other server add functions
		if (TargetInventory->GetOwner()->GetNetMode() == NM_ListenServer || TargetInventory->GetOwner()->GetNetMode() == NM_Standalone)
		{
			TargetInventory->OnItemAdded.Broadcast(NewItem);
		}
	}

	// Decrease/remove from source
	const int32 NewCount = Item->GetTotalStackCount() - TransferAmount;
	if (NewCount <= 0)
	{
		InventoryList.RemoveEntry(Item);
	}
	else
	{
		Item->SetTotalStackCount(NewCount);
	}
}

void UInv_InventoryComponent::Server_EquipSlotClicked_Implementation(UInv_InventoryItem* ItemToEquip,
	UInv_InventoryItem* ItemToUnequip)
{
	Multicast_EquipSlotClicked(ItemToEquip, ItemToUnequip);
}

void UInv_InventoryComponent::Multicast_EquipSlotClicked_Implementation(UInv_InventoryItem* ItemToEquip,
	UInv_InventoryItem* ItemToUnequip)
{
	// Equipment Component will listen to these delegates
	OnItemEquipped.Broadcast(ItemToEquip);
	OnItemUnequipped.Broadcast(ItemToUnequip);
}

void UInv_InventoryComponent::ToggleInventoryMenu()
{
	if (bIsInventoryMenuOpen)
	{
		CloseInventoryMenu();
	}
	else
	{
		OpenInventoryMenu();
	}
	OnInventoryMenuToggled.Broadcast(bIsInventoryMenuOpen);
}

void UInv_InventoryComponent::AddRepSubObject(UObject* SubObject)
{
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && IsValid(SubObject))
	{
		AddReplicatedSubObject(SubObject);
	}
}

void UInv_InventoryComponent::ConstructInventory()
{
	OwningController = Cast<APlayerController>(GetOwner());

	// Only construct the inventory menu if the owner is a local PlayerController
	if (!OwningController.IsValid() || !OwningController->IsLocalController())
	{
		return;
	}

	if (!IsValid(InventoryMenuClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryMenuClass is not set on %s. Skipping inventory menu construction."), *GetName());
		return;
	}

	InventoryMenu = CreateWidget<UInv_InventoryBase>(OwningController.Get(), InventoryMenuClass);
	if (!IsValid(InventoryMenu))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to create InventoryMenu widget for %s."), *GetName());
		return;
	}
	InventoryMenu->AddToViewport();
	CloseInventoryMenu();
}

void UInv_InventoryComponent::OpenInventoryMenu()
{
	if(!IsValid(InventoryMenu)) return;

	InventoryMenu->SetVisibility(ESlateVisibility::Visible);
	bIsInventoryMenuOpen = true;

	if (!OwningController.IsValid()) return;

	FInputModeGameAndUI InputMode;
	OwningController->SetInputMode(InputMode);
	OwningController->SetShowMouseCursor(true);
}

void UInv_InventoryComponent::CloseInventoryMenu()
{
	if(!IsValid(InventoryMenu)) return;

	
	InventoryMenu->SetVisibility(ESlateVisibility::Collapsed);
	bIsInventoryMenuOpen = false;

	if (!OwningController.IsValid()) return;

	FInputModeGameOnly InputMode;
	OwningController->SetInputMode(InputMode);
	OwningController->SetShowMouseCursor(false);
}


