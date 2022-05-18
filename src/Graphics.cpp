#include "Graphics.h"
#include "Serialization.h"

namespace Graphics::Slot
{
	void set_toggle_data(const RE::Actor* a_actor, const Biped a_slot, bool a_hide)
	{
		Biped slot;
		if (headSlots.find(a_slot) != headSlots.end()) {
			slot = Biped::kHead;
		} else {
			slot = a_slot;
		}

		Serialization::AutoToggleMap::GetSingleton()->Add(a_actor, slot, a_hide);
	}

	bool get_toggle_data(const RE::Actor* a_actor, const Biped a_slot, bool a_default)
	{
		Biped slot;
		if (headSlots.find(a_slot) != headSlots.end()) {
			slot = Biped::kHead;
		} else {
			slot = a_slot;
		}

		if (const auto state = Serialization::AutoToggleMap::GetSingleton()->GetToggleState(a_actor, slot); state != -1) {
			return static_cast<bool>(state);
		}

		Serialization::AutoToggleMap::GetSingleton()->Add(a_actor, slot, a_default);

		return a_default;
	}

	void ToggleActorEquipment(RE::Actor* a_actor, const bool a_hide)
	{
		for (auto& armorData : armorSlots | std::views::values) {
			auto& [autoToggle, slots] = armorData;

			for (std::uint32_t i = 0; i < 2; i++) {
				detail::toggle_slots(a_actor, a_actor->GetBiped(i), a_actor->Get3D(i), slots, a_hide);
			}
		}

		for (auto& weaponData : weaponSlots | std::views::values) {
			auto& [autoToggle, slots] = weaponData;

			for (std::uint32_t i = 0; i < 2; i++) {
				detail::toggle_slots(a_actor, a_actor->GetBiped(i), a_actor->Get3D(i), slots, a_hide);
			}
		}
	}

	void ToggleActorEquipment(RE::Actor* a_actor, const SlotData& a_slotData)
	{
		for (std::uint32_t i = 0; i < 2; i++) {
			detail::toggle_slots(a_actor, a_actor->GetBiped(i), a_actor->Get3D(i), a_slotData);
		}
	}

	void ToggleActorHeadParts(RE::Actor* a_actor, bool a_hide)
	{
		if (auto [autoToggle, slots] = get_head_slots(); !slots.empty()) {
			for (std::uint32_t i = 0; i < 2; i++) {
				detail::toggle_slots(a_actor, a_actor->GetBiped(i), a_actor->Get3D(i), slots, a_hide);
			}
		}
	}

	void ToggleFollowerEquipment(const bool a_hide)
	{
		if (const auto processList = RE::ProcessLists::GetSingleton()) {
			for (auto& handle : processList->highActorHandles) {
				const auto actor = handle.get();
				if (actor && actor->IsPlayerTeammate() && actor->HasKeywordString(NPC)) {
					for (auto& armorData : armorSlots | std::views::values) {
						auto& [autoToggle, slots] = armorData;
						detail::toggle_slots(actor.get(), actor->GetBiped(false), actor->Get3D(false), slots, a_hide);
					}
					for (auto& weaponData : weaponSlots | std::views::values) {
						auto& [autoToggle, slots] = weaponData;
						detail::toggle_slots(actor.get(), actor->GetBiped(false), actor->Get3D(false), slots, a_hide);
					}
				}
			}
		}
	}

	void ToggleFollowerEquipment(const SlotData& a_slotData, bool a_playerSync)
	{
		if (const auto processList = RE::ProcessLists::GetSingleton()) {
			for (auto& handle : processList->highActorHandles) {
				const auto actor = handle.get();
				if (actor && actor->IsPlayerTeammate() && actor->HasKeywordString(NPC)) {
					if (a_playerSync) {
						detail::toggle_slots_synced(actor.get(), actor->GetBiped(false), actor->Get3D(false), a_slotData);
					} else {
						detail::toggle_slots(actor.get(), actor->GetBiped(false), actor->Get3D(false), a_slotData);
					}
				}
			}
		}
	}

	void ToggleNPCEquipment(const bool a_hide)
	{
		if (const auto processList = RE::ProcessLists::GetSingleton(); processList) {
			for (auto& handle : processList->highActorHandles) {
				const auto actor = handle.get();
				if (actor && actor->HasKeywordString(NPC)) {
					for (auto& armorData : armorSlots | std::views::values) {
						auto& [autoToggle, slots] = armorData;
						detail::toggle_slots(actor.get(), actor->GetBiped(false), actor->Get3D(false), slots, a_hide);
					}
					for (auto& weaponData : weaponSlots | std::views::values) {
						auto& [autoToggle, slots] = weaponData;
						detail::toggle_slots(actor.get(), actor->GetBiped(false), actor->Get3D(false), slots, a_hide);
					}
				}
			}
		}
	}

	void ToggleNPCEquipment(const SlotData& a_slotData)
	{
		if (const auto processList = RE::ProcessLists::GetSingleton(); processList) {
			for (auto& handle : processList->highActorHandles) {
				const auto actor = handle.get();
				if (actor && actor->HasKeywordString(NPC)) {
					detail::toggle_slots(actor.get(), actor->GetBiped(false), actor->Get3D(false), a_slotData);
				}
			}
		}
	}
}

