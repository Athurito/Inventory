// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Inv_ItemDescription.generated.h"

class USizeBox;
/**
 * 
 */
UCLASS()
class INVENTORY_API UInv_ItemDescription : public UCommonUserWidget
{
	GENERATED_BODY()

public:

	FVector2D GetBoxSize() const;
	
private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> SizeBox;
};
