#pragma once

#include "Graphics.h"
#include "Serialization.h"
#include "Settings.h"

namespace Hooks
{
	struct detail
	{
		static bool get_equipped_and_visible_slot(RE::Actor* a_actor, Biped a_slot)
		{
			bool result = false;

			Settings::GetSingleton()->ForEachSlot([&](const SlotData& a_slotData) {
				auto& [hotKey, hide, unhide, slots] = a_slotData;
				if (slots.contains(a_slot) && hide.equipped.CanDoToggle(a_actor) && Serialization::GetToggleState(a_actor, a_slot)) {
					result = true;
					return false;
				}
				return true;
			});

			return result;
		}

		static void hide_armor(const RE::BipedAnim* a_biped, RE::NiAVObject* a_object, std::int32_t a_slot)
		{
			const auto ref = a_biped ? a_biped->actorRef.get() : RE::TESObjectREFRPtr();
			const auto actor = ref ? ref->As<RE::Actor>() : nullptr;

			if (actor) {
				const auto slot = static_cast<Biped>(a_slot);
				if (get_equipped_and_visible_slot(actor, slot)) {
					a_object->CullNode(true);
				}
			}
		}
	};

	namespace Armor
	{
		struct ProcessGeometry
		{
			static void thunk(RE::BipedAnim* a_biped, RE::BSGeometry* a_object, RE::BSDismemberSkinInstance* a_dismemberInstance, std::int32_t a_slot, bool a_unk05)
			{
				func(a_biped, a_object, a_dismemberInstance, a_slot, a_unk05);

				if (a_biped && a_object) {
					detail::hide_armor(a_biped, a_object, a_slot);
				}
			}
			inline static REL::Relocation<decltype(thunk)> func;
		};

		struct HideShowBufferedSkin
		{
			static void thunk(RE::BipedAnim* a_biped, RE::NiAVObject* a_object, std::int32_t a_slot, bool a_unk04)
			{
				func(a_biped, a_object, a_slot, a_unk04);

				if (a_biped && a_object) {
					detail::hide_armor(a_biped, a_object, a_slot);
				}
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		inline void Install()
		{
			REL::Relocation<std::uintptr_t> processAttachedGeometry{ RELOCATION_ID(15535, 15712), OFFSET(0x79A, 0x72F) };  //armor
			stl::write_thunk_call<ProcessGeometry>(processAttachedGeometry.address());

			REL::Relocation<std::uintptr_t> loadBipedParts{ RELOCATION_ID(15501, 15678), 0x1EA };  //armor 2
			stl::write_thunk_call<HideShowBufferedSkin>(loadBipedParts.address());
		}
	}

	namespace Weapon
	{
		struct AttachWeaponModelToActor
		{
			static RE::NiAVObject* thunk(RE::TESModel* a_model, std::int32_t a_slot, RE::TESObjectREFR* a_ref, RE::BipedAnim** a_biped, RE::NiAVObject* a_root3D)
			{
				const auto object = func(a_model, a_slot, a_ref, a_biped, a_root3D);

				if (a_ref && object) {
					if (const auto actor = a_ref->As<RE::Actor>(); actor) {
						const auto slot = static_cast<Biped>(a_slot);
						if (detail::get_equipped_and_visible_slot(actor, slot)) {
							object->CullNode(true);
						}
					}
				}

				return object;
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(15506, 15683) };

			stl::write_thunk_call<AttachWeaponModelToActor>(target.address() + OFFSET(0x17F, 0x2B1));
			stl::write_thunk_call<AttachWeaponModelToActor>(target.address() + OFFSET(0x1D0, 0x2FA));
		}
	}

	namespace Head
	{
		struct GetRootNode  // HEAD
		{
			static RE::NiAVObject* thunk(RE::Actor* a_actor)
			{
				const auto root = func(a_actor);
				if (a_actor && root && Graphics::ToggleActorHeadParts(a_actor, true)) {
					return nullptr;
				}

				return root;
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct UpdateDismemberPartition
		{
			static void thunk(RE::BipedAnim* a_biped, RE::NiAVObject* a_geometry, std::uint32_t a_slot)
			{
				const auto ref = a_biped ? a_biped->actorRef.get() : RE::NiPointer<RE::TESObjectREFR>();
				const auto actor = ref ? ref->As<RE::Actor>() : nullptr;

				if (a_biped && actor) {
					const auto slot = static_cast<Biped>(a_slot);
					if (detail::get_equipped_and_visible_slot(actor, slot)) {
						return;
					}
				}

				func(a_biped, a_geometry, a_slot);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		inline void Install()
		{
			REL::Relocation<std::uintptr_t> UpdateHairAndHead{ RELOCATION_ID(24220, 24724), OFFSET(0x1A, 0x19) };
			stl::write_thunk_call<GetRootNode>(UpdateHairAndHead.address());

			REL::Relocation<std::uintptr_t> ProcessArmorDismemberment{ RELOCATION_ID(15539, 15715), OFFSET(0x70, 0x2C2) };  //everything got inlined in AE
			stl::write_thunk_call<UpdateDismemberPartition>(ProcessArmorDismemberment.address());
		}
	}

	void Install();
}
