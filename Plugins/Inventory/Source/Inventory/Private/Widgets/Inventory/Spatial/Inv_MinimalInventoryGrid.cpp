#include "Widgets/Inventory/Spatial/Inv_MinimalInventoryGrid.h"

#include "Inventory.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"
#include "Items/Inv_InventoryItem.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Items/Fragments/Inv_FragmentTags.h"
#include "Items/Fragments/Inv_ItemFragment.h"
#include "Widgets/Inventory/GridSlots/Inv_GridSlot.h"
#include "Widgets/Inventory/HoverItem/Inv_HoverItem.h"
#include "Widgets/Inventory/SlottedItems/Inv_SlottedItem.h"
#include "Widgets/ItemPopUp/Inv_ItemPopUp.h"
#include "Widgets/Utils/Inv_WidgetUtils.h"

void UInv_MinimalInventoryGrid::SetInventoryComponent(UInv_InventoryComponent* InInventoryComponent)
{
	// Unbind from previous component if any
	if (InventoryComponent.IsValid())
	{
		InventoryComponent->OnItemAdded.RemoveAll(this);
		InventoryComponent->OnStackChange.RemoveAll(this);
		InventoryComponent->OnInventoryMenuToggled.RemoveAll(this);
	}

	InventoryComponent = InInventoryComponent;

	if (InventoryComponent.IsValid())
	{
		InventoryComponent->OnItemAdded.AddDynamic(this, &ThisClass::AddItem);
		InventoryComponent->OnStackChange.AddDynamic(this, &ThisClass::AddStacks);
		InventoryComponent->OnInventoryMenuToggled.AddDynamic(this, &ThisClass::OnInventoryMenuToggled);
	}
}

void UInv_MinimalInventoryGrid::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ConstructGrid();

	// If an external inventory component was not assigned, default to the player's inventory
	if (!InventoryComponent.IsValid())
	{
		InventoryComponent = UInv_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
	}
	if (InventoryComponent.IsValid())
	{
		InventoryComponent->OnItemAdded.AddDynamic(this, &ThisClass::AddItem);
		InventoryComponent->OnStackChange.AddDynamic(this, &ThisClass::AddStacks);
		InventoryComponent->OnInventoryMenuToggled.AddDynamic(this, &ThisClass::OnInventoryMenuToggled);
	}
}

void UInv_MinimalInventoryGrid::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!IsValid(CanvasPanel)) return;

	const FVector2D CanvasPos = UInv_WidgetUtils::GetWidgetPosition(CanvasPanel);
	const FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());

	if (CursorExitedCanvas(CanvasPos, UInv_WidgetUtils::GetWidgetSize(CanvasPanel), MousePosition))
	{
		return;
	}
	UpdateHoveredIndex(CanvasPos, MousePosition);
}

void UInv_MinimalInventoryGrid::ConstructGrid()
{
	GridSlots.Reserve(Rows * Columns);
	for (int32 i = 0; i < Rows; i++)
	{
		for (int32 j = 0; j < Columns; j++)
		{
			UInv_GridSlot* GridSlot = CreateWidget<UInv_GridSlot>(this, GridSlotClass);
			CanvasPanel->AddChild(GridSlot);

			const FIntPoint TilePosition(j, i);
			const int32 Index = UInv_WidgetUtils::GetIndexFromPosition(TilePosition, Columns);
			GridSlot->SetTileIndex(Index);

			UCanvasPanelSlot* GridCanvasPanelSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(GridSlot);
			GridCanvasPanelSlot->SetSize(FVector2D(TileSize));
			GridCanvasPanelSlot->SetPosition(TilePosition * TileSize);

			GridSlots.Add(GridSlot);

			GridSlot->GridSlotClicked.AddDynamic(this, &ThisClass::OnGridSlotClicked);
			GridSlot->GridSlotHovered.AddDynamic(this, &ThisClass::OnGridSlotHovered);
			GridSlot->GridSlotUnhovered.AddDynamic(this, &ThisClass::OnGridSlotUnhovered);
		}
	}
}

