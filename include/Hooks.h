#pragma once

#include "Graphics.h"
#include "Settings.h"

namespace Hooks
{
	struct detail
	{
		static std::tuple<bool, bool, Biped> get_slot_info(const SlotKeyVec& a_vec, std::function<bool(Biped a_slot)> a_func)
		{
			for (const auto& data : a_vec | std::views::values) {
				const auto& [autoToggle, slots] = data;
				for (auto& slot : slots) {
					if (a_func(slot)) {
						return { true, autoToggle, slot };
					}
				}
			}
			return { false, true, Biped::kNone };
		}

		static void hide_armor(const RE::BipedAnim* a_biped, RE::NiAVObject* a_object, std::int32_t a_slot)
		{
			if (const auto ref = a_biped->actorRef.get(); ref) {
				const auto actor = ref->As<RE::Actor>();
				if (actor && Settings::GetSingleton()->CanToggleEquipment(actor)) {
					auto slot = static_cast<Biped>(a_slot);
					if (const auto root = actor->GetCurrent3D(); root) {
						auto [contains, autoToggle, matchingSlot] = get_slot_info(armorSlots, [&](const auto& b_slot) {
							return b_slot == slot;
						});
						if (contains && Graphics::Slot::get_toggle_data(actor, slot, autoToggle)) {
							a_object->CullNode(true);
						}
					}
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

				if (a_ref && a_root3D && object) {
					const auto actor = a_ref->As<RE::Actor>();
					if (actor && Settings::GetSingleton()->CanToggleEquipment(actor)) {
						auto slot = static_cast<Biped>(a_slot);
						auto [contains, autoToggle, matchingSlot] = detail::get_slot_info(weaponSlots, [&](const auto& b_slot) {
							return b_slot == slot;
						});
						if (contains && Graphics::Slot::get_toggle_data(actor, slot, autoToggle)) {
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
			using Type = Settings::ToggleType;

			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(15506, 15683) };

			switch (Settings::GetSingleton()->autoToggleType) {
			case Type::kPlayerOnly:
				stl::write_thunk_call<AttachWeaponModelToActor>(target.address() + OFFSET(0x17F, 0x2B1));
				break;
			case Type::kFollowerOnly:
				stl::write_thunk_call<AttachWeaponModelToActor>(target.address() + OFFSET(0x1D0, 0x2FA));
				break;
			case Type::kPlayerAndFollower:
			case Type::kEveryone:
				{
					stl::write_thunk_call<AttachWeaponModelToActor>(target.address() + OFFSET(0x17F, 0x2B1));
					stl::write_thunk_call<AttachWeaponModelToActor>(target.address() + OFFSET(0x1D0, 0x2FA));
				}
				break;
			default:
				break;
			}
		}
	}

	namespace Head
	{
		struct GetRootNode // HEAD
		{
			static RE::NiAVObject* thunk(RE::Actor* a_actor)
			{
				const auto root = func(a_actor);

				if (a_actor && root && Settings::GetSingleton()->CanToggleEquipment(a_actor)) {
					auto [autoToggle, slots] = Graphics::Slot::get_head_slots();
					if (!slots.empty() && Graphics::Slot::get_toggle_data(a_actor, Biped::kHead, autoToggle)) {
						Graphics::Slot::ToggleActorHeadParts(a_actor, true);
					    return nullptr;
					}
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

				if (a_biped && actor && Settings::GetSingleton()->CanToggleEquipment(actor)) {
					if (const auto root = a_biped->root; root) {
						auto slot = static_cast<Biped>(a_slot);
						auto [contains, autoToggle, matchingSlot] = detail::get_slot_info(armorSlots, [&slot](const auto& b_slot) {
							return slot = b_slot;
						});
					    if (contains && Graphics::Slot::get_toggle_data(actor, slot, autoToggle)) {
							return;
						}
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
