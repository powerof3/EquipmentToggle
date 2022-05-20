#include "Graphics.h"

#include "Serialization.h"

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

namespace Graphics::Equipment
{
	void toggle_slots(RE::Actor* a_actor, const RE::BSTSmartPointer<RE::BipedAnim>& a_biped, RE::NiAVObject* a_root, const SlotSet& a_slots, bool a_hide)
	{
		if (!a_actor || !a_biped || !a_root) {
			return;
		}

		SKSE::GetTaskInterface()->AddTask([a_biped, a_slots, a_actor, a_root, a_hide]() {
			for (auto& slot : a_slots) {
				auto& object = a_biped->objects[slot];
				if (const auto node = object.partClone; node) {
					Serialization::SetToggleState(a_actor, slot, a_hide);

					node->CullNode(a_hide);
					if (slot < Biped::kEditorTotal) {
						if (const auto parent = node->parent; parent) {
							detail::toggle_decal(parent, node.get(), a_hide);
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
	void toggle_slots(RE::Actor* a_actor, const RE::BSTSmartPointer<RE::BipedAnim>& a_biped, RE::NiAVObject* a_root, const SlotSet& a_slots)
	{
		if (!a_actor || !a_biped || !a_root) {
			return;
		}

		SKSE::GetTaskInterface()->AddTask([a_biped, a_slots, a_actor, a_root]() {
			for (auto& slot : a_slots) {
                const auto& object = a_biped->objects[slot];
				if (const auto node = object.partClone; node) {
					const auto hiddenState = Serialization::GetToggleState(a_actor, slot);
				    Serialization::SetToggleState(a_actor, slot, !hiddenState);

					node->CullNode(!hiddenState);
					if (slot < Biped::kEditorTotal) {
						if (const auto parent = node->parent; parent) {
							detail::toggle_decal(parent, node.get(), !hiddenState);
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
	void toggle_slots_synced(RE::Actor* a_actor, const RE::BSTSmartPointer<RE::BipedAnim>& a_biped, RE::NiAVObject* a_root, const SlotSet& a_slots)
	{
		if (!a_actor || !a_biped || !a_root) {
			return;
		}

		const auto task = SKSE::GetTaskInterface();
		task->AddTask([a_biped, a_slots, a_actor, a_root]() {
			for (auto& slot : a_slots) {
				auto& object = a_biped->objects[slot];
				if (const auto item = object.item; !item) {
					continue;
				}
				if (const auto node = object.partClone.get(); node) {
					const auto hiddenState = !Serialization::GetToggleState(RE::PlayerCharacter::GetSingleton(), slot);
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

namespace Graphics
{
	SlotData GetHeadSlots()
	{
		SlotData slotData{};

	    Settings::GetSingleton()->ForEachArmorSlot([&](const SlotData& a_slotData) {
			if (a_slotData.ContainsHeadSlots()) {
				slotData = a_slotData;
				return false;
			}
			return true;
		});

	    return slotData;
	}

	void ToggleActorEquipment(RE::Actor* a_actor, std::function<bool(const SlotData& a_slotData)> a_func, bool a_hide)
	{
		Settings::GetSingleton()->ForEachSlot([&](const SlotData& a_slotData) {
			if (a_func(a_slotData)) {
				for (std::uint32_t i = 0; i < 2; i++) {
					Equipment::toggle_slots(a_actor, a_actor->GetBiped(i), a_actor->Get3D(i), a_slotData.slots, a_hide);
				}
			}
			return true;
		});
	}

	bool ToggleActorHeadParts(RE::Actor* a_actor, bool a_hide)
	{
		auto [hotKey, hide, unhide, slots] = GetHeadSlots();
	    if (!slots.empty() && hide.equipped.CanDoToggle(a_actor) && Serialization::GetToggleState(a_actor, Biped::kHead)) {
			for (std::uint32_t i = 0; i < 2; i++) {
				Equipment::toggle_slots(a_actor, a_actor->GetBiped(i), a_actor->Get3D(i), slots, a_hide);
			}
			return true;
		}
		return false;
	}

	void ToggleFollowerEquipment(std::function<bool(const SlotData& a_slotData)> a_func, bool a_hide)
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		const auto xFollowerList = player ? player->extraList.GetByType<RE::ExtraFollower>() : nullptr;

		if (xFollowerList) {
			for (auto& [handle, dist] : xFollowerList->actorFollowers) {
				const auto actor = handle.get();
				if (actor && actor->HasKeywordString(NPC)) {
					Settings::GetSingleton()->ForEachSlot([&](const SlotData& a_slotData) {
						if (a_func(a_slotData)) {
							Equipment::toggle_slots(actor.get(), actor->GetBiped(false), actor->Get3D(false), a_slotData.slots, a_hide);
						}
						return true;
					});
				}
			}
		}
	}

	void ToggleNPCEquipment(std::function<bool(RE::Actor* a_actor, const SlotData& a_slotData)> a_func)
	{
		if (const auto processList = RE::ProcessLists::GetSingleton(); processList) {
			for (auto& handle : processList->highActorHandles) {
				if (const auto actor = handle.get(); actor) {
					Settings::GetSingleton()->ForEachSlot([&](const SlotData& a_slotData) {
						if (a_func(actor.get(), a_slotData)) {
							Equipment::toggle_slots(actor.get(), actor->GetBiped(false), actor->Get3D(false), a_slotData.slots);
						}
						return true;
					});
				}
			}
		}
	}

    void ToggleAllEquipment(std::function<bool(RE::Actor* a_actor, const SlotData& a_slotData)> a_func)
	{
        const auto player = RE::PlayerCharacter::GetSingleton();
		Settings::GetSingleton()->ForEachSlot([&](const SlotData& a_slotData) {
			if (a_func(player, a_slotData)) {
				for (std::uint32_t i = 0; i < 2; i++) {
					Equipment::toggle_slots(player, player->GetBiped(i), player->Get3D(i), a_slotData.slots);
				}
			}
			return true;
		});

        ToggleNPCEquipment(a_func);
	}
}
