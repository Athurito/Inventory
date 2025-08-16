#pragma once

#include "CoreMinimal.h"
#include "Widgets/Inventory/InventoryBase/Inv_InventoryBase.h"
#include "Types/Inv_GridTypes.h"
#include "GameplayTagContainer.h"
#include "Inv_SimpleInventory.generated.h"

struct FGameplayTag;

class UCanvasPanel;
class UInv_GridSlot;
class UInv_SlottedItem;
class UInv_ItemComponent;
class UInv_InventoryItem;
class UInv_InventoryComponent;

UCLASS()
class INVENTORY_API UInv_SimpleInventory : public UInv_InventoryBase
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	// UInv_InventoryBase
	virtual FInv_SlotAvailabilityResult HasRoomForItem(UInv_ItemComponent* ItemComponent) const override;

protected:
	UFUNCTION()
	void OnItemAdded(UInv_InventoryItem* Item);

	void ConstructGrid();
	int32 FindFirstEmptySlotIndex() const;
	int32 FindExistingStackIndex(const FGameplayTag& ItemType, int32 MaxStackSize) const;
	void PlaceItemInSlot(UInv_InventoryItem* Item, int32 SlotIndex, int32 StackAmount);

private:
	// Layout
	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 Rows = 5;
	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 Columns = 6;
	UPROPERTY(EditAnywhere, Category = "Inventory")
	float TileSize = 64.f;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UInv_GridSlot> GridSlotClass;
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UInv_SlottedItem> SlottedItemClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanel;

	UPROPERTY()
	TArray<TObjectPtr<UInv_GridSlot>> GridSlots;

	UPROPERTY()
	TMap<int32, TObjectPtr<UInv_SlottedItem>> SlottedItems;

	UPROPERTY()
	TWeakObjectPtr<UInv_InventoryComponent> InventoryComponent;
};
