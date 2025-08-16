#pragma once

#include "CoreMinimal.h"
#include "Widgets/Inventory/InventoryBase/Inv_InventoryBase.h"
#include "Types/Inv_GridTypes.h"
#include "Inv_SimpleInventory.generated.h"

class UInv_ItemComponent;
class UInv_SimpleInventoryGrid;

UCLASS()
class INVENTORY_API UInv_SimpleInventory : public UInv_InventoryBase
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	// UInv_InventoryBase
	virtual FInv_SlotAvailabilityResult HasRoomForItem(UInv_ItemComponent* ItemComponent) const override;

private:
	// The grid widget this window contains (bind in UMG BP)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UInv_SimpleInventoryGrid> Grid_Simple;
};
