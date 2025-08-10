#include "InventoryManagement/FastArray/Inv_FastArray.h"

#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "Items/Inv_InventoryItem.h"
#include "Items/Components/Inv_ItemComponent.h"


TArray<UInv_InventoryItem*> FInv_InventoryFastArray::GetAllItems() const
{
	TArray<UInv_InventoryItem*> Results;
	Results.Reserve(Entries.Num());

	for (const auto& Entry : Entries)
	{
		if (!IsValid(Entry.Item))
			continue;
		
		Results.Add(Entry.Item);
	}
	return Results;
}

void FInv_InventoryFastArray::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	UInv_InventoryComponent* ItemComponent = Cast<UInv_InventoryComponent>(OwnerComponent);

	if (!IsValid(ItemComponent))
	{
		return;
	}

	for (int32 Index : RemovedIndices)
	{
		ItemComponent->OnItemRemoved.Broadcast(Entries[Index].Item);
	}
}

void FInv_InventoryFastArray::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	UInv_InventoryComponent* ItemComponent = Cast<UInv_InventoryComponent>(OwnerComponent);

	if (!IsValid(ItemComponent))
	{
		return;
	}

	for (int32 Index : AddedIndices)
	{
		ItemComponent->OnItemAdded.Broadcast(Entries[Index].Item);
	}
}

UInv_InventoryItem* FInv_InventoryFastArray::AddEntry(UInv_ItemComponent* ItemComponent)
{
	check(OwnerComponent)
	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority())

	UInv_InventoryComponent* InventoryComponent = Cast<UInv_InventoryComponent>(OwnerComponent);

	if (!IsValid(InventoryComponent))
	{
		return nullptr;
	}
	
	FInv_InventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Item = ItemComponent->GetItemManifest().Manifest(OwningActor);
	InventoryComponent->AddRepSubObject(NewEntry.Item);

	MarkItemDirty(NewEntry);

	return NewEntry.Item;
}

UInv_InventoryItem* FInv_InventoryFastArray::AddEntry(UInv_InventoryItem* Item)
{
	check(OwnerComponent)
	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority())

	FInv_InventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Item = Item;

	MarkItemDirty(NewEntry);

	return Item;
}

void FInv_InventoryFastArray::RemoveEntry(UInv_InventoryItem* Item)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FInv_InventoryEntry& Entry = *EntryIt;
		
		if (EntryIt->Item == Item)
		{
			EntryIt.RemoveCurrent();
			MarkArrayDirty();
		}
	}
}
