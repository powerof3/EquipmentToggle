#include "Graphics.h"

namespace Graphics::Slot
{
	SlotData GetHeadSlots()
	{
		for (auto& data : armorSlots | std::views::values) {
			auto& [autoToggle, slots] = data;
			for (auto& slot : slots) {
				if (headSlots.count(slot)) {
					return data;
				}
			}
		}
		return { false, std::set<Biped>{} };
	}

	void toggle_slots(RE::Actor* a_actor, const RE::BSTSmartPointer<RE::BipedAnim>& a_biped, RE::NiAVObject* a_root, const std::set<Biped>& a_slots, bool a_hide)
	{
		if (!a_actor || !a_biped || !a_root) {
			return;
		}

		SKSE::GetTaskInterface()->AddTask([a_biped, a_slots, a_actor, a_root, a_hide]() {
			for (auto& slot : a_slots) {
				auto& object = a_biped->objects[slot];
				if (const auto item = object.item; !item) {
					continue;
				}
				if (const auto node = object.partClone.get(); node) {
					Serialization::SetToggleState(a_actor, slot, a_hide);

					node->CullNode(a_hide);
					if (slot < Biped::kEditorTotal) {
						if (const auto parent = node->parent; parent) {
							detail::toggle_decal(parent, node, a_hide);
						}
						switch (slot) {
						case Biped::kHead:
						case Biped::kHair:
						case Biped::kLongHair:
						case Biped::kCirclet:
						case Biped::kEars:
						case Biped::kDecapitateHead:
							{
								Head::UpdateHeadPart(a_actor, a_root, object.addon, HeadPart::kHair, a_hide);
								Head::UpdateHeadPart(a_actor, a_root, object.addon, HeadPart::kFace, a_hide);
							}
							break;
						default:
							break;
						}
					}
				}
			}
		});
	}
	void toggle_slots(RE::Actor* a_actor, const RE::BSTSmartPointer<RE::BipedAnim>& a_biped, RE::NiAVObject* a_root, const SlotData& a_slotData)
	{
		if (!a_actor || !a_biped || !a_root) {
			return;
		}

		SKSE::GetTaskInterface()->AddTask([a_biped, a_slotData, a_actor, a_root]() {
			auto& [autoToggle, slots] = a_slotData;
			for (auto& slot : slots) {
				auto& object = a_biped->objects[slot];
				if (const auto item = object.item; !item) {
					continue;
				}
				if (const auto node = object.partClone.get(); node) {
					const auto hiddenState = Serialization::GetToggleState(a_actor, slot, autoToggle);
					Serialization::SetToggleState(a_actor, slot, !hiddenState);

					node->CullNode(!hiddenState);
					if (slot < Biped::kEditorTotal) {
						if (const auto parent = node->parent; parent) {
							detail::toggle_decal(parent, node, !hiddenState);
						}
						switch (slot) {
						case Biped::kHead:
						case Biped::kHair:
						case Biped::kLongHair:
						case Biped::kCirclet:
						case Biped::kEars:
						case Biped::kDecapitateHead:
							{
								Head::UpdateHeadPart(a_actor, a_root, object.addon, HeadPart::kHair, !hiddenState);
								Head::UpdateHeadPart(a_actor, a_root, object.addon, HeadPart::kFace, !hiddenState);
							}
							break;
						default:
							break;
						}
					}
				}
			}
		});
	}
	void toggle_slots_synced(RE::Actor* a_actor, const RE::BSTSmartPointer<RE::BipedAnim>& a_biped, RE::NiAVObject* a_root, const SlotData& a_slotData)
	{
		if (!a_actor || !a_biped || !a_root) {
			return;
		}

		const auto task = SKSE::GetTaskInterface();
		task->AddTask([a_biped, a_slotData, a_actor, a_root]() {
			auto& [autoToggle, slots] = a_slotData;
			for (auto& slot : slots) {
				auto& object = a_biped->objects[slot];
				if (const auto item = object.item; !item) {
					continue;
				}
				if (const auto node = object.partClone.get(); node) {
					const auto hiddenState = !Serialization::GetToggleState(RE::PlayerCharacter::GetSingleton(), slot, autoToggle);
					Serialization::SetToggleState(RE::PlayerCharacter::GetSingleton(), slot, !hiddenState);

					node->SetAppCulled(!hiddenState);
					if (slot < Biped::kEditorTotal) {
						if (const auto parent = node->parent; parent) {
							detail::toggle_decal(parent, node, !hiddenState);
						}
						switch (slot) {
						case Biped::kHead:
						case Biped::kHair:
						case Biped::kLongHair:
						case Biped::kCirclet:
						case Biped::kEars:
						case Biped::kDecapitateHead:
							{
								Head::UpdateHeadPart(a_actor, a_root, object.addon, HeadPart::kHair, !hiddenState);
								Head::UpdateHeadPart(a_actor, a_root, object.addon, HeadPart::kFace, !hiddenState);
							}
							break;
						default:
							break;
						}
					}
				}
			}
		});
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
}

namespace Graphics
{
	void ToggleActorEquipment(RE::Actor* a_actor, const bool a_hide)
	{
		for (auto& armorData : armorSlots | std::views::values) {
			auto& [autoToggle, slots] = armorData;

			for (std::uint32_t i = 0; i < 2; i++) {
				Slot::toggle_slots(a_actor, a_actor->GetBiped(i), a_actor->Get3D(i), slots, a_hide);
			}
		}

		for (auto& weaponData : weaponSlots | std::views::values) {
			auto& [autoToggle, slots] = weaponData;

			for (std::uint32_t i = 0; i < 2; i++) {
				Slot::toggle_slots(a_actor, a_actor->GetBiped(i), a_actor->Get3D(i), slots, a_hide);
			}
		}
	}

	void ToggleActorEquipment(RE::Actor* a_actor, const SlotData& a_slotData)
	{
		for (std::uint32_t i = 0; i < 2; i++) {
			Slot::toggle_slots(a_actor, a_actor->GetBiped(i), a_actor->Get3D(i), a_slotData);
		}
	}

	void ToggleActorHeadParts(RE::Actor* a_actor, bool a_hide)
	{
		if (auto [autoToggle, slots] = Slot::GetHeadSlots(); !slots.empty()) {
			for (std::uint32_t i = 0; i < 2; i++) {
				Slot::toggle_slots(a_actor, a_actor->GetBiped(i), a_actor->Get3D(i), slots, a_hide);
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
						Slot::toggle_slots(actor.get(), actor->GetBiped(false), actor->Get3D(false), slots, a_hide);
					}
					for (auto& weaponData : weaponSlots | std::views::values) {
						auto& [autoToggle, slots] = weaponData;
						Slot::toggle_slots(actor.get(), actor->GetBiped(false), actor->Get3D(false), slots, a_hide);
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
						Slot::toggle_slots_synced(actor.get(), actor->GetBiped(false), actor->Get3D(false), a_slotData);
					} else {
						Slot::toggle_slots(actor.get(), actor->GetBiped(false), actor->Get3D(false), a_slotData);
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
						Slot::toggle_slots(actor.get(), actor->GetBiped(false), actor->Get3D(false), slots, a_hide);
					}
					for (auto& weaponData : weaponSlots | std::views::values) {
						auto& [autoToggle, slots] = weaponData;
						Slot::toggle_slots(actor.get(), actor->GetBiped(false), actor->Get3D(false), slots, a_hide);
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
					Slot::toggle_slots(actor.get(), actor->GetBiped(false), actor->Get3D(false), a_slotData);
				}
			}
		}
	}
}
