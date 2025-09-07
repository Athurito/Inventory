#include "Widgets/Inventory/Spatial/Inv_SimpleInventory.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"
#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "Widgets/Inventory/Spatial/Inv_MinimalInventoryGrid.h"
#include "Widgets/Inventory/GridSlots/Inv_EquippedGridSlot.h"
#include "Widgets/Inventory/HoverItem/Inv_HoverItem.h"
#include "Widgets/Inventory/SlottedItems/Inv_EquippedSlottedItem.h"
#include "Widgets/ItemDescription/Inv_ItemDescription.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Items/Inv_InventoryItem.h"
#include "Items/Fragments/Inv_ItemFragment.h"

void UInv_SimpleInventory::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsValid(Button_Equipables))
	{
		Button_Equipables->OnClicked.AddDynamic(this, &ThisClass::ShowEquipables);
	}
	if (IsValid(Button_Consumables))
	{
		Button_Consumables->OnClicked.AddDynamic(this, &ThisClass::ShowConsumables);
	}
	if (IsValid(Button_Craftables))
	{
		Button_Craftables->OnClicked.AddDynamic(this, &ThisClass::ShowCraftables);
	}

	if (IsValid(Grid_Equipables)) Grid_Equipables->SetOwningCanvas(CanvasPanel);
	if (IsValid(Grid_Consumables)) Grid_Consumables->SetOwningCanvas(CanvasPanel);
	if (IsValid(Grid_Craftables)) Grid_Craftables->SetOwningCanvas(CanvasPanel);

	ShowEquipables();

	// Gather equipped grid slots from the widget tree and bind click event
	if (IsValid(WidgetTree))
	{
		WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			if (UInv_EquippedGridSlot* EquippedGridSlot = Cast<UInv_EquippedGridSlot>(Widget))
			{
				EquippedGridSlots.Add(EquippedGridSlot);
				EquippedGridSlot->EquippedGridSlotClicked.AddDynamic(this, &ThisClass::EquippedGridSlotClicked);
			}
		});
	}
}

FReply UInv_SimpleInventory::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (ActiveGrid.IsValid())
	{
		ActiveGrid->DropItem();
	}
	return FReply::Handled();
}

