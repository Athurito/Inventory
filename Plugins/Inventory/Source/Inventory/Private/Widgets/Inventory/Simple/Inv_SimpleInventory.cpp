#include "Widgets/Inventory/Simple/Inv_SimpleInventory.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Items/Fragments/Inv_ItemFragment.h"
#include "Items/Inv_InventoryItem.h"
#include "Widgets/Inventory/GridSlots/Inv_GridSlot.h"
#include "Widgets/Inventory/SlottedItems/Inv_SlottedItem.h"
#include "Widgets/Utils/Inv_WidgetUtils.h"

void UInv_SimpleInventory::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ConstructGrid();

	InventoryComponent = UInv_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
	if (InventoryComponent.IsValid())
	{
		InventoryComponent->OnItemAdded.AddDynamic(this, &ThisClass::OnItemAdded);
	}
}

FInv_SlotAvailabilityResult UInv_SimpleInventory::HasRoomForItem(UInv_ItemComponent* ItemComponent) const
{
	FInv_SlotAvailabilityResult Result;
	if (!IsValid(ItemComponent)) return Result;

	// Stack info
	const FInv_StackableFragment* StackableFragment = ItemComponent->GetItemManifest().GetFragmentOfType<FInv_StackableFragment>();
	Result.bStackable = StackableFragment != nullptr;
	const int32 MaxStackSize = StackableFragment ? StackableFragment->GetMaxStackSize() : 1;
	int32 AmountToFill = StackableFragment ? StackableFragment->GetStackCount() : 1;

	// First try to fill existing stack (same type) if any
	const int32 ExistingStackIndex = FindExistingStackIndex(ItemComponent->GetItemManifest().GetItemType(), MaxStackSize);
	if (ExistingStackIndex != INDEX_NONE)
	{
		// How much room in the existing stack
		const int32 CurrentCount = GridSlots[ExistingStackIndex]->GetStackCount();
		const int32 RoomInStack = FMath::Max(0, MaxStackSize - CurrentCount);
		const int32 FillHere = FMath::Min(AmountToFill, RoomInStack);
		if (FillHere > 0)
		{
			Result.TotalRoomToFill += FillHere;
			Result.SlotAvailabilities.Emplace(FInv_SlotAvailability{ExistingStackIndex, FillHere, true});
			AmountToFill -= FillHere;
			Result.Remainder = AmountToFill;
			if (AmountToFill == 0) return Result;
		}
	}

	// Then find empty slots and place one item/stack per slot
	while (AmountToFill > 0)
	{
		const int32 EmptyIndex = FindFirstEmptySlotIndex();
		if (EmptyIndex == INDEX_NONE)
		{
			// no more room
			break;
		}

		const int32 FillHere = Result.bStackable ? FMath::Min(MaxStackSize, AmountToFill) : 1;
		Result.TotalRoomToFill += FillHere;
		Result.SlotAvailabilities.Emplace(FInv_SlotAvailability{EmptyIndex, Result.bStackable ? FillHere : 0, false});
		AmountToFill -= FillHere;
		Result.Remainder = AmountToFill;
	}

	return Result;
}

void UInv_SimpleInventory::OnItemAdded(UInv_InventoryItem* Item)
{
	if (!IsValid(Item)) return;

	// If stackable and there is an existing stack, increase slot stack count
	const FInv_StackableFragment* Stackable = Item->GetItemManifest().GetFragmentOfType<FInv_StackableFragment>();
	const bool bStackable = Stackable != nullptr;
	const int32 MaxStack = Stackable ? Stackable->GetMaxStackSize() : 1;
	const FGameplayTag ItemType = Item->GetItemManifest().GetItemType();

	int32 TargetIndex = INDEX_NONE;
	if (bStackable)
	{
		TargetIndex = FindExistingStackIndex(ItemType, MaxStack);
	}

	if (TargetIndex == INDEX_NONE)
	{
		TargetIndex = FindFirstEmptySlotIndex();
	}

	// Place visually (and record grid state)
	const int32 StackAmount = bStackable ? Stackable->GetStackCount() : 0;
	PlaceItemInSlot(Item, TargetIndex, StackAmount);
}

