#include "Graphics.h"
#include "Serialization.h"

void Graphics::detail::toggle_partition(RE::BSGeometry& a_shape, const RE::TESObjectARMA& a_arma, bool a_hide)
{
	if (const auto dismemberInstance = netimmerse_cast<RE::BSDismemberSkinInstance*>(a_shape.skinInstance.get()); dismemberInstance->partitions) {
		const std::span span(dismemberInstance->partitions, dismemberInstance->numPartitions);
		for (const auto& data : span) {
			auto slot = data.slot;
			if (slot > 61) {
				if (slot - 130 > 31) {
					if (slot - 230 <= 31) {
						slot -= 200;
					}
				} else {
					slot -= 100;
				}
			}
			if (a_arma.HasPartOf(slotMap.at(slot))) {
				dismemberInstance->UpdateDismemberPartion(data.slot, a_hide);
			}
		}
	} else {
		a_shape.SetAppCulled(!a_hide);
	}
}
void Graphics::detail::toggle_partition(const RE::BSGeometry& a_shape, const RE::TESRace& a_race, bool a_hide)
{
	if (const auto dismemberInstance = netimmerse_cast<RE::BSDismemberSkinInstance*>(a_shape.skinInstance.get()); dismemberInstance->partitions) {
		const auto hairSlot = stl::to_underlying(a_race.data.hairObject.get()) + 30;

		const std::span span(dismemberInstance->partitions, dismemberInstance->numPartitions);
		for (const auto& data : span) {
			auto slot = data.slot;
			if (slot > 61) {
				if (slot - 130 > 31) {
					if (slot - 230 <= 31) {
						slot -= 200;
					}
				} else {
					slot -= 100;
				}
			}
			if (slot == hairSlot) {
				dismemberInstance->UpdateDismemberPartion(slot, a_hide);
			}
		}
	}
}
void Graphics::detail::toggle_partition(RE::BSGeometry& a_shape, bool a_hide)
{
	if (const auto dismemberInstance = netimmerse_cast<RE::BSDismemberSkinInstance*>(a_shape.skinInstance.get()); dismemberInstance->partitions) {
		std::span span(dismemberInstance->partitions, dismemberInstance->numPartitions);
		for (auto& [editorVisible, startNetBoneSet, slot] : span) {
			dismemberInstance->UpdateDismemberPartion(slot, a_hide);
		}
	} else {
		a_shape.SetAppCulled(!a_hide);
	}
}

void Graphics::detail::toggle_extra_parts(const RE::BSTArray<RE::BGSHeadPart*>& a_parts, RE::NiAVObject& a_root, const RE::TESObjectARMA& a_arma, bool a_hide)
{
	for (auto& headpart : a_parts) {
		if (headpart) {
			if (const auto node = a_root.GetObjectByName(headpart->formEditorID)) {
				if (const auto shape = node->AsGeometry()) {
					toggle_partition(*shape, a_arma, a_hide);
				}
			}
			toggle_extra_parts(headpart->extraParts, a_root, a_arma, a_hide);
		}
	}
}
void Graphics::detail::toggle_extra_parts(const RE::BSTArray<RE::BGSHeadPart*>& a_parts, RE::NiAVObject& a_root, const RE::TESRace& a_race, bool a_hide)
{
	for (auto& headpart : a_parts) {
		if (headpart) {
			if (const auto node = a_root.GetObjectByName(headpart->formEditorID)) {
				if (const auto shape = node->AsGeometry()) {
					toggle_partition(*shape, a_race, a_hide);
				}
			}
			toggle_extra_parts(headpart->extraParts, a_root, a_race, a_hide);
		}
	}
}
void Graphics::detail::toggle_extra_parts(const RE::BSTArray<RE::BGSHeadPart*>& a_parts, RE::NiAVObject& a_root, bool a_hide)
{
	for (auto& headpart : a_parts) {
		if (headpart) {
			if (const auto node = a_root.GetObjectByName(headpart->formEditorID)) {
				if (const auto shape = node->AsGeometry()) {
					toggle_partition(*shape, a_hide);
				}
			}
			toggle_extra_parts(headpart->extraParts, a_root, a_hide);
		}
	}
}

