// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/Manifest/Inv_ItemManifest.h"
#include "Inv_ItemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class INVENTORY_API UInv_ItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInv_ItemComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	void InitItemManifest(FInv_ItemManifest CopyOfManifest);
	FInv_ItemManifest GetItemManifest() const { return ItemManifest; }
	FInv_ItemManifest& GetItemManifestMutable() { return ItemManifest; }

	FString GetPickupMessage() const { return PickupMessage; }

	void PickedUp();
protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void OnPickUp();

private:

	UPROPERTY(Replicated, EditAnywhere, Category = "Inventory")
	FInv_ItemManifest ItemManifest;

	// Optional: Assign a shared manifest asset instead of configuring fragments per actor
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TObjectPtr<class UInv_ItemManifestAsset> ManifestAsset = nullptr;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	FString PickupMessage;
};
