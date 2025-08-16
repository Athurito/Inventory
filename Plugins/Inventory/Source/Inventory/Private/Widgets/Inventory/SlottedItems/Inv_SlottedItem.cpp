// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Inventory/SlottedItems/Inv_SlottedItem.h"
#include "Items/Inv_InventoryItem.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

FReply UInv_SlottedItem::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	OnSlottedItemClicked.Broadcast(GridIndex, InMouseEvent);
	return FReply::Handled();
}

void UInv_SlottedItem::SetInventoryItem(UInv_InventoryItem* Item)
{
	InventoryItem = Item;
}

void UInv_SlottedItem::SetImageBrush(const FSlateBrush& Brush) const
{
	Image_Icon->SetBrush(Brush);
}

void UInv_SlottedItem::UpdateStackCount(int32 StackCount) const
{
	if (StackCount > 0)
	{
		Text_StackCount->SetText(FText::AsNumber(StackCount));
		Text_StackCount->SetVisibility(ESlateVisibility::Visible);
		return;
	}
	
	Text_StackCount->SetVisibility(ESlateVisibility::Collapsed);
}