void UInv_SimpleInventory::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// If we are holding an item (hover item), ensure descriptions are hidden and timers are cleared
	if (HasHoverItem())
	{
		if (IsValid(ItemDescription))
		{
			ItemDescription->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (IsValid(EquippedItemDescription))
		{
			EquippedItemDescription->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (IsValid(GetOwningPlayer()))
		{
			GetOwningPlayer()->GetWorld()->GetTimerManager().ClearTimer(DescriptionTimer);
			GetOwningPlayer()->GetWorldTimerManager().ClearTimer(EquippedDescriptionTimer);
		}
		return;
	}

	if (!IsValid(ItemDescription)) return;
	SetItemDescriptionSizeAndPosition(ItemDescription, CanvasPanel);
	SetEquippedItemDescriptionSizeAndPosition(ItemDescription, EquippedItemDescription, CanvasPanel);
}

FInv_SlotAvailabilityResult UInv_SimpleInventory::HasRoomForItem(UInv_ItemComponent* ItemComponent) const
{
	if (!IsValid(ItemComponent)) return FInv_SlotAvailabilityResult();
	const EInv_ItemCategory Category = UInv_InventoryStatics::GetItemCategoryFromItemComponent(ItemComponent);
	switch (Category)
	{
		case EInv_ItemCategory::Equipable:
			return Grid_Equipables ? Grid_Equipables->HasRoomForItem(ItemComponent) : FInv_SlotAvailabilityResult();
		case EInv_ItemCategory::Consumable:
			return Grid_Consumables ? Grid_Consumables->HasRoomForItem(ItemComponent) : FInv_SlotAvailabilityResult();
		case EInv_ItemCategory::Craftable:
			return Grid_Craftables ? Grid_Craftables->HasRoomForItem(ItemComponent) : FInv_SlotAvailabilityResult();
		default:
			return FInv_SlotAvailabilityResult();
	}
}

void UInv_SimpleInventory::OnItemHovered(UInv_InventoryItem* Item)
{
	// Do not show any descriptions while holding a hover item
	if (HasHoverItem())
	{
		// Ensure descriptions are collapsed and any timers cleared
		if (IsValid(ItemDescription)) ItemDescription->SetVisibility(ESlateVisibility::Collapsed);
		if (IsValid(EquippedItemDescription)) EquippedItemDescription->SetVisibility(ESlateVisibility::Collapsed);
		if (IsValid(GetOwningPlayer()))
		{
			GetOwningPlayer()->GetWorld()->GetTimerManager().ClearTimer(DescriptionTimer);
			GetOwningPlayer()->GetWorldTimerManager().ClearTimer(EquippedDescriptionTimer);
		}
		return;
	}

	const auto& Manifest = Item->GetItemManifest();
	UInv_ItemDescription* DescriptionWidget = GetItemDescription();
	DescriptionWidget->SetVisibility(ESlateVisibility::Collapsed);

	GetOwningPlayer()->GetWorld()->GetTimerManager().ClearTimer(DescriptionTimer);
	GetOwningPlayer()->GetWorldTimerManager().ClearTimer(EquippedDescriptionTimer);
	
	FTimerDelegate DescriptionTimerDelegate;
	DescriptionTimerDelegate.BindLambda([this, Item, &Manifest, DescriptionWidget]()
	{
		// If a hover item appeared during the delay, skip showing
		if (HasHoverItem())
		{
			return;
		}
		GetItemDescription()->SetVisibility(ESlateVisibility::HitTestInvisible);
		Manifest.AssimilateInventoryFragments(DescriptionWidget);

		// For the second item description, showing the equipped item of this type after a delay
		FTimerDelegate EquippedDescriptionTimerDelegate;
		EquippedDescriptionTimerDelegate.BindUObject(this, &ThisClass::ShowEquippedItemDescription, Item);
		GetOwningPlayer()->GetWorldTimerManager().SetTimer(EquippedDescriptionTimer, EquippedDescriptionTimerDelegate, EquippedDescriptionTimerDelay, false);
	});
	
	GetOwningPlayer()->GetWorld()->GetTimerManager().SetTimer(DescriptionTimer, DescriptionTimerDelegate, DescriptionTimerDelay, false);
}

void UInv_SimpleInventory::OnItemUnHovered()
{
	GetItemDescription()->SetVisibility(ESlateVisibility::Collapsed);
	GetOwningPlayer()->GetWorld()->GetTimerManager().ClearTimer(DescriptionTimer);
	GetEquippedItemDescription()->SetVisibility(ESlateVisibility::Collapsed);
	GetOwningPlayer()->GetWorldTimerManager().ClearTimer(EquippedDescriptionTimer);
}

bool UInv_SimpleInventory::HasHoverItem() const
{
	return ActiveGrid.IsValid() && ActiveGrid->HasHoverItem();
}

UInv_HoverItem* UInv_SimpleInventory::GetHoverItem() const
{
	return ActiveGrid.IsValid() ? ActiveGrid->GetHoverItem() : nullptr;
}

float UInv_SimpleInventory::GetTileSize() const
{
	return Grid_Equipables ? Grid_Equipables->GetTileSize() : 0.f;
}

void UInv_SimpleInventory::ShowEquipables()
{
	SetActiveGrid(Grid_Equipables, Button_Equipables);
}

void UInv_SimpleInventory::ShowConsumables()
{
	SetActiveGrid(Grid_Consumables, Button_Consumables);
}

void UInv_SimpleInventory::ShowCraftables()
{
	SetActiveGrid(Grid_Craftables, Button_Craftables);
}

void UInv_SimpleInventory::DisableButton(UButton* Button)
{
	if (!IsValid(Button_Equipables) || !IsValid(Button_Consumables) || !IsValid(Button_Craftables)) return;
	Button_Equipables->SetIsEnabled(true);
	Button_Consumables->SetIsEnabled(true);
	Button_Craftables->SetIsEnabled(true);
	if (IsValid(Button)) Button->SetIsEnabled(false);
}

void UInv_SimpleInventory::SetActiveGrid(UInv_MinimalInventoryGrid* Grid, UButton* Button)
{
	if (ActiveGrid.IsValid())
	{
		ActiveGrid->HideCursor();
		ActiveGrid->OnHide();
	}
	ActiveGrid = Grid;
	if (ActiveGrid.IsValid())
	{
		ActiveGrid->ShowCursor();
	}
	DisableButton(Button);
	if (IsValid(Switcher) && IsValid(Grid))
	{
		Switcher->SetActiveWidget(Grid);
	}
}

void UInv_SimpleInventory::ShowEquippedItemDescription(UInv_InventoryItem* Item)
{
	// Do not show equipped item description while holding a hover item
	if (HasHoverItem()) return;
	if (!IsValid(Item)) return;

	// Get the hovered item's equipment type (if any)
	const FInv_ItemManifest& Manifest = Item->GetItemManifest();
	const FInv_EquipmentFragment* EquipmentFragment = Manifest.GetFragmentOfType<FInv_EquipmentFragment>();
	if (!EquipmentFragment) return;
	const FGameplayTag HoveredEquipmentType = EquipmentFragment->GetEquipmentType();

	// If the hovered item is already equipped, do nothing
	auto EquippedGridSlotIt = EquippedGridSlots.FindByPredicate([Item](const UInv_EquippedGridSlot* GridSlot)
	{
		return IsValid(GridSlot) && GridSlot->GetInventoryItem() == Item;
	});
	if (EquippedGridSlotIt != nullptr) return;

	// Find an equipped slot with an item of the same equipment type (guard every deref)
	auto FoundEquippedSlotIt = EquippedGridSlots.FindByPredicate([HoveredEquipmentType](const UInv_EquippedGridSlot* GridSlot)
	{
		if (!IsValid(GridSlot)) return false;
		UInv_InventoryItem* EqItem = GridSlot->GetInventoryItem().Get();
		if (!IsValid(EqItem)) return false;
		const FInv_ItemManifest& EqManifest = EqItem->GetItemManifest();
		const FInv_EquipmentFragment* EqFrag = EqManifest.GetFragmentOfType<FInv_EquipmentFragment>();
		return EqFrag && EqFrag->GetEquipmentType() == HoveredEquipmentType;
	});
	UInv_EquippedGridSlot* EquippedSlot = FoundEquippedSlotIt ? *FoundEquippedSlotIt : nullptr;
	if (!IsValid(EquippedSlot)) return; // No equipped item with the same equipment type

	UInv_InventoryItem* EquippedItem = EquippedSlot->GetInventoryItem().Get();
	if (!IsValid(EquippedItem)) return;
	const FInv_ItemManifest& EquippedItemManifest = EquippedItem->GetItemManifest();

	UInv_ItemDescription* EquippedDescWidget = GetEquippedItemDescription();
	if (!IsValid(EquippedDescWidget)) return;
	EquippedDescWidget->Collapse();
	EquippedDescWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	EquippedItemManifest.AssimilateInventoryFragments(EquippedDescWidget);
}

UInv_ItemDescription* UInv_SimpleInventory::GetItemDescription()
{
	if (!IsValid(ItemDescription))
	{
		ItemDescription = CreateWidget<UInv_ItemDescription>(GetOwningPlayer(), ItemDescriptionClass);
		CanvasPanel->AddChild(ItemDescription);
	}
	return ItemDescription;
}

UInv_ItemDescription* UInv_SimpleInventory::GetEquippedItemDescription()
{
	if (!IsValid(EquippedItemDescription))
	{
		EquippedItemDescription = CreateWidget<UInv_ItemDescription>(GetOwningPlayer(), EquippedItemDescriptionClass);
		CanvasPanel->AddChild(EquippedItemDescription);
	}
	return EquippedItemDescription;
}

void UInv_SimpleInventory::SetItemDescriptionSizeAndPosition(UInv_ItemDescription* Description, UCanvasPanel* Canvas) const
{
	UCanvasPanelSlot* DescriptionCanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Description);
	if (!IsValid(DescriptionCanvasSlot)) return;
	const FVector2D DescriptionSize = Description->GetBoxSize();
	DescriptionCanvasSlot->SetSize(DescriptionSize);
	FVector2D ClampedPosition = UInv_WidgetUtils::GetClampedWidgetPosition(UInv_WidgetUtils::GetWidgetSize(Canvas), DescriptionSize, UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer()));
	DescriptionCanvasSlot->SetPosition(ClampedPosition);
}