void UInv_MinimalInventoryGrid::UpdateHoveredIndex(const FVector2D& CanvasPosition, const FVector2D& MousePosition)
{
	if (!bMouseWithinCanvas) return;
	const int32 X = FMath::FloorToInt32((MousePosition.X - CanvasPosition.X) / TileSize);
	const int32 Y = FMath::FloorToInt32((MousePosition.Y - CanvasPosition.Y) / TileSize);
	const int32 Index = UInv_WidgetUtils::GetIndexFromPosition(FIntPoint(X, Y), Columns);
	if (Index != HoveredIndex)
	{
		UnHighlightIndex(LastHighlightedIndex);
		HoveredIndex = Index;
		HighlightIndex(HoveredIndex);
		LastHighlightedIndex = HoveredIndex;
	}
}

void UInv_MinimalInventoryGrid::HighlightIndex(int32 Index)
{
	if (!GridSlots.IsValidIndex(Index)) return;
	GridSlots[Index]->SetOccupiedTexture();
}

void UInv_MinimalInventoryGrid::UnHighlightIndex(int32 Index)
{
	if (!GridSlots.IsValidIndex(Index)) return;
	if (GridSlots[Index]->IsAvailable())
	{
		GridSlots[Index]->SetUnoccupiedTexture();
	}
	else
	{
		GridSlots[Index]->SetOccupiedTexture();
	}
}

bool UInv_MinimalInventoryGrid::CursorExitedCanvas(const FVector2D& BoundaryPos, const FVector2D& BoundarySize, const FVector2D& Location)
{
	bLastMouseWithinCanvas = bMouseWithinCanvas;
	bMouseWithinCanvas = UInv_WidgetUtils::IsWithinBounds(BoundaryPos, BoundarySize, Location);
	if (!bMouseWithinCanvas && bLastMouseWithinCanvas)
	{
		UnHighlightIndex(LastHighlightedIndex);
		// Reset hover tracking so that re-entering highlights immediately even if it's the same index as before
		HoveredIndex = INDEX_NONE;
		LastHighlightedIndex = INDEX_NONE;
		return true;
	}
	return false;
}

FInv_SlotAvailabilityResult UInv_MinimalInventoryGrid::HasRoomForItem(const UInv_ItemComponent* ItemComponent)
{
	if (!IsValid(ItemComponent)) return FInv_SlotAvailabilityResult();
	return HasRoomForItem(ItemComponent->GetItemManifest());
}

FInv_SlotAvailabilityResult UInv_MinimalInventoryGrid::HasRoomForItem(const UInv_InventoryItem* Item, int32 StackAmountOverride)
{
	if (!IsValid(Item)) return FInv_SlotAvailabilityResult();
	return HasRoomForItem(Item->GetItemManifest(), StackAmountOverride);
}

FInv_SlotAvailabilityResult UInv_MinimalInventoryGrid::HasRoomForItem(const FInv_ItemManifest& Manifest, int32 StackAmountOverride)
{
	FInv_SlotAvailabilityResult Result;

	const FInv_StackableFragment* Stackable = Manifest.GetFragmentOfType<FInv_StackableFragment>();
	const bool bStackable = Stackable != nullptr;
	const int32 MaxStack = Stackable ? FMath::Max(1, Stackable->GetMaxStackSize()) : 1;
	int32 AmountToPlace = Stackable ? FMath::Max(1, Stackable->GetStackCount()) : 1;
	if (StackAmountOverride > 0 && bStackable)
	{
		AmountToPlace = StackAmountOverride;
	}
	Result.bStackable = bStackable;

	const FGameplayTag ItemType = Manifest.GetItemType();

	// Pass 1: fill existing stacks of same type
	if (bStackable)
	{
		for (int32 i = 0; i < GridSlots.Num() && AmountToPlace > 0; ++i)
		{
			UInv_GridSlot* GridSlotLocal = GridSlots[i];
			if (!IsValid(GridSlotLocal)) continue;
			UInv_InventoryItem* Existing = GridSlotLocal->GetInventoryItem().Get();
			if (!IsValid(Existing)) continue;
			if (!Existing->IsStackable()) continue;
			if (!Existing->GetItemManifest().GetItemType().MatchesTagExact(ItemType)) continue;
			const int32 ExistingCount = GridSlotLocal->GetStackCount();
			const int32 Room = FMath::Max(0, MaxStack - ExistingCount);
			if (Room <= 0) continue;
			const int32 ToAdd = FMath::Min(Room, AmountToPlace);
			Result.SlotAvailabilities.Add(FInv_SlotAvailability(i, ToAdd, true));
			Result.TotalRoomToFill += ToAdd;
			AmountToPlace -= ToAdd;
		}
	}

	// Pass 2: place in empty slots
	for (int32 i = 0; i < GridSlots.Num() && AmountToPlace > 0; ++i)
	{
		UInv_GridSlot* GridSlotLocal = GridSlots[i];
		if (!IsValid(GridSlotLocal)) continue;
		if (GridSlotLocal->GetInventoryItem().IsValid()) continue;

		const int32 ToPlace = bStackable ? FMath::Min(MaxStack, AmountToPlace) : 1;
		Result.SlotAvailabilities.Add(FInv_SlotAvailability(i, ToPlace, false));
		Result.TotalRoomToFill += ToPlace;
		AmountToPlace -= ToPlace;
	}

	Result.Remainder = AmountToPlace;
	return Result;
}

