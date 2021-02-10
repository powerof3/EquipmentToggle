#include "hair.h"


void Dismemberment::Update(RE::Actor* a_actor, RE::NiAVObject* a_root, RE::TESObjectARMA* a_arma, const HeadPart a_type, const bool a_hide)
{
	if (!a_actor || !a_arma || !a_root) {
		return;
	}

	const auto npc = a_actor->GetActorBase();
	if (!npc) {
		return;
	}

	const auto face = a_root->GetObjectByName(RE::FixedStrings::GetSingleton()->bsFaceGenNiNodeSkinned);
	auto race = npc->GetRace();

	if (face && race) {
		const auto invChanges = a_actor->GetInventoryChanges();
		if (invChanges) {
			const auto wornMask = GetWornMask(invChanges);
			const auto headSlot = to_underlying(*race->data.headObject);
			if (headSlot <= 31) {
				const std::uint32_t hasHead = 1 << headSlot;
				if (hasHead & wornMask) {
					face->SetAppCulled(!a_hide);
				} else {
					a_hide ? face->SetAppCulled(!a_hide) : face->SetAppCulled(a_hide);
					auto headPart = npc->GetCurrentHeadPartByType(a_type);
					if (!headPart) {
						headPart = race->GetHeadPartByType(a_type, npc->GetSex());
					}
					if (headPart) {
						if (auto node = a_root->GetObjectByName(headPart->formEditorID); node) {
							if (const auto shape = node->AsGeometry(); shape) {
								TogglePartition(shape, a_arma, a_hide);
							}
						}
						ToggleExtraParts(headPart->extraParts, a_root, a_arma, a_hide);
					}
				}
			}
		}
	}
}


void Dismemberment::UpdateHair(RE::Actor* a_actor, RE::NiAVObject* a_root, const bool a_hide)
{
	if (!a_actor || !a_root) {
		return;
	}

	const auto npc = a_actor->GetActorBase();
	if (!npc) {
		return;
	}

	const auto face = a_root->GetObjectByName(RE::FixedStrings::GetSingleton()->bsFaceGenNiNodeSkinned);
	auto race = npc->GetRace();

	if (face && race) {
		const auto invChanges = a_actor->GetInventoryChanges();
		if (invChanges) {
			const auto wornMask = GetWornMask(invChanges);
			const auto headSlot = to_underlying(*race->data.headObject);
			if (headSlot <= 31) {
				const std::uint32_t hasHead = 1 << headSlot;
				if (hasHead & wornMask) {
					face->SetAppCulled(!a_hide);
				} else {
					a_hide ? face->SetAppCulled(!a_hide) : face->SetAppCulled(a_hide);
					auto headPart = npc->GetCurrentHeadPartByType(RE::HeadPartType::kHair);
					if (!headPart) {
						headPart = race->GetHeadPartByType(RE::HeadPartType::kHair, npc->GetSex());
					}
					if (headPart) {
						if (auto node = a_root->GetObjectByName(headPart->formEditorID); node) {
							if (const auto shape = node->AsGeometry(); shape) {
								TogglePartition(shape, race, a_hide);
							}
						}
						ToggleExtraParts(headPart->extraParts, a_root, race, a_hide);
					}
				}
			}
		}
	}
}


void Dismemberment::UpdateArmor(RE::Actor* a_actor, RE::NiAVObject* a_root, std::uint32_t a_slot, bool a_hide)
{
	if (!a_actor || !a_root) {
		return;
	}

	auto bipedSlot = a_slot + 30;

	RE::BSVisit::TraverseScenegraphGeometries(a_root, [&](RE::BSGeometry* a_geometry) -> RE::BSVisit::BSVisitControl {

		const auto skinInstance = a_geometry->skinInstance.get();
		if (const auto dismemberInstance = netimmerse_cast<RE::BSDismemberSkinInstance*>(skinInstance); dismemberInstance) {
			if (dismemberInstance->partitions) {
				stl::span<RE::BSDismemberSkinInstance::Data> span(dismemberInstance->partitions, dismemberInstance->numPartitions);
				for (auto& data : span) {
					auto slot = data.slot;
					if (slot - 130 > 31) {
						if (slot - 230 <= 31) {
							slot -= 200;
						}
					} else {
						slot -= 100;
					}
					if (slot == bipedSlot) {
						dismemberInstance->UpdateDismemberPartion(data.slot, a_hide);
					}
				}
			}
		}
		
		return RE::BSVisit::BSVisitControl::kContinue;
	});
}


