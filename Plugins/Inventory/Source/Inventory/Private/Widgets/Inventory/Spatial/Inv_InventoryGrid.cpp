// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Inventory/Spatial/Inv_InventoryGrid.h"

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
#include "Widgets/Utils/Inv_WidgetUtils.h"

void UInv_InventoryGrid::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ConstructGrid();

	InventoryComponent = UInv_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
	InventoryComponent->OnItemAdded.AddDynamic(this, &ThisClass::AddItem);
	InventoryComponent->OnStackChange.AddDynamic(this, &ThisClass::AddStacks);
	
}

FInv_SlotAvailabilityResult UInv_InventoryGrid::HasRoomForItem(const UInv_ItemComponent* ItemComponent)
{
	return HasRoomForItem(ItemComponent->GetItemManifest());
}

FInv_SlotAvailabilityResult UInv_InventoryGrid::HasRoomForItem(const UInv_InventoryItem* InventoryItem)
{
	return HasRoomForItem(InventoryItem->GetItemManifest());
}

FInv_SlotAvailabilityResult UInv_InventoryGrid::HasRoomForItem(const FInv_ItemManifest& Manifest)
{
	FInv_SlotAvailabilityResult Result;
	
	// Determine if the item is stackable.
	const FInv_StackableFragment* StackableFragment = Manifest.GetFragmentOfType<FInv_StackableFragment>();
	Result.bStackable = StackableFragment != nullptr;
	
	// Determine how many stacks to add.
	const int32 MaxStackSize = StackableFragment ? StackableFragment->GetMaxStackSize() : 1;
	int32 AmountToFill = StackableFragment ? StackableFragment->GetStackCount() : 1;


	TSet<int32> ClaimedIndices;
	// For each Grid Slot:

	FIntPoint Dimensions = GetItemDimensions(Manifest);
	for (const auto& GridSlot : GridSlots)
	{
		// If we don't have anymore to fill, break out of the loop early.
		if (AmountToFill == 0) break;
		// Is this index claimed yet?
		if (IsIndexClaimed(ClaimedIndices, GridSlot->GetTileIndex())) continue;

		// Is the Item in grid bounds
		if (!IsInGridBounds(GridSlot->GetTileIndex(), Dimensions)) continue;

		
		// Can the item fit here? (i.e. is it out of grid bounds?)
		TSet<int32> TentativelyClaimed;
		if (!HasRoomAtIndex(GridSlot, Dimensions, ClaimedIndices, TentativelyClaimed, Manifest.GetItemType(), MaxStackSize))
		{
			continue;
		}
		
		// How much to fill?
		const int32 AmountToFillInSlot = DetermineFillAmountForSlot(Result.bStackable, MaxStackSize, AmountToFill, GridSlot);
		if (AmountToFillInSlot == 0) continue;
		
		ClaimedIndices.Append(TentativelyClaimed);
		
		// Update the amount left to fill
		Result.TotalRoomToFill += AmountToFillInSlot;
		Result.SlotAvailabilities.Emplace(
			FInv_SlotAvailability
			{
				HasValidItem(GridSlot) ? GridSlot->GetUpperLeftIndex() : GridSlot->GetTileIndex(),
				Result.bStackable ? AmountToFillInSlot : 0,
				HasValidItem(GridSlot)
			}
		);

		AmountToFill -= AmountToFillInSlot;

		// How much is the Remainder?
		Result.Remainder = AmountToFill;

		if (AmountToFill == 0) return Result;
	}

	
	return Result;
}

