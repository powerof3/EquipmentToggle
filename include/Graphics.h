#pragma once

#include "Settings.h"

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
	}

	namespace Equipment
	{
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
		};

		static void toggle_slots(RE::Actor* a_actor, const RE::BSTSmartPointer<RE::BipedAnim>& a_biped, RE::NiAVObject* a_root, const SlotSet& a_slots, bool a_hide);

		static void toggle_slots(RE::Actor* a_actor, const RE::BSTSmartPointer<RE::BipedAnim>& a_biped, RE::NiAVObject* a_root, const SlotSet& a_slots);

		//static void toggle_slots_synced(RE::Actor* a_actor, const RE::BSTSmartPointer<RE::BipedAnim>& a_biped, RE::NiAVObject* a_root, const SlotSet& a_slots);
	}

	SlotData GetHeadSlots();

    void ToggleActorEquipment(RE::Actor* a_actor, std::function<bool(const SlotData& a_slotData)> a_func, bool a_hide);

    void ToggleFollowerEquipment(std::function<bool(const SlotData& a_slotData)> a_func, bool a_hide);

    void ToggleNPCEquipment(std::function<bool(RE::Actor* a_actor, const SlotData& a_slotData)> a_func);
	void ToggleAllEquipment(std::function<bool(RE::Actor* a_actor, const SlotData& a_slotData)> a_func);

    bool ToggleActorHeadParts(RE::Actor* a_actor, bool a_hide);
}