void Graphics::detail::update_head_part(RE::Actor* a_actor, RE::NiAVObject* a_root, RE::TESObjectARMA* a_arma, const HeadPart a_type, const bool a_hide)
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
								toggle_partition(*shape, a_hide);
							}
						}
						toggle_extra_parts(headPart->extraParts, *a_root, a_hide);
					}

				} else {
					a_hide ? face->SetAppCulled(!a_hide) : face->SetAppCulled(a_hide);

					if (const auto headPart = npc->GetCurrentHeadPartByType(a_type)) {
						if (const auto node = a_root->GetObjectByName(headPart->formEditorID)) {
							if (const auto shape = node->AsGeometry()) {
								toggle_partition(*shape, *a_arma, a_hide);
							}
						}
						toggle_extra_parts(headPart->extraParts, *a_root, *a_arma, a_hide);
					}
				}
			}
		}
	}
}

void Graphics::toggle_slots(RE::Actor* a_actor, const RE::BSTSmartPointer<RE::BipedAnim>& a_biped, RE::NiAVObject* a_root, const SlotSet& a_slots, bool a_hide)
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
							detail::update_head_part(a_actor, a_root, object.addon, HeadPart::kHair, a_hide);
							detail::update_head_part(a_actor, a_root, object.addon, HeadPart::kFace, a_hide);
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
void Graphics::toggle_slots(RE::Actor* a_actor, const RE::BSTSmartPointer<RE::BipedAnim>& a_biped, RE::NiAVObject* a_root, const SlotSet& a_slots)
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
							detail::update_head_part(a_actor, a_root, object.addon, HeadPart::kHair, !hiddenState);
							detail::update_head_part(a_actor, a_root, object.addon, HeadPart::kFace, !hiddenState);
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

SlotData Graphics::GetHeadSlots()
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

void Graphics::ToggleActorEquipment(RE::Actor* a_actor, std::function<bool(const SlotData& a_slotData)> a_func, State a_hide)
{
	Settings::GetSingleton()->ForEachSlot([&](const SlotData& a_slotData) {
		if (a_func(a_slotData)) {
			for (std::uint32_t i = 0; i < 2; i++) {
				toggle_slots(a_actor, a_actor->GetBiped(i), a_actor->Get3D(i), a_slotData.slots, static_cast<bool>(a_hide));
			}
		}
		return true;
	});
}

bool Graphics::ToggleActorHeadParts(RE::Actor* a_actor, State a_hide)
{
	auto [hotKey, hide, unhide, slots] = GetHeadSlots();
	if (!slots.empty() && hide.equipped.CanDoToggle(a_actor) && Serialization::GetToggleState(a_actor, Biped::kHead)) {
		for (std::uint32_t i = 0; i < 2; i++) {
			toggle_slots(a_actor, a_actor->GetBiped(i), a_actor->Get3D(i), slots, static_cast<bool>(a_hide));
		}
		return true;
	}
	return false;
}

void Graphics::ToggleFollowerEquipment(std::function<bool(const SlotData& a_slotData)> a_func, State a_hide)
{
	const auto player = RE::PlayerCharacter::GetSingleton();
	const auto xFollowerList = player ? player->extraList.GetByType<RE::ExtraFollower>() : nullptr;

	if (xFollowerList) {
		for (auto& [handle, dist] : xFollowerList->actorFollowers) {
			const auto actor = handle.get();
			if (actor && actor->HasKeywordString(NPC)) {
				Settings::GetSingleton()->ForEachSlot([&](const SlotData& a_slotData) {
					if (a_func(a_slotData)) {
						toggle_slots(actor.get(), actor->GetBiped(false), actor->Get3D(false), a_slotData.slots, static_cast<bool>(a_hide));
					}
					return true;
				});
			}
		}
	}
}

void Graphics::ToggleNPCEquipment(std::function<bool(RE::Actor* a_actor, const SlotData& a_slotData)> a_func)
{
	if (const auto processList = RE::ProcessLists::GetSingleton(); processList) {
		for (auto& handle : processList->highActorHandles) {
			if (const auto actor = handle.get(); actor) {
				Settings::GetSingleton()->ForEachSlot([&](const SlotData& a_slotData) {
					if (a_func(actor.get(), a_slotData)) {
						toggle_slots(actor.get(), actor->GetBiped(false), actor->Get3D(false), a_slotData.slots);
					}
					return true;
				});
			}
		}
	}
}

void Graphics::ToggleAllEquipment(std::function<bool(RE::Actor* a_actor, const SlotData& a_slotData)> a_func)
{
	const auto player = RE::PlayerCharacter::GetSingleton();
	Settings::GetSingleton()->ForEachSlot([&](const SlotData& a_slotData) {
		if (a_func(player, a_slotData)) {
			for (std::uint32_t i = 0; i < 2; i++) {
				toggle_slots(player, player->GetBiped(i), player->Get3D(i), a_slotData.slots);
			}
		}
		return true;
	});

	ToggleNPCEquipment(a_func);
}
