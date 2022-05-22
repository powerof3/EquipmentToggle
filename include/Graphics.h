#pragma once

struct SlotData;

class Graphics
{
public:
    static SlotData GetHeadSlots();

	static void ToggleActorEquipment(RE::Actor* a_actor, std::function<bool(const SlotData& a_slotData)> a_func, Slot::State a_state);

	static void ToggleFollowerEquipment(std::function<bool(const SlotData& a_slotData)> a_func, Slot::State a_state);

	static void ToggleNPCEquipment(std::function<bool(RE::Actor* a_actor, const SlotData& a_slotData)> a_func);
	static void ToggleAllEquipment(std::function<bool(RE::Actor* a_actor, const SlotData& a_slotData)> a_func);

	static bool ToggleActorHeadParts(RE::Actor* a_actor, Slot::State a_state);

	static bool IsFirstPerson(const RE::Actor* a_actor, const RE::NiAVObject* a_root);
	static bool IsFirstPerson(const RE::Actor* a_actor, const RE::BipedAnim* a_biped);

private:
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

		static void toggle_partition(RE::BSGeometry& a_shape, const RE::TESObjectARMA& a_arma, bool a_hide);
        static void toggle_partition(const RE::BSGeometry& a_shape, const RE::TESRace& a_race, bool a_hide);
        static void toggle_partition(RE::BSGeometry& a_shape, bool a_hide);

        static void toggle_extra_parts(const RE::BSTArray<RE::BGSHeadPart*>& a_parts, RE::NiAVObject& a_root, const RE::TESObjectARMA& a_arma, bool a_hide);
        static void toggle_extra_parts(const RE::BSTArray<RE::BGSHeadPart*>& a_parts, RE::NiAVObject& a_root, const RE::TESRace& a_race, bool a_hide);
        static void toggle_extra_parts(const RE::BSTArray<RE::BGSHeadPart*>& a_parts, RE::NiAVObject& a_root, bool a_hide);

		static void update_head_part(RE::Actor* a_actor, RE::NiAVObject* a_root, RE::TESObjectARMA* a_arma, HeadPart a_type, bool a_hide);
    };

	static void toggle_slots(RE::Actor* a_actor, const RE::BSTSmartPointer<RE::BipedAnim>& a_biped, RE::NiAVObject* a_root, const Slot::Set& a_slots, Slot::State a_state);

	static void toggle_slots(RE::Actor* a_actor, const RE::BSTSmartPointer<RE::BipedAnim>& a_biped, RE::NiAVObject* a_root, const Slot::Set& a_slots);
};