int32 UInv_SimpleInventory::FindFirstEmptySlotIndex() const
{
	for (UInv_GridSlot* GridCell : GridSlots)
	{
		if (!IsValid(GridCell)) continue;
		if (!GridCell->GetInventoryItem().IsValid())
		{
			return GridCell->GetTileIndex();
		}
	}
	return INDEX_NONE;
}

int32 UInv_SimpleInventory::FindExistingStackIndex(const FGameplayTag& ItemType, int32 MaxStackSize) const
{
	for (UInv_GridSlot* GridCell : GridSlots)
	{
		if (!IsValid(GridCell)) continue;
		if (!GridCell->GetInventoryItem().IsValid()) continue;
		const UInv_InventoryItem* Existing = GridCell->GetInventoryItem().Get();
		if (!Existing || !Existing->IsStackable()) continue;
		if (!Existing->GetItemManifest().GetItemType().MatchesTagExact(ItemType)) continue;
		if (GridCell->GetStackCount() >= MaxStackSize) continue;
		return GridCell->GetTileIndex();
	}
	return INDEX_NONE;
}

void UInv_SimpleInventory::PlaceItemInSlot(UInv_InventoryItem* Item, int32 SlotIndex, int32 StackAmount)
{
	if (!GridSlots.IsValidIndex(SlotIndex)) return;
	UInv_GridSlot* GridSlot = GridSlots[SlotIndex];

	GridSlot->SetInventoryItem(Item);
	GridSlot->SetUpperLeftIndex(SlotIndex);
	GridSlot->SetOccupiedTexture();
	GridSlot->SetAvailable(false);
	if (StackAmount > 0)
	{
		GridSlot->SetStackCount(StackAmount);
	}

	// Visual widget
	if (!SlottedItemClass) return;
	const FInv_ImageFragment* ImageFragment = Item->GetItemManifest().GetFragmentOfType<FInv_ImageFragment>();
	if (!ImageFragment || !CanvasPanel) return;

	UInv_SlottedItem* Slotted = CreateWidget<UInv_SlottedItem>(GetOwningPlayer(), SlottedItemClass);
	Slotted->SetInventoryItem(Item);
	Slotted->SetGridIndex(SlotIndex);
	Slotted->SetIsStackable(StackAmount > 0);
	Slotted->UpdateStackCount(StackAmount);

	// Set the icon texture if we have one
	if (ImageFragment && ImageFragment->GetIcon())
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(ImageFragment->GetIcon());
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageSize = FVector2D(TileSize, TileSize);
		Slotted->SetImageBrush(Brush);
	}

	CanvasPanel->AddChild(Slotted);
	if (UCanvasPanelSlot* CPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(Slotted))
	{
		CPS->SetSize(FVector2D(TileSize));
		const FVector2D DrawPos = UInv_WidgetUtils::GetPositionFromIndex(SlotIndex, Columns) * TileSize;
		CPS->SetPosition(DrawPos);
	}
	SlottedItems.Add(SlotIndex, Slotted);
}

void UInv_SimpleInventory::ConstructGrid()
{
	GridSlots.Reset();
	GridSlots.Reserve(Rows * Columns);
	if (!CanvasPanel || !GridSlotClass) return;

	for (int32 r = 0; r < Rows; ++r)
	{
		for (int32 c = 0; c < Columns; ++c)
		{
			UInv_GridSlot* GridSlot = CreateWidget<UInv_GridSlot>(this, GridSlotClass);
			CanvasPanel->AddChild(GridSlot);

			const int32 TileIndex = UInv_WidgetUtils::GetIndexFromPosition(FIntPoint(c, r), Columns);
			GridSlot->SetTileIndex(TileIndex);

			if (UCanvasPanelSlot* CPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(GridSlot))
			{
				CPS->SetSize(FVector2D(TileSize));
				CPS->SetPosition(FVector2D(c * TileSize, r * TileSize));
			}

			GridSlots.Add(GridSlot);
		}
	}
}