void UInv_MinimalInventoryGrid::AddItem(UInv_InventoryItem* Item)
{
	if (!MatchesCategory(Item)) return;
	FInv_SlotAvailabilityResult Result = HasRoomForItem(Item);
	AddItemToIndices(Result, Item);
}

void UInv_MinimalInventoryGrid::AddItemToIndices(const FInv_SlotAvailabilityResult& Result, UInv_InventoryItem* Item)
{
	for (const auto& S : Result.SlotAvailabilities)
	{
		AddItemAtIndex(Item, S.Index, Result.bStackable, S.AmountToFill);
		UpdateGridSlots(Item, S.Index, Result.bStackable, S.AmountToFill);
	}
}

bool UInv_MinimalInventoryGrid::MatchesCategory(const UInv_InventoryItem* Item) const
{
	return Item->GetItemManifest().GetItemCategory() == ItemCategory;
}

FVector2D UInv_MinimalInventoryGrid::GetDrawSize(const FInv_GridFragment* GridFragment) const
{
	const float LocalPadding = GridFragment ? GridFragment->GetGridPadding() : 0.f;
	const float IconTileWidth = TileSize - LocalPadding * 2;
	return FVector2D(IconTileWidth, IconTileWidth);
}

void UInv_MinimalInventoryGrid::SetSlottedItemImage(const UInv_SlottedItem* SlottedItem, const FInv_GridFragment* GridFragment, const FInv_ImageFragment* ImageFragment) const
{
	FSlateBrush Brush;
	Brush.SetResourceObject(ImageFragment->GetIcon());
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = GetDrawSize(GridFragment);
	const_cast<UInv_SlottedItem*>(SlottedItem)->SetImageBrush(Brush);
}

void UInv_MinimalInventoryGrid::AddItemAtIndex(UInv_InventoryItem* Item, int32 Index, bool bStackable, int32 StackAmount)
{
	const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(Item, FragmentTags::GridFragment);
	const FInv_ImageFragment* ImageFragment = GetFragment<FInv_ImageFragment>(Item, FragmentTags::ImageFragment);
	if (!GridFragment || !ImageFragment) return;

	UInv_SlottedItem* SlottedItem = CreateSlottedItem(Item, bStackable, StackAmount, GridFragment, ImageFragment, Index);
	CanvasPanel->AddChild(SlottedItem);
	UCanvasPanelSlot* GridCanvasPanelSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(SlottedItem);
	GridCanvasPanelSlot->SetSize(GetDrawSize(GridFragment));
	const FVector2D DrawPos = UInv_WidgetUtils::GetPositionFromIndex(Index, Columns) * TileSize;
	const FVector2D DrawPosWithPadding = DrawPos + FVector2D(GridFragment->GetGridPadding());
	GridCanvasPanelSlot->SetPosition(DrawPosWithPadding);
	SlottedItems.Add(Index, SlottedItem);
}

