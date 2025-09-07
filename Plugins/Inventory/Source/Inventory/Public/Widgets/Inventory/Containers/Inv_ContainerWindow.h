#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Inv_ContainerWindow.generated.h"

class UInv_MinimalInventoryGrid;
class UInv_InventoryComponent;
class UPanelWidget;

/**
 * Container window capable of showing the FULL player inventory (same widget you open normally)
 * or, if no host panel is provided, a minimal grid fallback. The container side uses a minimal grid.
 *
 * In your UMG Blueprint derived from this class you can:
 * - Add a panel named PlayerInventoryHost (e.g., a SizeBox/VerticalBox) and bind it to host the existing
 *   InventoryMenu widget instance from UInv_InventoryComponent, guaranteeing the same categories and slots.
 * - Optionally keep PlayerGrid for fallback if you don't add a host panel.
 * - Add a UInv_MinimalInventoryGrid named ContainerGrid for the container.
 */
UCLASS()
class INVENTORY_API UInv_ContainerWindow : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	// Call to bind the grids to provided inventory components. If PlayerInventoryHost is bound,
	// it will adopt the player's existing InventoryMenu widget instance into the container window.
	UFUNCTION(BlueprintCallable, Category = "Inventory|Container")
	void InitializeWindow(UInv_InventoryComponent* PlayerInventory, UInv_InventoryComponent* ContainerInventory);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Container")
	UInv_MinimalInventoryGrid* GetPlayerGrid() const { return PlayerGrid; }

	UFUNCTION(BlueprintCallable, Category = "Inventory|Container")
	UInv_MinimalInventoryGrid* GetContainerGrid() const { return ContainerGrid; }

protected:
	// Optional host for the full inventory widget (UInv_InventoryBase). If provided, we will reparent
	// the player's existing InventoryMenu into this panel.
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UPanelWidget> PlayerInventoryHost;

	// Fallback minimal grid for player side (used only if no host is bound)
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UInv_MinimalInventoryGrid> PlayerGrid;

	// Minimal grid for the container side
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UInv_MinimalInventoryGrid> ContainerGrid;
};