void UInv_SimpleInventory::SetEquippedItemDescriptionSizeAndPosition(UInv_ItemDescription* Description, UInv_ItemDescription* EquippedDescription, UCanvasPanel* Canvas) const
{
	UCanvasPanelSlot* ItemDescriptionCPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(Description);
	UCanvasPanelSlot* EquippedItemDescriptionCPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(EquippedDescription);
	if (!IsValid(ItemDescriptionCPS) || !IsValid(EquippedItemDescriptionCPS)) return;
	const FVector2D ItemDescriptionSize = Description->GetBoxSize();
	const FVector2D EquippedItemDescriptionSize = EquippedDescription->GetBoxSize();
	FVector2D ClampedPosition = UInv_WidgetUtils::GetClampedWidgetPosition(
		UInv_WidgetUtils::GetWidgetSize(Canvas),
		ItemDescriptionSize,
		UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer()));
	ClampedPosition.X -= EquippedItemDescriptionSize.X;
	EquippedItemDescriptionCPS->SetSize(EquippedItemDescriptionSize);
	EquippedItemDescriptionCPS->SetPosition(ClampedPosition);
}

void UInv_SimpleInventory::EquippedGridSlotClicked(UInv_EquippedGridSlot* EquippedGridSlot, const FGameplayTag& EquipmentTypeTag)
{
	if (!CanEquipHoverItem(EquippedGridSlot, EquipmentTypeTag)) return;
	UInv_HoverItem* HoverItem = GetHoverItem();
	const float TileSizeLocal = GetTileSize();
	UInv_EquippedSlottedItem* EquippedSlottedItem = EquippedGridSlot->OnItemEquipped(
		HoverItem->GetInventoryItem(),
		EquipmentTypeTag,
		TileSizeLocal);
	EquippedSlottedItem->OnEquippedSlottedItemClicked.AddDynamic(this, &ThisClass::EquippedSlottedItemClicked);

	UInv_InventoryComponent* InventoryComponent = UInv_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
	check(IsValid(InventoryComponent));
	InventoryComponent->Server_EquipSlotClicked(HoverItem->GetInventoryItem(), nullptr);

	// Clear the hover item from the grid (use equipables grid since equipping is equipable-only)
	if (Grid_Equipables)
	{
		Grid_Equipables->ClearHoverItem();
	}
}

