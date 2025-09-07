// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Components/Inv_ItemComponent.h"

#include "Net/UnrealNetwork.h"
#include "Items/Manifest/Inv_ItemManifestAsset.h"


UInv_ItemComponent::UInv_ItemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PickupMessage = "E - Pickup";
	SetIsReplicatedByDefault(true);
}

void UInv_ItemComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, ItemManifest);
}

void UInv_ItemComponent::BeginPlay()
{
	Super::BeginPlay();

	// If a ManifestAsset is assigned, prefer it over per-actor fragments to simplify authoring
	#if WITH_EDITORONLY_DATA
	// No special handling needed in editor-only
	#endif

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (ManifestAsset)
		{
			// Copy the manifest from the asset so it replicates to clients via ItemManifest
			ItemManifest = ManifestAsset->ItemManifest;
		}
	}
}

void UInv_ItemComponent::InitItemManifest(FInv_ItemManifest CopyOfManifest)
{
	ItemManifest = CopyOfManifest;
}

void UInv_ItemComponent::PickedUp()
{
	OnPickUp();
	GetOwner()->Destroy();
}