bool UInv_InventoryGrid::HasRoomAtIndex(const UInv_GridSlot* GridSlot, const FIntPoint& Dimensions, const TSet<int32>& ClaimedIndices, TSet<int32>& OutTentativelyClaimed, const FGameplayTag& ItemType, const int32 MaxStackSize)
{
	// Is there room at this index? (i.e. are there other items in the way?)
	bool bHasRoomAtIndex = true;
	UInv_InventoryStatics::ForEach2D(GridSlots, GridSlot->GetTileIndex(), Dimensions, Columns, [&](const UInv_GridSlot* SubGridSlot)
	{
		if (CheckSlotConstraints(GridSlot, SubGridSlot, ClaimedIndices, OutTentativelyClaimed, ItemType, MaxStackSize))
		{
			OutTentativelyClaimed.Add(SubGridSlot->GetTileIndex());
		}
		else
		{
			bHasRoomAtIndex = false;
		}
	});
	return bHasRoomAtIndex;
}

FIntPoint UInv_InventoryGrid::GetItemDimensions(const FInv_ItemManifest& Manifest) const
{
	const FInv_GridFragment* GridFragment = Manifest.GetFragmentOfType<FInv_GridFragment>();
	const FIntPoint Dimensions = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
	return Dimensions;
}

bool UInv_InventoryGrid::CheckSlotConstraints(const UInv_GridSlot* GridSlot, const UInv_GridSlot* SubGridSlot, const TSet<int32>& ClaimedIndices, TSet<int32>& OutTentativelyClaimed, const FGameplayTag& ItemType, const int32 MaxStackSize) const
{

	// Index claimed?
	if (IsIndexClaimed(ClaimedIndices, SubGridSlot->GetTileIndex()))
	{
		return false;
	}
	// Has valid item?
	if (!HasValidItem(SubGridSlot))
	{
		OutTentativelyClaimed.Add(SubGridSlot->GetTileIndex());
		return true;
	}
	// Is this Grid Slot an upper left slot
	if (!IsUpperLeftSLot(GridSlot, SubGridSlot)) return false;
	
	// If so, is this a stackable item?
	const UInv_InventoryItem* SubItem = SubGridSlot->GetInventoryItem().Get();
	if (!SubItem->IsStackable()) return false;
	
	// Is this item the same type as the item we're trying to add?
	if (!DoesItemTypeMatch(SubItem, ItemType)) return false;
	
	// If stackable, is this slot at the max stack size already?
	if (GridSlot->GetStackCount() >= MaxStackSize) return false;
	
	return true;
}

bool UInv_InventoryGrid::HasValidItem(const UInv_GridSlot* GridSlot) const
{
	return GridSlot->GetInventoryItem().IsValid();
}

bool UInv_InventoryGrid::IsUpperLeftSLot(const UInv_GridSlot* GridSlot, const UInv_GridSlot* SubGridSlot) const
{
	return SubGridSlot->GetUpperLeftIndex() == GridSlot->GetTileIndex();
}

bool UInv_InventoryGrid::DoesItemTypeMatch(const UInv_InventoryItem* SubItem, const FGameplayTag& ItemType) const
{
	return SubItem->GetItemManifest().GetItemType().MatchesTagExact(ItemType);
}

bool UInv_InventoryGrid::IsInGridBounds(const int32 StartIndex, const FIntPoint& ItemDimensions) const
{
	if (StartIndex < 0|| StartIndex >= GridSlots.Num()) return false;

	const int32 EndColumn = (StartIndex % Columns) + ItemDimensions.X;
	const int32 EndRow = (StartIndex / Columns) + ItemDimensions.Y;

	return EndColumn <= Columns && EndRow <= Rows;
	
}

int32 UInv_InventoryGrid::DetermineFillAmountForSlot(const bool bStackable, const int32 MaxStackSize,
	const int32 AmountToFill, const UInv_GridSlot* GridSlot) const
{
	//calculate Room in Slot
	const int32 RoomInSlot = MaxStackSize - GetStackAmount(GridSlot);
	return bStackable ? FMath::Min(AmountToFill, RoomInSlot) : 1;
	//If stackable, need the minimum between AmountToFill and RoomInSlot
}

