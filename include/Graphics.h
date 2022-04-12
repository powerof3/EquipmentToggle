#pragma once

namespace Graphics
{
	namespace Head
	{
		struct detail
		{
			static void toggle_partition(RE::BSGeometry& a_shape, const RE::TESObjectARMA& a_arma, bool a_hide)
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
			static void toggle_partition(const RE::BSGeometry& a_shape, const RE::TESRace& a_race, bool a_hide)
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
			static void toggle_partition(RE::BSGeometry& a_shape, bool a_hide)
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

			static void toggle_extra_parts(const RE::BSTArray<RE::BGSHeadPart*>& a_parts, RE::NiAVObject& a_root, const RE::TESObjectARMA& a_arma, bool a_hide)
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
			static void toggle_extra_parts(const RE::BSTArray<RE::BGSHeadPart*>& a_parts, RE::NiAVObject& a_root, const RE::TESRace& a_race, bool a_hide)
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
			static void toggle_extra_parts(const RE::BSTArray<RE::BGSHeadPart*>& a_parts, RE::NiAVObject& a_root, bool a_hide)
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
		};

		void UpdateHeadPart(RE::Actor* a_actor, RE::NiAVObject* a_root, RE::TESObjectARMA* a_arma, HeadPart a_type, bool a_hide);
		void UpdateHair(RE::Actor* a_actor, RE::NiAVObject* a_root, bool a_hide);
		void UpdateFacePartitions(RE::Actor* a_actor, RE::NiAVObject* a_root, std::uint32_t a_slot, bool a_hide);
	}

	namespace Slot
	{
        void set_toggle_data(RE::NiAVObject* a_object, const Biped a_slot, bool a_hide);

        bool get_toggle_data(RE::NiAVObject* a_object, const Biped a_slot, bool a_default);

        struct detail
		{
		    static void toggle_decal(RE::NiAVObject* a_root, RE::NiAVObject* a_node, bool a_hide)
			{
                if (const auto decalNode = netimmerse_cast<RE::BGSDecalNode*>(a_root->GetObjectByName(RE::FixedStrings::GetSingleton()->skinnedDecalNode))) {
					RE::BSVisit::TraverseScenegraphGeometries(a_node, [&](RE::BSGeometry* a_geometry) -> RE::BSVisit::BSVisitControl {
						for (auto& tempEffect : decalNode->decals) {
							if (tempEffect) {
								const auto geometryDecalEffect = tempEffect->As<RE::BSTempEffectGeometryDecal>();
								if (geometryDecalEffect && geometryDecalEffect->attachedGeometry.get() == a_geometry) {
									if (const auto decal = geometryDecalEffect->Get3D()) {
										decal->CullNode(a_hide);
									}
								}
							}
						}
						return RE::BSVisit::BSVisitControl::kContinue;
					});
				}
			}

			static void toggle_slots(RE::Actor* a_actor, const RE::BSTSmartPointer<RE::BipedAnim>& a_biped, RE::NiAVObject* a_root, const std::set<Biped>& a_slots, bool a_hide)
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
							set_toggle_data(a_root, slot, a_hide);

							node->CullNode(a_hide);
							if (slot < Biped::kEditorTotal) {
								if (const auto parent = node->parent; parent) {
									toggle_decal(parent, node, a_hide);
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

			static void toggle_slots(RE::Actor* a_actor, const RE::BSTSmartPointer<RE::BipedAnim>& a_biped, RE::NiAVObject* a_root, const SlotData& a_slotData)
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
							const auto hiddenState = get_toggle_data(a_root, slot, autoToggle);
							set_toggle_data(a_root, slot, !hiddenState);

							node->CullNode(!hiddenState);
							if (slot < Biped::kEditorTotal) {
								if (const auto parent = node->parent; parent) {
									toggle_decal(parent, node, !hiddenState);
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

			static void toggle_slots_synced(RE::Actor* a_actor, const RE::BSTSmartPointer<RE::BipedAnim>& a_biped, RE::NiAVObject* a_root, RE::NiAVObject* a_playerRoot, const SlotData& a_slotData)
			{
				if (!a_actor || !a_biped || !a_root || !a_playerRoot) {
					return;
				}

				const auto task = SKSE::GetTaskInterface();
				task->AddTask([a_biped, a_slotData, a_actor, a_root, a_playerRoot]() {
					auto& [autoToggle, slots] = a_slotData;
					for (auto& slot : slots) {
						auto& object = a_biped->objects[slot];
						if (const auto item = object.item; !item) {
							continue;
						}
						if (const auto node = object.partClone.get(); node) {
							const auto hiddenState = !get_toggle_data(a_playerRoot, slot, autoToggle);
							set_toggle_data(a_root, slot, !hiddenState);

							node->SetAppCulled(!hiddenState);
							if (slot < Biped::kEditorTotal) {
								if (const auto parent = node->parent; parent) {
									toggle_decal(parent, node, !hiddenState);
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
		};

		void ToggleActorEquipment(RE::Actor* a_actor, bool a_hide);
		void ToggleActorEquipment(RE::Actor* a_actor, const SlotData& a_slotData);

		void ToggleFollowerEquipment(bool a_hide);
		void ToggleFollowerEquipment(const SlotData& a_slotData, bool a_playerSync = false);

		void ToggleNPCEquipment(bool a_hide);
		void ToggleNPCEquipment(const SlotData& a_slotData);
	}
}
