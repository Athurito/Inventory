#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Types/Inv_GridTypes.h"
#include "Inv_MinimalInventoryGrid.generated.h"

class UInv_ItemPopUp;
class UInv_HoverItem;
class UInv_SlottedItem;
class UCanvasPanel;
class UInv_GridSlot;
class UInv_InventoryItem;
class UInv_InventoryComponent;
class UInv_ItemComponent;
struct FInv_ItemManifest;
struct FInv_GridFragment;
struct FInv_ImageFragment;

// A truly minimal, Valheim-like inventory grid:
// - 1 slot = 1 item (all items treated as 1x1)
// - If stackable: fill existing stacks of same type first, then use empty slots
// - No spatial quadrants, no multi-tile logic
// Keeps only the public API used by SpatialInventory
UCLASS()
class INVENTORY_API UInv_MinimalInventoryGrid : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	EInv_ItemCategory GetItemCategory() const { return ItemCategory; }
	FInv_SlotAvailabilityResult HasRoomForItem(const UInv_ItemComponent* ItemComponent);

	// Allow overriding which inventory component this grid listens to (useful for container windows)
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void SetInventoryComponent(UInv_InventoryComponent* InInventoryComponent);

	// Moves the current hover item from this grid's inventory to the target inventory
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void TransferHoverItemToInventory(UInv_InventoryComponent* TargetInventory);

	// Accessor for Blueprints to get the grid's bound inventory component
	UFUNCTION(BlueprintCallable, Category="Inventory")
	UInv_InventoryComponent* GetInventoryComponent() const { return InventoryComponent.Get(); }

	void ShowCursor();
	void HideCursor();
	void SetOwningCanvas(UCanvasPanel* OwningCanvas);
	void DropItem();
	bool HasHoverItem() const;
	UInv_HoverItem* GetHoverItem() const;
	float GetTileSize() const { return TileSize; }
	void ClearHoverItem();
	void AssignHoverItem(UInv_InventoryItem* InventoryItem);
	void OnHide();

	UFUNCTION()
	void AddItem(UInv_InventoryItem* Item);

private:
	// Data
	TWeakObjectPtr<UInv_InventoryComponent> InventoryComponent;
	TWeakObjectPtr<UCanvasPanel> OwningCanvasPanel;

	// Construction
	void ConstructGrid();

	// Core placement
	FInv_SlotAvailabilityResult HasRoomForItem(const UInv_InventoryItem* Item, int32 StackAmountOverride = -1);
	FInv_SlotAvailabilityResult HasRoomForItem(const FInv_ItemManifest& Manifest, int32 StackAmountOverride = -1);

	// UI/placement helpers
	void AddItemToIndices(const FInv_SlotAvailabilityResult& Result, UInv_InventoryItem* Item);
	bool MatchesCategory(const UInv_InventoryItem* Item) const;
	FVector2D GetDrawSize(const FInv_GridFragment* GridFragment) const;
	void SetSlottedItemImage(const UInv_SlottedItem* SlottedItem, const FInv_GridFragment* GridFragment, const FInv_ImageFragment* ImageFragment) const;
	void AddItemAtIndex(UInv_InventoryItem* Item, int32 Index, bool bStackable, int32 StackAmount);
	UInv_SlottedItem* CreateSlottedItem(UInv_InventoryItem* Item,
		bool bStackable,
		int32 StackAmount,
		const FInv_GridFragment* GridFragment,
		const FInv_ImageFragment* ImageFragment,
		int32 Index);
	void UpdateGridSlots(UInv_InventoryItem* Item, int32 Index, bool bStackableItem, int32 StackAmount);
	bool IsInGridBounds(int32 Index) const;

	// Simple hover tracking (single slot only)
	void UpdateHoveredIndex(const FVector2D& CanvasPosition, const FVector2D& MousePosition);
	void HighlightIndex(int32 Index);
	void UnHighlightIndex(int32 Index);
	bool CursorExitedCanvas(const FVector2D& BoundaryPos, const FVector2D& BoundarySize, const FVector2D& Location);

	// Click/stacking behavior (reduced)
	bool IsRightClick(const FPointerEvent& MouseEvent) const;
	bool IsLeftClick(const FPointerEvent& MouseEvent) const;
	bool IsSameStackable(const UInv_InventoryItem* ClickedInventoryItem) const;
	void SwapWithHoverItem(UInv_InventoryItem* ClickedInventoryItem, int32 GridIndex);
	bool ShouldSwapStackCounts(int32 RoomInClickedSlot, int32 HoveredStackCount, int32 MaxStackSize) const;
	void SwapStackCounts(int32 ClickedStackCount, int32 HoveredStackCount, int32 Index);
	bool ShouldConsumeHoverItemStacks(int32 HoveredStackCount, int32 RoomInClickedSlot) const;
	void ConsumeHoverItemStacks(int32 ClickedStackCount, int32 HoveredStackCount, int32 Index);
	bool ShouldFillInStack(int32 RoomInClickedSlot, int32 HoveredStackCount) const;
	void FillInStack(int32 FillAmount, int32 Remainder, int32 Index);
	void PickUp(UInv_InventoryItem* ClickedInventoryItem, int32 GridIndex);
	void RemoveItemFromGrid(UInv_InventoryItem* Item, int32 GridIndex);
	void PutDownOnIndex(int32 Index);

	// PopUp actions (kept minimal)
	void CreateItemPopUp(int32 GridIndex);
	void PutHoverItemBack();

	// Events
	UFUNCTION() void AddStacks(const FInv_SlotAvailabilityResult& Result);
	UFUNCTION() void OnSlottedItemClicked(int32 GridIndex, const FPointerEvent& MouseEvent);
	UFUNCTION() void OnGridSlotClicked(int32 GridIndex, const FPointerEvent& MouseEvent);
	UFUNCTION() void OnGridSlotHovered(int32 GridIndex, const FPointerEvent& MouseEvent);
	UFUNCTION() void OnGridSlotUnhovered(int32 GridIndex, const FPointerEvent& MouseEvent);
	UFUNCTION() void OnPopUpMenuSplit(int32 SplitAmount, int32 Index);
	UFUNCTION() void OnPopUpMenuDrop(int32 Index);
	UFUNCTION() void OnPopUpConsume(int32 Index);
	UFUNCTION() void OnInventoryMenuToggled(bool bOpen);