int32 UInv_InventoryGrid::GetStackAmount(const UInv_GridSlot* GridSlot) const
{
	int32 CurrentSlotStackAmount = GridSlot->GetStackCount();
	// if we are at a slot that diesnt holt the stack coint we must get the actual stack count.
	if (const int32 UpperLeftIndex = GridSlot->GetUpperLeftIndex(); UpperLeftIndex != INDEX_NONE)
	{
		UInv_GridSlot *UpperLeftSlot = GridSlots[UpperLeftIndex];
		CurrentSlotStackAmount = UpperLeftSlot->GetStackCount();
	}
	return CurrentSlotStackAmount;
}

bool UInv_InventoryGrid::IsRightClick(const FPointerEvent& MouseEvent) const
{
	return MouseEvent.GetEffectingButton() == EKeys::RightMouseButton;
}

bool UInv_InventoryGrid::IsLeftClick(const FPointerEvent& MouseEvent) const
{
	return MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;
}

void UInv_InventoryGrid::AddStacks(const FInv_SlotAvailabilityResult& Result)
{
	if (!MatchesCategory(Result.Item.Get())) return;

	for (const auto& SlotAvailability : Result.SlotAvailabilities)
	{
		if (SlotAvailability.bItemAtIndex)
		{
			const auto& GridSlot = GridSlots[SlotAvailability.Index];
			const auto& SlottedItem = SlottedItems.FindChecked(SlotAvailability.Index);
			SlottedItem->UpdateStackCount(GridSlot->GetStackCount() + SlotAvailability.AmountToFill);
			GridSlot->SetStackCount(GridSlot->GetStackCount() + SlotAvailability.AmountToFill);
			continue;
		}
		
		AddItemAtIndex(Result.Item.Get(), SlotAvailability.Index, Result.bStackable, SlotAvailability.AmountToFill);
	}
}

void UInv_InventoryGrid::OnSlottedItemClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	check(GridSlots.IsValidIndex(GridIndex));
	UInv_InventoryItem* ClickedInventoryItem = GridSlots[GridIndex]->GetInventoryItem().Get();

	if (!IsValid(HoverItem))
	{
		// Todo Pickup - Assign the Hoveritme and remove the slotted item from the grid
	}
}

void UInv_InventoryGrid::AddItem(UInv_InventoryItem* Item)
{
	if (!MatchesCategory(Item)) return;
	
	FInv_SlotAvailabilityResult Result = HasRoomForItem(Item);

	AddItemToIndices(Result, Item);
}

void UInv_InventoryGrid::AddItemToIndices(const FInv_SlotAvailabilityResult& Result, UInv_InventoryItem* Item)
{

	for (const auto& SlotAvailability : Result.SlotAvailabilities)
	{
		AddItemAtIndex(Item, SlotAvailability.Index, Result.bStackable, SlotAvailability.AmountToFill);
		UpdateGridSlots(Item, SlotAvailability.Index, Result.bStackable, SlotAvailability.AmountToFill);
	}
}

void UInv_InventoryGrid::AddItemAtIndex(UInv_InventoryItem* Item, const int32 Index, const bool bStackable,
	const int32 StackAmount)
{
	const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(Item, FragmentTags::GridFragment);
	const FInv_ImageFragment* ImageFragment = GetFragment<FInv_ImageFragment>(Item, FragmentTags::ImageFragment);

	if (!GridFragment || !ImageFragment) return;

	UInv_SlottedItem* SlottedItem = CreateSlottedItem(Item, bStackable, StackAmount, GridFragment, ImageFragment, Index);

	AddSlottedItemToCanvas(Index, GridFragment, SlottedItem);

	SlottedItems.Add(Index, SlottedItem);
}

UInv_SlottedItem* UInv_InventoryGrid::CreateSlottedItem(UInv_InventoryItem* Item, const bool bStackable,
	const int32 StackAmount, const FInv_GridFragment* GridFragment, const FInv_ImageFragment* ImageFragment,
	const int32 Index)
{
	UInv_SlottedItem* SlottedItem = CreateWidget<UInv_SlottedItem>(GetOwningPlayer(), SlottedItemClass);
	
	SlottedItem->SetInventoryItem(Item);
	SetSlottedItemImage(SlottedItem, GridFragment, ImageFragment);
	SlottedItem->SetGridIndex(Index);
	SlottedItem->SetIsStackable(bStackable);
	const int32 StackUpdateAmount = bStackable ? StackAmount : 0;
	SlottedItem->UpdateStackCount(StackUpdateAmount);

	SlottedItem->OnSlottedItemClicked.AddDynamic(this, &ThisClass::OnSlottedItemClicked);
	
	return SlottedItem;
}

