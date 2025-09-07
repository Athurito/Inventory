// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Inv_PlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Widgets/HUD/Inv_HudWidget.h"
#include "Inventory.h"
#include "Interaction/Inv_Highlightable.h"
#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/Inventory/Containers/Inv_ContainerWindow.h"
#include "Widgets/Inventory/InventoryBase/Inv_InventoryBase.h"

AInv_PlayerController::AInv_PlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	TraceLength = 500.f;
	ItemTraceChannel = ECC_GameTraceChannel1;
	ContainerTraceChannel = ECC_GameTraceChannel2;
}

void AInv_PlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TraceForItem();
	TraceForContainer();
}

void AInv_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	InventoryComponent = FindComponentByClass<UInv_InventoryComponent>();
	CreateHudWidget();
}

void AInv_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Add Input Mapping Contexts
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
		{
			Subsystem->AddMappingContext(CurrentContext, 0);
		}
	}

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);

	EnhancedInputComponent->BindAction(PrimaryInteractAction, ETriggerEvent::Started, this, &AInv_PlayerController::PrimaryInteract);
	EnhancedInputComponent->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &AInv_PlayerController::ToggleInventory);
	
}

void AInv_PlayerController::ToggleInventory()
{
	if (!InventoryComponent.IsValid()) return;

	InventoryComponent->ToggleInventoryMenu();

	if (InventoryComponent->IsMenuOpen())
	{
		HudWidget->SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		HudWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

	}
}

void AInv_PlayerController::OnContainerInteract_Implementation(AActor* ContainerActor)
{
	if (!IsLocalController()) return;
	if (!IsValid(ContainerActor)) return;

	// Find the container's inventory component
	UInv_InventoryComponent* ContainerInv = ContainerActor->FindComponentByClass<UInv_InventoryComponent>();
	if (!IsValid(ContainerInv) || !InventoryComponent.IsValid())
	{
		return;
	}

	if (!IsValid(ContainerWindow))
	{
		if (!ContainerWindowClass)
		{
			// No UI class set; nothing to display
			return;
		}
		ContainerWindow = CreateWidget<UInv_ContainerWindow>(this, ContainerWindowClass);
	}

	if (!IsValid(ContainerWindow)) return;
	ContainerWindow->AddToViewport();
	ContainerWindow->InitializeWindow(InventoryComponent.Get(), ContainerInv);

	// Show mouse and UI focus
	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetWidgetToFocus(ContainerWindow->TakeWidget());
	SetInputMode(InputMode);

	if (IsValid(HudWidget))
	{
		HudWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void AInv_PlayerController::CloseContainerWindow()
{
	if (IsValid(ContainerWindow))
	{
		ContainerWindow->RemoveFromParent();
	}

	// If the player's inventory widget was adopted into the container window, restore it back to viewport
	if (InventoryComponent.IsValid())
	{
		if (UInv_InventoryBase* InventoryMenu = InventoryComponent->GetInventoryMenu())
		{
			if (IsValid(InventoryMenu))
			{
				InventoryMenu->RemoveFromParent();
				InventoryMenu->AddToViewport();
				// Set visibility based on whether the inventory is currently open
				InventoryMenu->SetVisibility(InventoryComponent->IsMenuOpen() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			}
		}
	}

	// Restore HUD/Input
	if (InventoryComponent.IsValid() && !InventoryComponent->IsMenuOpen() && IsValid(HudWidget))
	{
		HudWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	bShowMouseCursor = false;
	FInputModeGameOnly GameOnly;
	SetInputMode(GameOnly);
}

void AInv_PlayerController::PrimaryInteract()
{
	// If we're aiming at a container, invoke alternate interaction (e.g., open container UI)
	if (ThisContainerActor.IsValid())
	{
		OnContainerInteract(ThisContainerActor.Get());
		return;
	}

	// Otherwise, default to picking up an item
	if (!ThisItemActor.IsValid()) return;

	UInv_ItemComponent* ItemComponent = ThisItemActor->FindComponentByClass<UInv_ItemComponent>();
	if (!IsValid(ItemComponent) || !InventoryComponent.IsValid()) return;

	InventoryComponent->TryAddItem(ItemComponent);
}

void AInv_PlayerController::CreateHudWidget()
{
	if (!IsLocalController()) return;

	HudWidget = CreateWidget<UInv_HudWidget>(this, HudWidgetClass);

	if (IsValid(HudWidget))
	{
		HudWidget->AddToViewport();
	}
}

void AInv_PlayerController::TraceForItem()
{
	if (!IsValid(GEngine) || !IsValid(GEngine->GameViewport))
	{
		return;
	}

	FVector2D ViewportSize;

	GEngine->GameViewport->GetViewportSize(ViewportSize);
	const FVector2D ViewportCenter = ViewportSize * 0.5f;
	FVector TraceStart;
	FVector Forward;
	
	if (!UGameplayStatics::DeprojectScreenToWorld(this, ViewportCenter, TraceStart, Forward))
	{
		return;
	}
	const FVector TraceEnd = TraceStart + Forward * TraceLength;

	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ItemTraceChannel);

 LastItemActor = ThisItemActor;
	ThisItemActor = HitResult.GetActor();


	if (!ThisItemActor.IsValid())
	{
		if (IsValid(HudWidget))
		{
			HudWidget->HidePickupMessage();
		}
	}
	
	if (ThisItemActor == LastItemActor)
	{
		return;
	}

	

	if (ThisItemActor.IsValid())
	{

		if (UActorComponent* Highlightable = ThisItemActor->FindComponentByInterface(UInv_Highlightable::StaticClass()); IsValid(Highlightable))
		{
			IInv_Highlightable::Execute_Highlight(Highlightable);
		}
		
		UInv_ItemComponent* ItemComponent = ThisItemActor->FindComponentByClass<UInv_ItemComponent>();
		if (!IsValid(ItemComponent)) return;

		if (IsValid(HudWidget))
		{
			HudWidget->ShowPickupMessage(ItemComponent->GetPickupMessage());
		}
	}
	if (LastItemActor.IsValid())
	{
		if (UActorComponent* Highlightable = LastItemActor->FindComponentByInterface(UInv_Highlightable::StaticClass()); IsValid(Highlightable))
		{
			IInv_Highlightable::Execute_UnHighlight(Highlightable);
		}
	}
}

void AInv_PlayerController::TraceForContainer()
{
	if (!IsValid(GEngine) || !IsValid(GEngine->GameViewport))
	{
		return;
	}

	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);
	const FVector2D ViewportCenter = ViewportSize * 0.5f;
	FVector TraceStart;
	FVector Forward;
	if (!UGameplayStatics::DeprojectScreenToWorld(this, ViewportCenter, TraceStart, Forward))
	{
		return;
	}
	const FVector TraceEnd = TraceStart + Forward * TraceLength;

	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ContainerTraceChannel);

	LastContainerActor = ThisContainerActor;
	ThisContainerActor = HitResult.GetActor();

	if (ThisContainerActor == LastContainerActor)
	{
		return;
	}

	if (ThisContainerActor.IsValid())
	{
		OnContainerTraceHit(ThisContainerActor.Get());
		if (IsValid(HudWidget))
		{
			HudWidget->ShowPickupMessage(TEXT("Open Container"));
		}
	}
	else if (LastContainerActor.IsValid())
	{
		OnContainerTraceLost(LastContainerActor.Get());
		if (IsValid(HudWidget) && !ThisItemActor.IsValid())
		{
			HudWidget->HidePickupMessage();
		}
	}
}