UInv_SlottedItem* UInv_MinimalInventoryGrid::CreateSlottedItem(UInv_InventoryItem* Item, bool bStackable, int32 StackAmount, const FInv_GridFragment* GridFragment, const FInv_ImageFragment* ImageFragment, int32 Index)
{
	UInv_SlottedItem* SlottedItem = CreateWidget<UInv_SlottedItem>(GetOwningPlayer(), SlottedItemClass);
	SlottedItem->SetInventoryItem(Item);
	SetSlottedItemImage(SlottedItem, GridFragment, ImageFragment);
	SlottedItem->SetGridIndex(Index);
	SlottedItem->SetIsStackable(bStackable);
	SlottedItem->UpdateStackCount(bStackable ? StackAmount : 0);
	SlottedItem->OnSlottedItemClicked.AddDynamic(this, &ThisClass::OnSlottedItemClicked);
	return SlottedItem;
}

void UInv_MinimalInventoryGrid::UpdateGridSlots(UInv_InventoryItem* Item, int32 Index, bool bStackableItem, int32 StackAmount)
{
	if (!GridSlots.IsValidIndex(Index)) return;
	UInv_GridSlot* GridSlotLocal = GridSlots[Index];
	if (bStackableItem)
	{
		GridSlotLocal->SetStackCount(StackAmount);
	}
	GridSlotLocal->SetInventoryItem(Item);
	GridSlotLocal->SetUpperLeftIndex(Index);
	GridSlotLocal->SetOccupiedTexture();
	GridSlotLocal->SetAvailable(false);
}

bool UInv_MinimalInventoryGrid::IsInGridBounds(int32 Index) const
{
	return Index >= 0 && Index < GridSlots.Num();
}

bool UInv_MinimalInventoryGrid::HasHoverItem() const
{
	// Only report the grid's own local hover item state to avoid recursion with global statics lookup
	return IsValid(HoverItem);
}

UInv_HoverItem* UInv_MinimalInventoryGrid::GetHoverItem() const
{
	// Only return the grid's local hover item to avoid recursion
	return IsValid(HoverItem) ? HoverItem : nullptr;
}

void UInv_MinimalInventoryGrid::AssignHoverItem(UInv_InventoryItem* InventoryItem)
{
	if (!IsValid(InventoryItem)) return;
	if (!IsValid(HoverItem))
	{
		HoverItem = CreateWidget<UInv_HoverItem>(GetOwningPlayer(), HoverItemClass);
	}
	const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(InventoryItem, FragmentTags::GridFragment);
	const FInv_ImageFragment* ImageFragment = GetFragment<FInv_ImageFragment>(InventoryItem, FragmentTags::ImageFragment);
	if (!GridFragment || !ImageFragment) return;

	FSlateBrush IconBrush;
	IconBrush.SetResourceObject(ImageFragment->GetIcon());
	IconBrush.DrawAs = ESlateBrushDrawType::Image;
	IconBrush.ImageSize = GetDrawSize(GridFragment) * UWidgetLayoutLibrary::GetViewportScale(this);
	HoverItem->SetImageBrush(IconBrush);
	HoverItem->SetGridDimensions(FIntPoint(1,1));
	HoverItem->SetInventoryItem(InventoryItem);
	HoverItem->SetIsStackable(InventoryItem->IsStackable());

	// Remember source inventory for cross-inventory auto transfer
	HoverItem->SetSourceInventory(InventoryComponent);

	GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Default, HoverItem);
}

void UInv_MinimalInventoryGrid::OnHide()
{
	PutHoverItemBack();
}

void UInv_MinimalInventoryGrid::PickUp(UInv_InventoryItem* ClickedInventoryItem, int32 GridIndex)
{
	AssignHoverItem(ClickedInventoryItem);
	HoverItem->SetPreviousIndex(GridIndex);
	HoverItem->UpdateStackCount(ClickedInventoryItem->IsStackable() ? GridSlots[GridIndex]->GetStackCount() : 0);
	RemoveItemFromGrid(ClickedInventoryItem, GridIndex);
}

void UInv_MinimalInventoryGrid::RemoveItemFromGrid(UInv_InventoryItem* Item, int32 GridIndex)
{
	if (!GridSlots.IsValidIndex(GridIndex)) return;
	UInv_GridSlot* GridSlotLocal = GridSlots[GridIndex];
	GridSlotLocal->SetInventoryItem(nullptr);
	GridSlotLocal->SetUpperLeftIndex(INDEX_NONE);
	GridSlotLocal->SetUnoccupiedTexture();
	GridSlotLocal->SetAvailable(true);
	GridSlotLocal->SetStackCount(0);
	if (SlottedItems.Contains(GridIndex))
	{
		TObjectPtr<UInv_SlottedItem> Found;
		SlottedItems.RemoveAndCopyValue(GridIndex, Found);
		if (IsValid(Found)) Found->RemoveFromParent();
	}
}

