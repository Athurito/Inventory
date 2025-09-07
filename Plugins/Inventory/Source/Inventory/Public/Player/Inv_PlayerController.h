// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Inv_PlayerController.generated.h"

class UInv_InventoryComponent;
class UInv_HudWidget;
class UInputAction;
class UInputMappingContext;
/**
 * 
 */
UCLASS()
class INVENTORY_API AInv_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AInv_PlayerController();
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ToggleInventory();
	
protected:
	virtual void BeginPlay() override;

	/** Input Mapping Contexts */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

private:

	void PrimaryInteract();
	void CreateHudWidget();
	void TraceForItem();
	void TraceForContainer();

	TWeakObjectPtr<UInv_InventoryComponent> InventoryComponent;
	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TObjectPtr<UInputAction> PrimaryInteractAction;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TObjectPtr<UInputAction> ToggleInventoryAction;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<UInv_HudWidget> HudWidgetClass;

	UPROPERTY()
	TObjectPtr<UInv_HudWidget> HudWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	double TraceLength;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TEnumAsByte<ECollisionChannel> ItemTraceChannel;

	// Separate trace channel for containers (to trigger alternate behavior like opening widgets)
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TEnumAsByte<ECollisionChannel> ContainerTraceChannel;
	
	TWeakObjectPtr<AActor> ThisItemActor;
	TWeakObjectPtr<AActor> LastItemActor;

	TWeakObjectPtr<AActor> ThisContainerActor;
	TWeakObjectPtr<AActor> LastContainerActor;

protected:
	// Blueprint hooks so designers can react to container trace events (open/close container UI)
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Trace")
	void OnContainerTraceHit(AActor* HitContainerActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Trace")
	void OnContainerTraceLost(AActor* PreviousContainerActor);

	// Called when interacting while aiming at a container (different flow than picking up an item)
	UFUNCTION(BlueprintNativeEvent, Category = "Inventory|Trace")
	void OnContainerInteract(AActor* ContainerActor);

	// Default UI for container interaction
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void CloseContainerWindow();

protected:
	// Class to spawn when interacting with a container. Create a UMG BP deriving from UInv_ContainerWindow and assign here.
	UPROPERTY(EditDefaultsOnly, Category = "Inventory|UI")
	TSubclassOf<class UInv_ContainerWindow> ContainerWindowClass;

	UPROPERTY()
	TObjectPtr<class UInv_ContainerWindow> ContainerWindow;
	
};
