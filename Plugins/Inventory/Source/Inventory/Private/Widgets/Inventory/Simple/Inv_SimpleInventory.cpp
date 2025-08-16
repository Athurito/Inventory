#include "Widgets/Inventory/Simple/Inv_SimpleInventory.h"

#include "Widgets/Inventory/Simple/Inv_SimpleInventoryGrid.h"
#include "Items/Components/Inv_ItemComponent.h"

void UInv_SimpleInventory::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// This class is now just a window that contains the simple grid.
}

FInv_SlotAvailabilityResult UInv_SimpleInventory::HasRoomForItem(UInv_ItemComponent* ItemComponent) const
{
	if (!IsValid(ItemComponent) || !IsValid(Grid_Simple))
	{
		return FInv_SlotAvailabilityResult();
	}
	return Grid_Simple->HasRoomForItem(ItemComponent);
}
