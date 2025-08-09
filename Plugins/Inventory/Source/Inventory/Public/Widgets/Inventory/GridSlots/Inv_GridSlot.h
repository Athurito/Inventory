// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Inv_GridSlot.generated.h"

class UImage;
/**
 * 
 */
UCLASS()
class INVENTORY_API UInv_GridSlot : public UCommonUserWidget
{
	GENERATED_BODY()


public:
	void SetTileIndex(int32 Index) { TileIndex = Index; };
	int32 GetTileIndex() const { return TileIndex; };

private:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_GridSlot;

	int32 TileIndex;
};