private:
	// Config and bindings (same names so UMG bindings work)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"), Category="Inventory")
	EInv_ItemCategory ItemCategory;

	UPROPERTY()
	TArray<TObjectPtr<UInv_GridSlot>> GridSlots;

	UPROPERTY(EditAnywhere, Category="Inventory")
	TSubclassOf<UInv_GridSlot> GridSlotClass;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanel;

	UPROPERTY(EditAnywhere, Category="Inventory")
	TSubclassOf<UInv_SlottedItem> SlottedItemClass;

	UPROPERTY()
	TMap<int32, TObjectPtr<UInv_SlottedItem>> SlottedItems;

	UPROPERTY(EditAnywhere, Category="Inventory")
	int32 Rows{0};

	UPROPERTY(EditAnywhere, Category="Inventory")
	int32 Columns{0};

	UPROPERTY(EditAnywhere, Category="Inventory")
	float TileSize{64.f};

	UPROPERTY(EditAnywhere, Category="Inventory")
	TSubclassOf<UInv_HoverItem> HoverItemClass;

	UPROPERTY(EditAnywhere, Category="Inventory")
	FVector2D ItemPopUpOffset{8.f, 8.f};

	UPROPERTY(EditAnywhere, Category="Inventory")
	TSubclassOf<UInv_ItemPopUp> ItemPopUpClass;

	UPROPERTY() TObjectPtr<UInv_ItemPopUp> ItemPopUp;
	UPROPERTY() TObjectPtr<UInv_HoverItem> HoverItem;
	UPROPERTY() TObjectPtr<UUserWidget> VisibleCursorWidget;
	UPROPERTY() TObjectPtr<UUserWidget> HiddenCursorWidget;
	UPROPERTY(EditAnywhere, Category="Inventory") TSubclassOf<UUserWidget> VisibleCursorWidgetClass;
	UPROPERTY(EditAnywhere, Category="Inventory") TSubclassOf<UUserWidget> HiddenCursorWidgetClass;

	// Hover helpers
	int32 HoveredIndex{INDEX_NONE};
	int32 LastHighlightedIndex{INDEX_NONE};
	bool bMouseWithinCanvas{false};
	bool bLastMouseWithinCanvas{false};
};
