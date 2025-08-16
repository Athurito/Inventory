// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "Inv_HoverItem.generated.h"

/**
 * The Hover item is the item what will apear and follow the mouse when an item on the grid has been clicked
 */

class UInv_InventoryItem;
class UImage;
class UTextBlock;

UCLASS()
class INVENTORY_API UInv_HoverItem : public UCommonUserWidget
{
	GENERATED_BODY()

public:

	void SetImageBrush(const FSlateBrush& Brush) const;
	void UpdateStackCount(int32 Count) const;

	FGameplayTag GetItemType() const;
	int32 GetStackCount() const { return StackCount; };
	bool IsStackable() const { return bIsStackable; };
	void SetIsStackable(bool bStacks);
	int32 GetPreviousIndex() const { return PreviousGridIndex; };
	void SetPreviousIndex(int32 Index) { PreviousGridIndex = Index; };
	FIntPoint GetGridDimensions() const { return GridDimensions; };
	void SetGridDimensions(const FIntPoint& Dimensions) { GridDimensions = Dimensions; };
	void SetInventoryItem(UInv_InventoryItem* Item);
	UInv_InventoryItem* GetInventoryItem() const;
	
	

private:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_StackCount;

	int32 PreviousGridIndex{INDEX_NONE};
	FIntPoint GridDimensions;

	TWeakObjectPtr<UInv_InventoryItem> InventoryItem;
	bool bIsStackable{false};
	int32 StackCount{0};
	
};
