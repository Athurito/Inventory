#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Types/Inv_GridTypes.h"
#include "GameplayTagContainer.h"
#include "Inv_SimpleInventoryGrid.generated.h"

class UCanvasPanel;
class UInv_GridSlot;
class UInv_SlottedItem;
class UInv_ItemComponent;
class UInv_InventoryItem;
class UInv_InventoryComponent;
class UTexture2D;

UCLASS()
class INVENTORY_API UInv_SimpleInventoryGrid : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	FInv_SlotAvailabilityResult HasRoomForItem(const UInv_ItemComponent* ItemComponent) const;

	EInv_ItemCategory GetItemCategory() const { return ItemCategory; }

private:
	UFUNCTION()
	void OnItemAdded(UInv_InventoryItem* Item);

	void ConstructGrid();
	int32 FindFirstEmptySlotIndex() const;
	int32 FindExistingStackIndex(const FGameplayTag& ItemType, int32 MaxStackSize) const;
	void PlaceItemInSlot(UInv_InventoryItem* Item, int32 SlotIndex, int32 StackAmount);
	void SetSlottedItemImage(const UInv_SlottedItem* SlottedItem, UTexture2D* Icon, const FVector2D& DrawSize) const;

private:
	// Category for this grid (set in UMG BP)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Inventory")
	EInv_ItemCategory ItemCategory = EInv_ItemCategory::None;

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