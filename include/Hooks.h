#pragma once

namespace Hooks
{
	struct detail
	{
		static bool can_hide_on_equip(RE::Actor* a_actor, Biped a_slot);

		static void hide_armor(const RE::BipedAnim* a_biped, RE::NiAVObject* a_object, std::int32_t a_slot);
	};

	namespace Armor
	{
		struct ProcessGeometry
		{
			static void thunk(RE::BipedAnim* a_biped, RE::BSGeometry* a_object, RE::BSDismemberSkinInstance* a_dismemberInstance, std::int32_t a_slot, bool a_unk05);
			inline static REL::Relocation<decltype(thunk)> func;
		};

		struct HideShowBufferedSkin
		{
			static void thunk(RE::BipedAnim* a_biped, RE::NiAVObject* a_object, std::int32_t a_slot, bool a_unk04);
			static inline REL::Relocation<decltype(thunk)> func;
		};

		inline void Install();
	}

	namespace Weapon
	{
		struct LoadAndAttachAddOn
		{
			static RE::NiAVObject* thunk(RE::TESModel* a_model, RE::BIPED_OBJECT a_slot, RE::TESObjectREFR* a_ref, RE::BSTSmartPointer<RE::BipedAnim>& a_biped, RE::NiAVObject* a_root3D);
			static inline REL::Relocation<decltype(thunk)> func;
		};

		inline void Install();
	}

	namespace Head
	{
		struct GetRootNode  // HEAD
		{
			static RE::NiAVObject* thunk(RE::Actor* a_actor);
			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct UpdateDismemberPartition
		{
			static void thunk(RE::BipedAnim* a_biped, RE::NiAVObject* a_geometry, std::uint32_t a_slot);
			static inline REL::Relocation<decltype(thunk)> func;
		};

		inline void Install();
	}

	void Install();
}
