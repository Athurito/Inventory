// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Inv_InfoMessage.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class INVENTORY_API UInv_InfoMessage : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	
	virtual void NativeOnInitialized() override;

	UFUNCTION(BLueprintImplementableEvent, Category = "Inventory")
	void ShowMessage();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void HideMessage();


	void SetMessage(const FText& Message);

private:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Message;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	float MessageLifeTime{3.f};

	FTimerHandle MessageTimer;
	bool bIsMessageActive{false};
};