void UInv_SimpleInventory::EquippedSlottedItemClicked(UInv_EquippedSlottedItem* EquippedSlottedItem)
{
	UInv_InventoryStatics::ItemUnhovered(GetOwningPlayer());
	if (IsValid(GetHoverItem()) && GetHoverItem()->IsStackable()) return;

	UInv_InventoryItem* ItemToEquip = IsValid(GetHoverItem()) ? GetHoverItem()->GetInventoryItem() : nullptr;
	UInv_InventoryItem* ItemToUnequip = EquippedSlottedItem->GetInventoryItem();
	UInv_EquippedGridSlot* EquippedGridSlot = FindSlotWithEquippedItem(ItemToUnequip);
	ClearSlotOfItem(EquippedGridSlot);
	if (Grid_Equipables)
	{
		Grid_Equipables->AssignHoverItem(ItemToUnequip);
	}
	RemoveEquippedSlottedItem(EquippedSlottedItem);
	MakeEquippedSlottedItem(EquippedSlottedItem, EquippedGridSlot, ItemToEquip);
	BroadcastSlotClickedDelegates(ItemToEquip, ItemToUnequip);
}

bool UInv_SimpleInventory::CanEquipHoverItem(UInv_EquippedGridSlot* EquippedGridSlot, const FGameplayTag& EquipmentTypeTag) const
{
	if (!IsValid(EquippedGridSlot) || EquippedGridSlot->GetInventoryItem().IsValid()) return false;
	UInv_HoverItem* HoverItem = GetHoverItem();
	if (!IsValid(HoverItem)) return false;
	UInv_InventoryItem* HeldItem = HoverItem->GetInventoryItem();
	return HasHoverItem() && IsValid(HeldItem) && !HoverItem->IsStackable() &&
		HeldItem->GetItemManifest().GetItemCategory() == EInv_ItemCategory::Equipable &&
		HeldItem->GetItemManifest().GetItemType().MatchesTag(EquipmentTypeTag);
}

