#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/Manifest/Inv_ItemManifest.h"
#include "Inv_ItemManifestAsset.generated.h"

/**
 * Primary Data Asset that holds an Item Manifest so multiple actors can share the same item configuration
 */
UCLASS(BlueprintType)
class INVENTORY_API UInv_ItemManifestAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	// Expose the manifest so it can be configured in an asset instead of per-actor fragments
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	FInv_ItemManifest ItemManifest;
};