void UInv_InventoryGrid::AddSlottedItemToCanvas(const int32 Index, const FInv_GridFragment* GridFragment,
	UInv_SlottedItem* SlottedItem) const
{
	CanvasPanel->AddChild(SlottedItem);
	UCanvasPanelSlot* GridCanvasPanelSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(SlottedItem);
	GridCanvasPanelSlot->SetSize(GetDrawSize(GridFragment));

	const FVector2D DrawPos = UInv_WidgetUtils::GetPositionFromIndex(Index, Columns) * TileSize;
	const FVector2D DrawPosWithPadding = DrawPos + FVector2D(GridFragment->GetGridPadding());
	GridCanvasPanelSlot->SetPosition(DrawPosWithPadding);
}

void UInv_InventoryGrid::UpdateGridSlots(UInv_InventoryItem* Item, const int32 Index, const bool bStackableItem, const int32 StackAmount)
{
	check(GridSlots.IsValidIndex(Index));

	if (bStackableItem)
	{
		GridSlots[Index]->SetStackCount(StackAmount);
	}
	
	const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(Item, FragmentTags::GridFragment);
	const FIntPoint Dimensions = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
	

	UInv_InventoryStatics::ForEach2D(GridSlots, Index,Dimensions, Columns, [&](UInv_GridSlot* GridSlot)
	{
		GridSlot->SetInventoryItem(Item);
		GridSlot->SetUpperLeftIndex(Index);
		GridSlot->SetOccupiedTexture();
		GridSlot->SetAvailable(false);
	});
}

bool UInv_InventoryGrid::IsIndexClaimed(const TSet<int32>& ClaimedIndices, const int32 Index) const
{
	return ClaimedIndices.Contains(Index);
}



FVector2D UInv_InventoryGrid::GetDrawSize(const FInv_GridFragment* GridFragment) const
{
	const float IconTileWidth = TileSize - GridFragment->GetGridPadding() * 2;
	FVector2D IconSize = GridFragment->GetGridSize() * IconTileWidth;
	return IconSize;
}

void UInv_InventoryGrid::SetSlottedItemImage(const UInv_SlottedItem* SlottedItem, const FInv_GridFragment* GridFragment,
	const FInv_ImageFragment* ImageFragment) const
{
	FSlateBrush Brush;
	Brush.SetResourceObject(ImageFragment->GetIcon());
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = GetDrawSize(GridFragment);

	SlottedItem->SetImageBrush(Brush);
}



void UInv_InventoryGrid::ConstructGrid()
{
	GridSlots.Reserve(Rows * Columns);
	
	for (int32 i = 0; i < Rows; i++)
	{
		for (int32 j = 0; j < Columns; j++)
		{
			UInv_GridSlot* GridSlot = CreateWidget<UInv_GridSlot>(this, GridSlotClass);
			
			CanvasPanel->AddChild(GridSlot);

			const FIntPoint TilePosition(j, i);
			GridSlot->SetTileIndex(UInv_WidgetUtils::GetIndexFromPosition(TilePosition, Columns));

			UCanvasPanelSlot* GridCanvasPanelSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(GridSlot);

			GridCanvasPanelSlot->SetSize(FVector2D(TileSize));
			GridCanvasPanelSlot->SetPosition(TilePosition * TileSize);

			GridSlots.Add(GridSlot);
		}
	}
}



bool UInv_InventoryGrid::MatchesCategory(const UInv_InventoryItem* Item) const
{
	return Item->GetItemManifest().GetItemCategory() == ItemCategory;
}