void UInv_MinimalInventoryGrid::PutDownOnIndex(int32 Index)
{
	if (!IsValid(HoverItem)) return;
	AddItemAtIndex(HoverItem->GetInventoryItem(), Index, HoverItem->IsStackable(), HoverItem->GetStackCount());
	UpdateGridSlots(HoverItem->GetInventoryItem(), Index, HoverItem->IsStackable(), HoverItem->GetStackCount());
	ClearHoverItem();
}

void UInv_MinimalInventoryGrid::TransferHoverItemToInventory(UInv_InventoryComponent* TargetInventory)
{
	if (!InventoryComponent.IsValid()) return;
	if (!IsValid(TargetInventory)) return;
	UInv_HoverItem* LocalHover = GetHoverItem();
	if (!IsValid(LocalHover)) return;
	UInv_InventoryItem* Item = LocalHover->GetInventoryItem();
	if (!IsValid(Item)) return;

	const int32 Amount = LocalHover->IsStackable() ? LocalHover->GetStackCount() : 1;
	InventoryComponent->Server_TransferItemToInventory(TargetInventory, Item, Amount);
	ClearHoverItem();
	ShowCursor();
}

void UInv_MinimalInventoryGrid::ClearHoverItem()
{
	// Try to clear our own local hover first
	if (IsValid(HoverItem))
	{
		HoverItem->SetInventoryItem(nullptr);
		HoverItem->SetIsStackable(false);
		HoverItem->SetPreviousIndex(INDEX_NONE);
		HoverItem->UpdateStackCount(0);
		HoverItem->SetImageBrush(FSlateNoResource());
		HoverItem->RemoveFromParent();
		HoverItem = nullptr;
		ShowCursor();
		return;
	}
	// Otherwise, if the hover item is owned by the full inventory UI, clear that one
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (UInv_HoverItem* ExternalHover = UInv_InventoryStatics::GetHoverItem(PC))
		{
			ExternalHover->SetInventoryItem(nullptr);
			ExternalHover->SetIsStackable(false);
			ExternalHover->SetPreviousIndex(INDEX_NONE);
			ExternalHover->UpdateStackCount(0);
			ExternalHover->SetImageBrush(FSlateNoResource());
			ExternalHover->RemoveFromParent();
			ShowCursor();
		}
	}
}

void UInv_MinimalInventoryGrid::ShowCursor()
{
	if (!IsValid(GetOwningPlayer())) return;
	if (!IsValid(VisibleCursorWidget))
	{
		VisibleCursorWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), VisibleCursorWidgetClass);
	}
	GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Default, VisibleCursorWidget);
}

void UInv_MinimalInventoryGrid::HideCursor()
{
	if (!IsValid(GetOwningPlayer())) return;
	if (!IsValid(HiddenCursorWidget))
	{
		HiddenCursorWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), HiddenCursorWidgetClass);
	}
	GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Default, HiddenCursorWidget);
}

void UInv_MinimalInventoryGrid::SetOwningCanvas(UCanvasPanel* OwningCanvas)
{
	OwningCanvasPanel = OwningCanvas;
}

bool UInv_MinimalInventoryGrid::IsRightClick(const FPointerEvent& MouseEvent) const
{
	return MouseEvent.GetEffectingButton() == EKeys::RightMouseButton;
}

bool UInv_MinimalInventoryGrid::IsLeftClick(const FPointerEvent& MouseEvent) const
{
	return MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;
}

bool UInv_MinimalInventoryGrid::IsSameStackable(const UInv_InventoryItem* ClickedInventoryItem) const
{
	if (!IsValid(HoverItem)) return false;
	const bool bIsSameItem = ClickedInventoryItem == HoverItem->GetInventoryItem();
	const bool bIsStackable = ClickedInventoryItem->IsStackable();
	return bIsSameItem && bIsStackable && HoverItem->GetItemType().MatchesTagExact(ClickedInventoryItem->GetItemManifest().GetItemType());
}

