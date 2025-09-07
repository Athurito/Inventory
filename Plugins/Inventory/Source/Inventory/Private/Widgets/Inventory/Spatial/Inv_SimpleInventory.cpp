#include "Widgets/Inventory/Spatial/Inv_SimpleInventory.h"

#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Components/CanvasPanel.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"
#include "Widgets/Inventory/Spatial/Inv_MinimalInventoryGrid.h"
#include "Items/Components/Inv_ItemComponent.h"

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
}

FReply UInv_SimpleInventory::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (ActiveGrid.IsValid())
	{
		ActiveGrid->DropItem();
	}
	return FReply::Handled();
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
