#include "Widgets/Inventory/Simple/Inv_SimpleInventory.h"

#include "Widgets/Inventory/Simple/Inv_SimpleInventoryGrid.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"
#include "Inventory.h"

void UInv_SimpleInventory::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (Button_Equipables)
    {
        Button_Equipables->OnClicked.AddDynamic(this, &ThisClass::ShowEquipables);
    }
    if (Button_Consumables)
    {
        Button_Consumables->OnClicked.AddDynamic(this, &ThisClass::ShowConsumables);
    }
    if (Button_Craftables)
    {
        Button_Craftables->OnClicked.AddDynamic(this, &ThisClass::ShowCraftables);
    }

    // Default to Equipables tab
    ShowEquipables();
}

FInv_SlotAvailabilityResult UInv_SimpleInventory::HasRoomForItem(UInv_ItemComponent* ItemComponent) const
{
    if (!IsValid(ItemComponent))
    {
        return FInv_SlotAvailabilityResult();
    }

    switch (UInv_InventoryStatics::GetItemCategoryFromItemComponent(ItemComponent))
    {
        case EInv_ItemCategory::Equipable:
            return IsValid(Grid_Equipables) ? Grid_Equipables->HasRoomForItem(ItemComponent) : FInv_SlotAvailabilityResult();
        case EInv_ItemCategory::Consumable:
            return IsValid(Grid_Consumables) ? Grid_Consumables->HasRoomForItem(ItemComponent) : FInv_SlotAvailabilityResult();
        case EInv_ItemCategory::Craftable:
            return IsValid(Grid_Craftables) ? Grid_Craftables->HasRoomForItem(ItemComponent) : FInv_SlotAvailabilityResult();
        default:
            UE_LOG(LogInventory, Error, TEXT("ItemComponent doesn't have a valid item category"));
            return FInv_SlotAvailabilityResult();
    }
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
    if (!Button_Equipables || !Button_Consumables || !Button_Craftables) return;

    Button_Equipables->SetIsEnabled(true);
    Button_Consumables->SetIsEnabled(true);
    Button_Craftables->SetIsEnabled(true);

    if (Button)
    {
        Button->SetIsEnabled(false);
    }
}

void UInv_SimpleInventory::SetActiveGrid(UInv_SimpleInventoryGrid* Grid, UButton* Button)
{
    DisableButton(Button);
    if (Switcher && Grid)
    {
        Switcher->SetActiveWidget(Grid);
    }
}