void UInv_MinimalInventoryGrid::SwapWithHoverItem(UInv_InventoryItem* ClickedInventoryItem, int32 GridIndex)
{
	if (!IsValid(HoverItem)) return;
	UInv_InventoryItem* TempItem = HoverItem->GetInventoryItem();
	const int32 TempStack = HoverItem->GetStackCount();
	const bool bTempStackable = HoverItem->IsStackable();
	AssignHoverItem(ClickedInventoryItem);
	HoverItem->SetPreviousIndex(HoverItem->GetPreviousGridIndex());
	RemoveItemFromGrid(ClickedInventoryItem, GridIndex);
	AddItemAtIndex(TempItem, HoveredIndex, bTempStackable, TempStack);
	UpdateGridSlots(TempItem, HoveredIndex, bTempStackable, TempStack);
}

bool UInv_MinimalInventoryGrid::ShouldSwapStackCounts(int32 RoomInClickedSlot, int32 HoveredStackCount, int32 MaxStackSize) const
{
	return RoomInClickedSlot == 0 && HoveredStackCount < MaxStackSize;
}

void UInv_MinimalInventoryGrid::SwapStackCounts(int32 ClickedStackCount, int32 HoveredStackCount, int32 Index)
{
 UInv_GridSlot* GridSlotLocal = GridSlots[Index];
	GridSlotLocal->SetStackCount(HoveredStackCount);
	UInv_SlottedItem* ClickedSlottedItem = SlottedItems.FindChecked(Index);
	ClickedSlottedItem->UpdateStackCount(HoveredStackCount);
	HoverItem->UpdateStackCount(ClickedStackCount);
}

bool UInv_MinimalInventoryGrid::ShouldConsumeHoverItemStacks(int32 HoveredStackCount, int32 RoomInClickedSlot) const
{
	return RoomInClickedSlot >= HoveredStackCount;
}

void UInv_MinimalInventoryGrid::ConsumeHoverItemStacks(int32 ClickedStackCount, int32 HoveredStackCount, int32 Index)
{
	const int32 NewCount = ClickedStackCount + HoveredStackCount;
	GridSlots[Index]->SetStackCount(NewCount);
	SlottedItems.FindChecked(Index)->UpdateStackCount(NewCount);
	ClearHoverItem();
	ShowCursor();
	HighlightIndex(Index);
}

bool UInv_MinimalInventoryGrid::ShouldFillInStack(int32 RoomInClickedSlot, int32 HoveredStackCount) const
{
	return RoomInClickedSlot < HoveredStackCount;
}

void UInv_MinimalInventoryGrid::FillInStack(int32 FillAmount, int32 Remainder, int32 Index)
{
	UInv_GridSlot* GridSlotLocal = GridSlots[Index];
	const int32 NewCount = GridSlotLocal->GetStackCount() + FillAmount;
	GridSlotLocal->SetStackCount(NewCount);
	UInv_SlottedItem* ClickedSlottedItem = SlottedItems.FindChecked(Index);
	ClickedSlottedItem->UpdateStackCount(NewCount);
	HoverItem->UpdateStackCount(Remainder);
}

void UInv_MinimalInventoryGrid::OnGridSlotClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	UInv_InventoryStatics::ItemUnhovered(GetOwningPlayer());
	if (!HasHoverItem()) return;
	if (!IsInGridBounds(HoveredIndex)) return;

	UInv_GridSlot* GridSlotLocal = GridSlots[HoveredIndex];
	if (GridSlotLocal->GetInventoryItem().IsValid())
	{
		OnSlottedItemClicked(HoveredIndex, MouseEvent);
		return;
	}
	// empty slot
	// If the hover item came from a different inventory, automatically transfer to this grid's inventory
	if (UInv_HoverItem* LocalHover = GetHoverItem())
	{
		UInv_InventoryComponent* SourceInv = LocalHover->GetSourceInventory();
		UInv_InventoryComponent* TargetInv = InventoryComponent.Get();
		if (IsValid(SourceInv) && IsValid(TargetInv) && SourceInv != TargetInv)
		{
			UInv_InventoryItem* Item = LocalHover->GetInventoryItem();
			if (IsValid(Item))
			{
				const int32 Amount = LocalHover->IsStackable() ? LocalHover->GetStackCount() : 1;
				SourceInv->Server_TransferItemToInventory(TargetInv, Item, Amount);
			}
			ClearHoverItem();
			ShowCursor();
			return;
		}
	}
	// Same-inventory put down
	PutDownOnIndex(HoveredIndex);
}

