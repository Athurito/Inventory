#include "Widgets/Inventory/Containers/Inv_ContainerWindow.h"

#include "Widgets/Inventory/Spatial/Inv_MinimalInventoryGrid.h"
#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "Widgets/Inventory/InventoryBase/Inv_InventoryBase.h"
#include "Components/PanelWidget.h"

void UInv_ContainerWindow::InitializeWindow(UInv_InventoryComponent* PlayerInventory, UInv_InventoryComponent* ContainerInventory)
{
	// Player side: Prefer hosting the actual full inventory widget if a host panel is bound
	if (IsValid(PlayerInventory) && IsValid(PlayerInventoryHost))
	{
		if (UInv_InventoryBase* FullInventory = PlayerInventory->GetInventoryMenu())
		{
			if (IsValid(FullInventory))
			{
				FullInventory->RemoveFromParent();
				PlayerInventoryHost->AddChild(FullInventory);
				// Ensure the full inventory is interactive inside the container window
				FullInventory->SetVisibility(ESlateVisibility::Visible);
			}
		}
	}
	else if (IsValid(PlayerGrid) && IsValid(PlayerInventory))
	{
		// Fallback to minimal grid
		PlayerGrid->SetInventoryComponent(PlayerInventory);
	}

	// Container side: minimal grid bound to container inventory
	if (IsValid(ContainerGrid) && IsValid(ContainerInventory))
	{
		ContainerGrid->SetInventoryComponent(ContainerInventory);
	}
}