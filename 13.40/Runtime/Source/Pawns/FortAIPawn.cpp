#include "pch.h"
#include "Core.h"
#include "Pawns/FortAIPawn.h"
DEFINE_ENGINE_INIT(FortAIPawn)

void FortAIPawn::DropAllItems(AFortAIPawn* _this, bool bSpawnPickups)
{
	AFortInventory* Inventory = _this->Inventory;

	if (_this->Role != ENetRole::ROLE_Authority || Inventory == NULL || _this->bHasDroppedLoot)
		return;

	_this->bHasDroppedLoot = true;

	const FVector DropLocation = _this->K2_GetActorLocation() + FVector{ 0.f, 0.f, 50.f };
	TArray<UFortWorldItem*> Items = Inventory->GetItems();

	for (UFortWorldItem* Item : Items)
	{
		if (Item == NULL)
			continue;

		if (bSpawnPickups && Item->ItemEntry.Count > 0)
		{
			FFortCreatePickupData CreatePickupData{};
			CreatePickupData.World = GWorld;
			CreatePickupData.ItemEntry = Item->ItemEntry;
			CreatePickupData.ItemEntry.SetItemGuid(NewGuid());
			CreatePickupData.SpawnLocation = DropLocation;
			CreatePickupData.PickupSourceTypeFlags = EFortPickupSourceTypeFlag::AI;

			if (AFortPickup* Pickup = AFortPickup::CreateFromData(CreatePickupData))
			{
				Pickup->PawnWhoDroppedPickup = _this;
				Pickup->TossPickup(DropLocation, _this, -1, true, true, EFortPickupSourceTypeFlag::AI, EFortPickupSpawnSource::Unset);
			}
		}

		Inventory->RemoveInventoryItem(Item->ItemEntry.ItemGuid, -1, true);
	}
}

void FortAIPawn::HandleDeath(AFortAIPawn* _this, float KillingDamage, const FGameplayTagContainer& GameplayTags, const FGameplayEffectContextHandle& EffectContext, AController* EventInstigator, AActor* DamageCauser)
{
	static FName SilentDeathTag = FName(L"Gameplay.Effect.InstantDeath.Silent");

	bool bSilentDeath = false;

	for (const FGameplayTag& Tag : GameplayTags.GameplayTags)
		bSilentDeath |= Tag.TagName == SilentDeathTag;

	if (_this->bHasInventory && _this->bShouldTossInventoryOnDeath && !bSilentDeath)
		DropAllItems(_this, true);

	HandleDeathOG(_this, KillingDamage, GameplayTags, EffectContext, EventInstigator, DamageCauser);
}

void FortAIPawn::execDropAllItems(UObject* Context, FFrame& Stack, void* const Result)
{
	bool bSpawnPickups = Stack.StepCompiledInRef<bool>();
	Stack.IncrementCode();

	DropAllItems(static_cast<AFortAIPawn*>(Context), bSpawnPickups);
}

void FortAIPawn::Init()
{
	Hooking::Hook<AFortAIPawn>(0xA90 / 8, HandleDeath, HandleDeathOG);
	Hooking::ExecHook(AFortAIPawn::StaticClass()->GetFunction("DropAllItems"), execDropAllItems);
}