void UInv_MinimalInventoryGrid::OnSlottedItemClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	check(GridSlots.IsValidIndex(GridIndex));
	UInv_InventoryItem* ClickedItem = GridSlots[GridIndex]->GetInventoryItem().Get();
	if (!IsValid(ClickedItem)) return;

	if (!IsValid(HoverItem) && IsLeftClick(MouseEvent))
	{
		PickUp(ClickedItem, GridIndex);
		return;
	}
	if (IsRightClick(MouseEvent))
	{
		CreateItemPopUp(GridIndex);
		return;
	}
	if (IsSameStackable(ClickedItem))
	{
		const int32 ClickedCount = GridSlots[GridIndex]->GetStackCount();
		const FInv_StackableFragment* Stackable = ClickedItem->GetItemManifest().GetFragmentOfType<FInv_StackableFragment>();
		const int32 MaxStack = Stackable->GetMaxStackSize();
		const int32 Room = MaxStack - ClickedCount;
		const int32 HoverCount = HoverItem->GetStackCount();
		if (ShouldSwapStackCounts(Room, HoverCount, MaxStack))
		{
			SwapStackCounts(ClickedCount, HoverCount, GridIndex);
			return;
		}
		if (ShouldConsumeHoverItemStacks(HoverCount, Room))
		{
			ConsumeHoverItemStacks(ClickedCount, HoverCount, GridIndex);
			return;
		}
		if (ShouldFillInStack(Room, HoverCount))
		{
			FillInStack(Room, HoverCount - Room, GridIndex);
			return;
		}
		if (Room == 0) return;
	}
	// Try swap with hover item if different item
	SwapWithHoverItem(ClickedItem, GridIndex);
}

void UInv_MinimalInventoryGrid::CreateItemPopUp(int32 GridIndex)
{
	UInv_InventoryItem* RightClickedItem = GridSlots[GridIndex]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return;
	if (IsValid(GridSlots[GridIndex]->GetItemPopUp())) return;

	ItemPopUp = CreateWidget<UInv_ItemPopUp>(this, ItemPopUpClass);
	GridSlots[GridIndex]->SetItemPopUp(ItemPopUp);
	OwningCanvasPanel->AddChild(ItemPopUp);
	UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(ItemPopUp);
	const FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());
	CanvasSlot->SetPosition(MousePosition - ItemPopUpOffset);
	CanvasSlot->SetSize(ItemPopUp->GetBoxSize());

	const int32 SliderMax = GridSlots[GridIndex]->GetStackCount() - 1;
	if (RightClickedItem->IsStackable() && SliderMax > 0)
	{
		ItemPopUp->OnSplit.BindDynamic(this, &ThisClass::OnPopUpMenuSplit);
		ItemPopUp->SetSliderParams(SliderMax, FMath::Max(1, GridSlots[GridIndex]->GetStackCount() / 2));
	}
	else
	{
		ItemPopUp->CollapseSplitButton();
	}
	ItemPopUp->OnDrop.BindDynamic(this, &ThisClass::OnPopUpMenuDrop);
	if (RightClickedItem->IsConsumable())
	{
		ItemPopUp->OnConsume.BindDynamic(this, &ThisClass::OnPopUpConsume);
	}
	else
	{
		ItemPopUp->CollapseConsumeButton();
	}
}

void UInv_MinimalInventoryGrid::PutHoverItemBack()
{
	if (!IsValid(HoverItem)) return;
	FInv_SlotAvailabilityResult Result = HasRoomForItem(HoverItem->GetInventoryItem(), HoverItem->GetStackCount());
	Result.Item = HoverItem->GetInventoryItem();
	AddStacks(Result);
	ClearHoverItem();
}

