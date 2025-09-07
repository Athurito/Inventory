#pragma once

#include "CoreMinimal.h"
#include "Widgets/Inventory/InventoryBase/Inv_InventoryBase.h"
#include "Types/Inv_GridTypes.h"
#include "Inv_SimpleInventory.generated.h"

class UInv_MinimalInventoryGrid;
class UCanvasPanel;
class UButton;
class UWidgetSwitcher;
class UInv_ItemComponent;
class UInv_HoverItem;
class UInv_InventoryItem;

UCLASS()
class INVENTORY_API UInv_SimpleInventory : public UInv_InventoryBase
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// UInv_InventoryBase overrides
	virtual FInv_SlotAvailabilityResult HasRoomForItem(UInv_ItemComponent* ItemComponent) const override;
	virtual void OnItemHovered(UInv_InventoryItem* /*Item*/) override {}
	virtual void OnItemUnHovered() override {}
	virtual bool HasHoverItem() const override;
	virtual UInv_HoverItem* GetHoverItem() const override;
	virtual float GetTileSize() const override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> Switcher;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UInv_MinimalInventoryGrid> Grid_Equipables;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UInv_MinimalInventoryGrid> Grid_Consumables;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UInv_MinimalInventoryGrid> Grid_Craftables;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Equipables;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Consumables;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Craftables;

	UFUNCTION()
	void ShowEquipables();

	UFUNCTION()
	void ShowConsumables();

	UFUNCTION()
	void ShowCraftables();

	void DisableButton(UButton* Button);
	void SetActiveGrid(UInv_MinimalInventoryGrid* Grid, UButton* Button);

	TWeakObjectPtr<UInv_MinimalInventoryGrid> ActiveGrid;
};
