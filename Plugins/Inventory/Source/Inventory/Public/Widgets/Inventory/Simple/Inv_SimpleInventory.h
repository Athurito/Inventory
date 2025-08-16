#pragma once

#include "CoreMinimal.h"
#include "Widgets/Inventory/InventoryBase/Inv_InventoryBase.h"
#include "Types/Inv_GridTypes.h"
#include "Inv_SimpleInventory.generated.h"

class UInv_ItemComponent;
class UInv_SimpleInventoryGrid;
class UButton;
class UWidgetSwitcher;

UCLASS()
class INVENTORY_API UInv_SimpleInventory : public UInv_InventoryBase
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	// UInv_InventoryBase
	virtual FInv_SlotAvailabilityResult HasRoomForItem(UInv_ItemComponent* ItemComponent) const override;

private:
	// Switcher and category grids (bind these in the UMG BP)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> Switcher;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UInv_SimpleInventoryGrid> Grid_Equipables;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UInv_SimpleInventoryGrid> Grid_Consumables;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UInv_SimpleInventoryGrid> Grid_Craftables;

	// Category buttons
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Equipables;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Consumables;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Craftables;

	// Handlers
	UFUNCTION()
	void ShowEquipables();
	UFUNCTION()
	void ShowConsumables();
	UFUNCTION()
	void ShowCraftables();

	void DisableButton(UButton* Button);
	void SetActiveGrid(UInv_SimpleInventoryGrid* Grid, UButton* Button);
};