void UInv_MinimalInventoryGrid::DropItem()
{
	UInv_HoverItem* LocalHover = GetHoverItem();
	if (!IsValid(LocalHover)) return;
	if (!IsValid(LocalHover->GetInventoryItem())) return;
	if (InventoryComponent.IsValid())
	{
		InventoryComponent->Server_DropItem(LocalHover->GetInventoryItem(), LocalHover->GetStackCount());
	}
	ClearHoverItem();
	ShowCursor();
}

void UInv_MinimalInventoryGrid::AddStacks(const FInv_SlotAvailabilityResult& Result)
{
	if (!MatchesCategory(Result.Item.Get())) return;
	for (const auto& S : Result.SlotAvailabilities)
	{
		if (S.bItemAtIndex)
		{
			UInv_GridSlot* GridSlot = GridSlots[S.Index];
			UInv_SlottedItem* Slotted = SlottedItems.FindChecked(S.Index);
			Slotted->UpdateStackCount(GridSlot->GetStackCount() + S.AmountToFill);
			GridSlot->SetStackCount(GridSlot->GetStackCount() + S.AmountToFill);
			continue;
		}
		AddItemAtIndex(Result.Item.Get(), S.Index, Result.bStackable, S.AmountToFill);
		UpdateGridSlots(Result.Item.Get(), S.Index, Result.bStackable, S.AmountToFill);
	}
}

void UInv_MinimalInventoryGrid::OnGridSlotHovered(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	if (IsValid(HoverItem)) return;
	if (!GridSlots.IsValidIndex(GridIndex)) return;
	UInv_GridSlot* GridSlotLocal = GridSlots[GridIndex];
	if (GridSlotLocal->IsAvailable())
	{
		GridSlotLocal->SetOccupiedTexture();
	}
}

void UInv_MinimalInventoryGrid::OnGridSlotUnhovered(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	if (IsValid(HoverItem)) return;
	if (!GridSlots.IsValidIndex(GridIndex)) return;
	UInv_GridSlot* GridSlotLocal = GridSlots[GridIndex];
	if (GridSlotLocal->IsAvailable())
	{
		GridSlotLocal->SetUnoccupiedTexture();
	}
}

void UInv_MinimalInventoryGrid::OnPopUpMenuSplit(int32 SplitAmount, int32 Index)
{
	UInv_InventoryItem* RightClickedItem = GridSlots[Index]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return;
	if (!RightClickedItem->IsStackable()) return;

	const int32 UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	UInv_GridSlot* UpperLeftGridSlot = GridSlots[UpperLeftIndex];
	const int32 StackCount = UpperLeftGridSlot->GetStackCount();
	const int32 NewStackCount = StackCount - SplitAmount;
	UpperLeftGridSlot->SetStackCount(NewStackCount);
	SlottedItems.FindChecked(UpperLeftIndex)->UpdateStackCount(NewStackCount);

	AssignHoverItem(RightClickedItem);
	HoverItem->SetPreviousIndex(UpperLeftIndex);
	HoverItem->UpdateStackCount(SplitAmount);
}

void UInv_MinimalInventoryGrid::OnPopUpMenuDrop(int32 Index)
{
	UInv_InventoryItem* RightClickedItem = GridSlots[Index]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return;
	PickUp(RightClickedItem, Index);
	DropItem();
}

void UInv_MinimalInventoryGrid::OnPopUpConsume(int32 Index)
{
	UInv_InventoryItem* RightClickedItem = GridSlots[Index]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return;

	const int32 UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	UInv_GridSlot* UpperLeftGridSlot = GridSlots[UpperLeftIndex];
	const int32 NewStackCount = UpperLeftGridSlot->GetStackCount() - 1;
	SlottedItems.FindChecked(UpperLeftIndex)->UpdateStackCount(NewStackCount);

	if (InventoryComponent.IsValid())
	{
		InventoryComponent->Server_ConsumeItem(RightClickedItem);
	}

	if (NewStackCount <= 0)
	{
		RemoveItemFromGrid(RightClickedItem, UpperLeftIndex);
	}
}

void UInv_MinimalInventoryGrid::OnInventoryMenuToggled(bool bOpen)
{
	if (!bOpen)
	{
		PutHoverItemBack();
	}
}