namespace Graphics::Head
{
	void UpdateHeadPart(RE::Actor* a_actor, RE::NiAVObject* a_root, RE::TESObjectARMA* a_arma, const HeadPart a_type, const bool a_hide)
	{
		const auto npc = a_actor ? a_actor->GetActorBase() : nullptr;
		const auto race = npc ? npc->GetRace() : nullptr;
		const auto face = a_root ? a_root->GetObjectByName(RE::FixedStrings::GetSingleton()->bsFaceGenNiNodeSkinned) : nullptr;

		if (face && race && a_arma) {
			if (const auto invChanges = a_actor->GetInventoryChanges()) {
				const auto wornMask = invChanges->GetWornMask();
				if (const auto headSlot = stl::to_underlying(*race->data.headObject); headSlot <= 31) {
					const auto headMask = 1 << headSlot;
					if (headMask & wornMask) {
						face->SetAppCulled(!a_hide);

						if (const auto headPart = npc->GetCurrentHeadPartByType(a_type)) {
							if (const auto node = a_root->GetObjectByName(headPart->formEditorID)) {
								if (const auto shape = node->AsGeometry()) {
									detail::toggle_partition(*shape, a_hide);
								}
							}
							detail::toggle_extra_parts(headPart->extraParts, *a_root, a_hide);
						}

					} else {
						a_hide ? face->SetAppCulled(!a_hide) : face->SetAppCulled(a_hide);

						if (const auto headPart = npc->GetCurrentHeadPartByType(a_type)) {
							if (const auto node = a_root->GetObjectByName(headPart->formEditorID)) {
								if (const auto shape = node->AsGeometry()) {
									detail::toggle_partition(*shape, *a_arma, a_hide);
								}
							}
							detail::toggle_extra_parts(headPart->extraParts, *a_root, *a_arma, a_hide);
						}
					}
				}
			}
		}
	}

	void UpdateHair(RE::Actor* a_actor, RE::NiAVObject* a_root, const bool a_hide)
	{
		const auto npc = a_actor ? a_actor->GetActorBase() : nullptr;
		const auto race = a_actor->GetRace();
		const auto face = a_root ? a_root->GetObjectByName(RE::FixedStrings::GetSingleton()->bsFaceGenNiNodeSkinned) : nullptr;

		if (npc && race && face) {
			if (const auto invChanges = a_actor->GetInventoryChanges()) {
				const auto wornMask = invChanges->GetWornMask();
				if (const auto headSlot = stl::to_underlying(*race->data.headObject); headSlot <= 31) {
					const auto headMask = 1 << headSlot;
					if (headMask & wornMask) {
						face->SetAppCulled(!a_hide);

						if (const auto headPart = npc->GetCurrentHeadPartByType(HeadPart::kHair)) {
							if (const auto node = a_root->GetObjectByName(headPart->formEditorID); node) {
								if (const auto shape = node->AsGeometry(); shape) {
									detail::toggle_partition(*shape, a_hide);
								}
							}
							detail::toggle_extra_parts(headPart->extraParts, *a_root, a_hide);
						}
					} else {
						a_hide ? face->SetAppCulled(!a_hide) : face->SetAppCulled(a_hide);

						if (const auto headPart = npc->GetCurrentHeadPartByType(HeadPart::kHair)) {
							if (const auto node = a_root->GetObjectByName(headPart->formEditorID)) {
								if (const auto shape = node->AsGeometry()) {
									detail::toggle_partition(*shape, *race, a_hide);
								}
							}
							detail::toggle_extra_parts(headPart->extraParts, *a_root, *race, a_hide);
						}
					}
				}
			}
		}
	}

	void UpdateFacePartitions(RE::Actor* a_actor, RE::NiAVObject* a_root, const std::uint32_t a_slot, bool a_hide)
	{
		if (!a_actor || !a_root) {
			return;
		}

		const auto bipedSlot = a_slot + 30;

		RE::BSVisit::TraverseScenegraphGeometries(a_root, [&](RE::BSGeometry* a_geometry) -> RE::BSVisit::BSVisitControl {
			if (const auto dismemberInstance = netimmerse_cast<RE::BSDismemberSkinInstance*>(a_geometry->skinInstance.get()); dismemberInstance) {
				if (dismemberInstance->partitions) {
					const std::span span(dismemberInstance->partitions, dismemberInstance->numPartitions);
					for (auto& [editorVisible, startNetBoneSet, slot] : span) {
						if (slot > 61) {
							if (slot - 130 > 31) {
								if (slot - 230 <= 31) {
									slot -= 200;
								}
							} else {
								slot -= 100;
							}
						}
						if (slot == bipedSlot) {
							dismemberInstance->UpdateDismemberPartion(slot, a_hide);
						}
					}
				}
			} else {
				a_geometry->SetAppCulled(!a_hide);
			}

			return RE::BSVisit::BSVisitControl::kContinue;
		});
	}
}
