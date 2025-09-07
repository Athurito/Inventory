// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Utils/Inv_InventoryStatics.h"

#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Widgets/Inventory/InventoryBase/Inv_InventoryBase.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Widgets/Inventory/Containers/Inv_ContainerWindow.h"
#include "Widgets/Inventory/Spatial/Inv_MinimalInventoryGrid.h"
#include "Widgets/Inventory/HoverItem/Inv_HoverItem.h"

UInv_InventoryComponent* UInv_InventoryStatics::GetInventoryComponent(const APlayerController* PlayerController)
{
	// Defensively resolve the InventoryComponent from the possessed pawn first (most setups attach it to the Character),
	// then fall back to the controller if needed. This avoids calling FindComponentByClass on an invalid actor.
	if (!IsValid(PlayerController))
	{
		return nullptr;
	}

	// Try the pawn
	if (const APawn* Pawn = PlayerController->GetPawn())
	{
		if (IsValid(Pawn))
		{
			if (UInv_InventoryComponent* InvOnPawn = Pawn->FindComponentByClass<UInv_InventoryComponent>())
			{
				return InvOnPawn;
			}
		}
	}

	// Fallback: try the controller itself
	return PlayerController->FindComponentByClass<UInv_InventoryComponent>();
}

EInv_ItemCategory UInv_InventoryStatics::GetItemCategoryFromItemComponent(const UInv_ItemComponent* ItemComponent)
{
	if (!IsValid(ItemComponent))
	{
		return EInv_ItemCategory::None;
	}

	return ItemComponent->GetItemManifest().GetItemCategory();
}

void UInv_InventoryStatics::ItemHovered(APlayerController* PlayerController, UInv_InventoryItem* Item)
{
	UInv_InventoryComponent* InventoryComponent = GetInventoryComponent(PlayerController);
	if (!IsValid(InventoryComponent)) return;

	UInv_InventoryBase* InventoryBase = InventoryComponent->GetInventoryMenu();
	if (!IsValid(InventoryBase)) return;

	if (InventoryBase->HasHoverItem()) return;

	InventoryBase->OnItemHovered(Item);
}

void UInv_InventoryStatics::ItemUnhovered(APlayerController* PlayerController)
{
	UInv_InventoryComponent* InventoryComponent = GetInventoryComponent(PlayerController);
	if (!IsValid(InventoryComponent)) return;

	UInv_InventoryBase* InventoryBase = InventoryComponent->GetInventoryMenu();
	if (!IsValid(InventoryBase)) return;

	InventoryBase->OnItemUnHovered();
}

UInv_HoverItem* UInv_InventoryStatics::GetHoverItem(APlayerController* PC)
{
	// First, try the player's current full inventory widget
	if (IsValid(PC))
	{
		if (UInv_InventoryComponent* IC = GetInventoryComponent(PC))
		{
			if (UInv_InventoryBase* InventoryBase = IC->GetInventoryMenu())
			{
				if (IsValid(InventoryBase))
				{
					if (UInv_HoverItem* Hover = InventoryBase->GetHoverItem())
					{
						if (IsValid(Hover)) return Hover;
					}
				}
			}
		}
	}

	// Fallback: If a container window is open, check its grids for a hover item
	if (IsValid(PC))
	{
		TArray<UUserWidget*> FoundWidgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(PC, FoundWidgets, UInv_ContainerWindow::StaticClass(), false);
		for (UUserWidget* Widget : FoundWidgets)
		{
			UInv_ContainerWindow* CW = Cast<UInv_ContainerWindow>(Widget);
			if (!IsValid(CW)) continue;
			if (UInv_MinimalInventoryGrid* PlayerGrid = CW->GetPlayerGrid())
			{
				if (PlayerGrid->HasHoverItem())
				{
					if (UInv_HoverItem* Hover = PlayerGrid->GetHoverItem())
					{
						if (IsValid(Hover)) return Hover;
					}
				}
			}
			if (UInv_MinimalInventoryGrid* ContainerGrid = CW->GetContainerGrid())
			{
				if (ContainerGrid->HasHoverItem())
				{
					if (UInv_HoverItem* Hover = ContainerGrid->GetHoverItem())
					{
						if (IsValid(Hover)) return Hover;
					}
				}
			}
		}
	}
	return nullptr;
}

UInv_InventoryBase* UInv_InventoryStatics::GetInventoryWidget(APlayerController* PC)
{
	// Original: try via the player's inventory component
	if (IsValid(PC))
	{
		if (UInv_InventoryComponent* IC = GetInventoryComponent(PC))
		{
			if (UInv_InventoryBase* InventoryBase = IC->GetInventoryMenu())
			{
				if (IsValid(InventoryBase)) return InventoryBase;
			}
		}
	}

	// Fallback: inventory widget might have been reparented into the container window; search in viewport
	if (IsValid(PC))
	{
		TArray<UUserWidget*> FoundInventoryWidgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(PC, FoundInventoryWidgets, UInv_InventoryBase::StaticClass(), false);
		for (UUserWidget* Widget : FoundInventoryWidgets)
		{
			UInv_InventoryBase* InvBase = Cast<UInv_InventoryBase>(Widget);
			if (!IsValid(InvBase)) continue;
			// Prefer widgets owned by this player controller
			if (InvBase->GetOwningPlayer() == PC)
			{
				return InvBase;
			}
		}
	}
	return nullptr;
}