UInv_EquippedGridSlot* UInv_SimpleInventory::FindSlotWithEquippedItem(UInv_InventoryItem* EquippedItem) const
{
	auto* FoundEquippedGridSlot = EquippedGridSlots.FindByPredicate([EquippedItem](const UInv_EquippedGridSlot* GridSlot)
	{
		return GridSlot->GetInventoryItem() == EquippedItem;
	});
	return FoundEquippedGridSlot ? *FoundEquippedGridSlot : nullptr;
}

void UInv_SimpleInventory::ClearSlotOfItem(UInv_EquippedGridSlot* EquippedGridSlot)
{
	if (IsValid(EquippedGridSlot))
	{
		EquippedGridSlot->SetEquippedSlottedItem(nullptr);
		EquippedGridSlot->SetInventoryItem(nullptr);
	}
}

void UInv_SimpleInventory::RemoveEquippedSlottedItem(UInv_EquippedSlottedItem* EquippedSlottedItem)
{
	if (!IsValid(EquippedSlottedItem)) return;
	if (EquippedSlottedItem->OnEquippedSlottedItemClicked.IsAlreadyBound(this, &ThisClass::EquippedSlottedItemClicked))
	{
		EquippedSlottedItem->OnEquippedSlottedItemClicked.RemoveDynamic(this, &ThisClass::EquippedSlottedItemClicked);
	}
	EquippedSlottedItem->RemoveFromParent();
}

void UInv_SimpleInventory::MakeEquippedSlottedItem(UInv_EquippedSlottedItem* EquippedSlottedItem, UInv_EquippedGridSlot* EquippedGridSlot, UInv_InventoryItem* ItemToEquip)
{
	if (!IsValid(EquippedGridSlot)) return;
	UInv_EquippedSlottedItem* SlottedItem = EquippedGridSlot->OnItemEquipped(
		ItemToEquip,
		EquippedSlottedItem->GetEquipmentTypeTag(),
		GetTileSize());
	if (IsValid(SlottedItem)) SlottedItem->OnEquippedSlottedItemClicked.AddDynamic(this, &ThisClass::EquippedSlottedItemClicked);
	EquippedGridSlot->SetEquippedSlottedItem(SlottedItem);
}

void UInv_SimpleInventory::BroadcastSlotClickedDelegates(UInv_InventoryItem* ItemToEquip, UInv_InventoryItem* ItemToUnequip) const
{
	UInv_InventoryComponent* InventoryComponent = UInv_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
	check(IsValid(InventoryComponent));
	InventoryComponent->Server_EquipSlotClicked(ItemToEquip, ItemToUnequip);
}
