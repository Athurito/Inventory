// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Inv_GridSlot.generated.h"

class UInv_InventoryItem;
class UImage;

UENUM(BlueprintType)
enum class EInv_GridSlotState : uint8
{
	Unoccupied,
	Occupied,
	Selected,
	GrayedOut
};

UCLASS()
class INVENTORY_API UInv_GridSlot : public UCommonUserWidget
{
	GENERATED_BODY()


public:
	void SetTileIndex(int32 Index) { TileIndex = Index; };
	int32 GetTileIndex() const { return TileIndex; };
	
	EInv_GridSlotState GetGridSlotState() const { return GridSlotState; };
	void SetGridSlotState(EInv_GridSlotState State) { GridSlotState = State; };
	
	void SetInventoryItem(UInv_InventoryItem* Item);
	TWeakObjectPtr<UInv_InventoryItem> GetInventoryItem() const { return InventoryItem; };
	
	void SetUpperLeftIndex(int32 Index) { UpperLeftIndex = Index; };
	int32 GetUpperLeftIndex() const { return UpperLeftIndex; };
	
	void SetStackCount(int32 Count) { StackCount = Count; };
	int32 GetStackCount() const { return StackCount; };

	void SetAvailable(bool bIsAvailable) { bAvailable = bIsAvailable; };
	bool IsAvailable() const { return bAvailable; };

	void SetOccupiedTexture();
	void SetUnoccupiedTexture();
	void SetSelectedTexture();
	void SetGrayedOutTexture();

private:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_GridSlot;

	int32 TileIndex;
	int32 StackCount;
	int32 UpperLeftIndex{INDEX_NONE};
	TWeakObjectPtr<UInv_InventoryItem> InventoryItem;
	bool bAvailable{true};

	UPROPERTY(EditAnywhere, Category = "Inventory")
	FSlateBrush Brush_Unoccupied;
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FSlateBrush Brush_Occupied;
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FSlateBrush Brush_Selected;
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FSlateBrush Brush_GrayedOut;

	EInv_GridSlotState GridSlotState;
};