std::uint32_t Dismemberment::GetWornMask(RE::InventoryChanges* a_changes)
{
	using func_t = decltype(&GetWornMask);
	REL::Relocation<func_t> func{ REL::ID(15806) };
	return func(a_changes);
}


void Dismemberment::TogglePartition(RE::BSGeometry* a_shape, RE::TESObjectARMA* a_arma, const bool a_enable)
{
	const auto skinInstance = a_shape->skinInstance.get();
	if (const auto dismemberInstance = netimmerse_cast<RE::BSDismemberSkinInstance*>(skinInstance); dismemberInstance) {
		if (dismemberInstance->partitions) {
			stl::span<RE::BSDismemberSkinInstance::Data> span(dismemberInstance->partitions, dismemberInstance->numPartitions);
			for (auto& data : span) {
				auto slot = data.slot;
				if (slot - 130 > 31) {
					if (slot - 230 <= 31) {
						slot -= 200;
					}
				} else {
					slot -= 100;
				}
				if (a_arma->HasPartOf(slotMap.at(slot))) {
					dismemberInstance->UpdateDismemberPartion(data.slot, a_enable);
				}
			}
		}
	} else {
		a_shape->SetAppCulled(a_enable);
	}
}


void Dismemberment::ToggleExtraParts(const RE::BSTArray<RE::BGSHeadPart*>& a_parts, RE::NiAVObject* a_root, RE::TESObjectARMA* a_arma, const bool a_hide)
{
	for (auto& headpart : a_parts) {
		if (headpart) {
			if (auto node = a_root->GetObjectByName(headpart->formEditorID); node) {
				if (const auto shape = node->AsGeometry(); shape) {
					TogglePartition(shape, a_arma, a_hide);
				}
			}
			ToggleExtraParts(headpart->extraParts, a_root, a_arma, a_hide);
		}
	}
}


void Dismemberment::ToggleExtraParts(const RE::BSTArray<RE::BGSHeadPart*>& a_parts, RE::NiAVObject* a_root, RE::TESRace* a_race, bool a_hide)
{
	for (auto& headpart : a_parts) {
		if (headpart) {
			if (auto node = a_root->GetObjectByName(headpart->formEditorID); node) {
				if (const auto shape = node->AsGeometry(); shape) {
					TogglePartition(shape, a_race, a_hide);
				}
			}
			ToggleExtraParts(headpart->extraParts, a_root, a_race, a_hide);
		}
	}
}

void Dismemberment::TogglePartition(RE::BSGeometry* a_shape, RE::TESRace* a_race, bool a_enable)
{
	const auto skinInstance = a_shape->skinInstance.get();
	if (const auto dismemberInstance = netimmerse_cast<RE::BSDismemberSkinInstance*>(skinInstance); dismemberInstance) {
		if (dismemberInstance->partitions) {
			stl::span<RE::BSDismemberSkinInstance::Data> span(dismemberInstance->partitions, dismemberInstance->numPartitions);
			for (auto& data : span) {
				auto slot = data.slot;
				if (slot - 130 > 31) {
					if (slot - 230 <= 31) {
						slot -= 200;
					}
				} else {
					slot -= 100;
				}
				if (slot == to_underlying(*a_race->data.hairObject) + 30) {
					dismemberInstance->UpdateDismemberPartion(data.slot, a_enable);
				}
			}
		}
	} else {
		a_shape->SetAppCulled(a_enable);
	}
}