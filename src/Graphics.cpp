#include "Graphics.h"
#include "Serialization.h"
#include "Settings.h"

void Graphics::detail::toggle_partition(RE::BSGeometry& a_shape, const RE::TESObjectARMA& a_arma, bool a_hide)
{
	if (const auto dismemberInstance = netimmerse_cast<RE::BSDismemberSkinInstance*>(a_shape.skinInstance.get()); dismemberInstance && dismemberInstance->partitions) {
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
			const auto bipedSlot = slotMap.find(slot);
			if (bipedSlot != slotMap.end() && a_arma.HasPartOf(bipedSlot->second)) {
				dismemberInstance->UpdateDismemberPartion(data.slot, a_hide);
			}
		}
	} else {
		a_shape.SetAppCulled(!a_hide);
	}
}

void Graphics::detail::toggle_partition(const RE::BSGeometry& a_shape, const RE::TESRace& a_race, bool a_hide)
{
	if (const auto dismemberInstance = netimmerse_cast<RE::BSDismemberSkinInstance*>(a_shape.skinInstance.get()); dismemberInstance && dismemberInstance->partitions) {
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
	if (const auto dismemberInstance = netimmerse_cast<RE::BSDismemberSkinInstance*>(a_shape.skinInstance.get()); dismemberInstance && dismemberInstance->partitions) {
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
			if (const auto node = a_root.GetObjectByName(headpart->formEditorID); node) {
				if (const auto shape = node->AsGeometry(); shape) {
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
			if (const auto node = a_root.GetObjectByName(headpart->formEditorID); node) {
				if (const auto shape = node->AsGeometry(); shape) {
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
			if (const auto node = a_root.GetObjectByName(headpart->formEditorID); node) {
				if (const auto shape = node->AsGeometry(); shape) {
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
		if (const auto invChanges = a_actor->GetInventoryChanges(); invChanges) {
			const auto wornMask = invChanges->GetWornMask();
			if (const auto headSlot = stl::to_underlying(*race->data.headObject); headSlot <= 31) {
				const auto headMask = 1 << headSlot;
				if (headMask & wornMask) {
					face->SetAppCulled(!a_hide);

					if (const auto headPart = npc->GetCurrentHeadPartByType(a_type); headPart) {
						if (const auto node = a_root->GetObjectByName(headPart->formEditorID); node) {
							if (const auto shape = node->AsGeometry(); shape) {
								toggle_partition(*shape, a_hide);
							}
						}
						toggle_extra_parts(headPart->extraParts, *a_root, a_hide);
					}

				} else {
					a_hide ? face->SetAppCulled(!a_hide) : face->SetAppCulled(a_hide);

					if (const auto headPart = npc->GetCurrentHeadPartByType(a_type); headPart) {
						if (const auto node = a_root->GetObjectByName(headPart->formEditorID); node) {
							if (const auto shape = node->AsGeometry(); shape) {
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

void Graphics::toggle_slots(RE::Actor* a_actor, const RE::BSTSmartPointer<RE::BipedAnim>& a_biped, RE::NiAVObject* a_root, const Slot::Set& a_slots, Slot::State a_state)
{
	if (!a_actor || !a_biped || !a_root) {
		return;
	}

	SKSE::GetTaskInterface()->AddTask([a_biped, a_slots, a_actor, a_root, a_state]() {
		const bool isFP = IsFirstPerson(a_actor, a_root);
		for (auto& slot : a_slots) {
			auto& object = a_biped->objects[slot];
			if (const auto node = object.partClone; node) {
				Serialization::SetToggleState(a_actor, slot, a_state, isFP);

				node->CullNode(static_cast<bool>(a_state));
				if (slot < Biped::kEditorTotal) {
					if (const auto parent = node->parent; parent) {
						detail::toggle_decal(parent, node.get(), static_cast<bool>(a_state));
					}
					switch (slot) {
					case Biped::kHead:
					case Biped::kHair:
					case Biped::kLongHair:
					case Biped::kEars:
						{
							detail::update_head_part(a_actor, a_root, object.addon, HeadPart::kHair, static_cast<bool>(a_state));
							detail::update_head_part(a_actor, a_root, object.addon, HeadPart::kFace, static_cast<bool>(a_state));
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

void Graphics::toggle_slots(RE::Actor* a_actor, const RE::BSTSmartPointer<RE::BipedAnim>& a_biped, RE::NiAVObject* a_root, const Slot::Set& a_slots)
{
	if (!a_actor || !a_biped || !a_root) {
		return;
	}

	SKSE::GetTaskInterface()->AddTask([a_biped, a_slots, a_actor, a_root]() {
		const bool isFP = IsFirstPerson(a_actor, a_root);
		for (auto& slot : a_slots) {
			const auto& object = a_biped->objects[slot];
			if (const auto node = object.partClone; node) {
				auto invert_state = !Serialization::GetToggleState(a_actor, slot, isFP);
				Serialization::SetToggleState(a_actor, slot, invert_state, isFP);

				node->CullNode(static_cast<bool>(invert_state));
				if (slot < Biped::kEditorTotal) {
					if (const auto parent = node->parent; parent) {
						detail::toggle_decal(parent, node.get(), static_cast<bool>(invert_state));
					}
					switch (slot) {
					case Biped::kHead:
					case Biped::kHair:
					case Biped::kLongHair:
					case Biped::kEars:
						{
							detail::update_head_part(a_actor, a_root, object.addon, HeadPart::kHair, static_cast<bool>(invert_state));
							detail::update_head_part(a_actor, a_root, object.addon, HeadPart::kFace, static_cast<bool>(invert_state));
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

	Settings::GetSingleton()->ForEachSlot([&](const SlotData& a_slotData) {
		if (a_slotData.ContainsHeadSlots()) {
			slotData = a_slotData;
			return false;
		}
		return true;
	});

	return slotData;
}

void Graphics::ToggleActorEquipment(RE::Actor* a_actor, std::function<bool(const SlotData& a_slotData)> a_func, Slot::State a_state)
{
	Settings::GetSingleton()->ForEachSlot([&](const SlotData& a_slotData) {
		if (a_func(a_slotData)) {
			for (std::uint32_t i = 0; i < 2; i++) {
				toggle_slots(a_actor, a_actor->GetBiped(i), a_actor->Get3D(i), a_slotData.slots, a_state);
			}
		}
		return true;
	});
}

bool Graphics::ToggleActorHeadParts(RE::Actor* a_actor, Slot::State a_state)
{
	auto [hotKey, hide, unhide, slots] = GetHeadSlots();
	if (!slots.empty() && hide.equipped.CanDoToggle(a_actor) && Serialization::GetToggleState(a_actor, Biped::kHead, false) == a_state) {
		for (std::uint32_t i = 0; i < 2; i++) {
			toggle_slots(a_actor, a_actor->GetBiped(i), a_actor->Get3D(i), slots, a_state);
		}
		return true;
	}
	return false;
}

bool Graphics::IsFirstPerson(const RE::Actor* a_actor, const RE::NiAVObject* a_root)
{
	if (a_actor->IsPlayerRef()) {
		return a_root && a_root == a_actor->Get3D(true);
	}
	return false;
}

bool Graphics::IsFirstPerson(const RE::Actor* a_actor, const RE::BipedAnim* a_biped)
{
	if (a_actor->IsPlayerRef()) {
		return a_actor->GetBiped(true) && a_actor->GetBiped(true).get() == a_biped;
	}
	return false;
}

void Graphics::ToggleFollowerEquipment(std::function<bool(const SlotData& a_slotData)> a_func, Slot::State a_state)
{
	const auto player = RE::PlayerCharacter::GetSingleton();
	const auto xFollowerList = player ? player->extraList.GetByType<RE::ExtraFollower>() : nullptr;

	if (xFollowerList) {
		for (auto& [handle, dist] : xFollowerList->actorFollowers) {
			const auto actor = handle.get();
			if (actor && actor->HasKeywordString(NPC)) {
				Settings::GetSingleton()->ForEachSlot([&](const SlotData& a_slotData) {
					if (a_func(a_slotData)) {
						toggle_slots(actor.get(), actor->GetBiped(false), actor->Get3D(false), a_slotData.slots, a_state);
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
