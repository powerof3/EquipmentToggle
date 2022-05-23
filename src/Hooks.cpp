#include "Hooks.h"

#include "Graphics.h"
#include "Serialization.h"
#include "Settings.h"

bool Hooks::detail::can_hide_on_equip(RE::Actor* a_actor, Biped a_slot)
{
	bool result = false;

	Settings::GetSingleton()->ForEachSlot([&](const SlotData& a_slotData) {
		auto& [hotKey, hide, unhide, slots] = a_slotData;
		if (slots.contains(a_slot) && hide.equipped.CanDoToggle(a_actor)) {
			result = true;
		}
		return !result;
	});

	return result;
}

void Hooks::detail::hide_armor(const RE::BipedAnim* a_biped, RE::NiAVObject* a_object, std::int32_t a_slot)
{
	if (a_biped) {
		const auto ref = a_biped->actorRef.get();
		const auto actor = ref ? ref->As<RE::Actor>() : nullptr;

		if (actor) {
			const auto slot = static_cast<Biped>(a_slot);
			const auto isFP = Graphics::IsFirstPerson(actor, a_biped);
			if (can_hide_on_equip(actor, slot) && Serialization::GetToggleState(actor, slot, isFP) == Slot::State::kHide) {
				a_object->CullNode(true);
			}
		}
	}
}

void Hooks::Armor::ProcessGeometry::thunk(RE::BipedAnim* a_biped, RE::BSGeometry* a_object, RE::BSDismemberSkinInstance* a_dismemberInstance, std::int32_t a_slot, bool a_unk05)
{
	func(a_biped, a_object, a_dismemberInstance, a_slot, a_unk05);

	if (a_biped && a_object) {
		detail::hide_armor(a_biped, a_object, a_slot);
	}
}

void Hooks::Armor::HideShowBufferedSkin::thunk(RE::BipedAnim* a_biped, RE::NiAVObject* a_object, std::int32_t a_slot, bool a_unk04)
{
	func(a_biped, a_object, a_slot, a_unk04);

	if (a_biped && a_object) {
		detail::hide_armor(a_biped, a_object, a_slot);
	}
}

void Hooks::Armor::Install()
{
	REL::Relocation<std::uintptr_t> processAttachedGeometry{ RELOCATION_ID(15535, 15712), OFFSET(0x79A, 0x72F) };  //armor
	stl::write_thunk_call<ProcessGeometry>(processAttachedGeometry.address());

	REL::Relocation<std::uintptr_t> loadBipedParts{ RELOCATION_ID(15501, 15678), 0x1EA };  //armor 2
	stl::write_thunk_call<HideShowBufferedSkin>(loadBipedParts.address());
}

RE::NiAVObject* Hooks::Weapon::LoadAndAttachAddOn::thunk(RE::TESModel* a_model, RE::BIPED_OBJECT a_slot, RE::TESObjectREFR* a_ref, RE::BSTSmartPointer<RE::BipedAnim>& a_biped, RE::NiAVObject* a_root3D)
{
	const auto object = func(a_model, a_slot, a_ref, a_biped, a_root3D);

	if (a_biped && a_ref && object) {
		if (const auto actor = a_ref->As<RE::Actor>(); actor) {
			const auto isFP = Graphics::IsFirstPerson(actor, a_biped.get());
			if (detail::can_hide_on_equip(actor, a_slot) && Serialization::GetToggleState(actor, a_slot, isFP) == Slot::State::kHide) {
				object->CullNode(true);
			}
		}
	}

	return object;
}

void Hooks::Weapon::Install()
{
	std::array targets{
		std::make_pair(RELOCATION_ID(15506, 15683), OFFSET(0x17F, 0x2B1)),  // BipedAnim::AttachBipedWeapon
		std::make_pair(RELOCATION_ID(15506, 15683), OFFSET(0x1D0, 0x2FA)),  // BipedAnim::AttachBipedWeapon

		std::make_pair(RELOCATION_ID(15511, 15688), OFFSET(0x141, 0x199)),  // BipedAnim::AttachBipedAmmo (Quiver)

		std::make_pair(RELOCATION_ID(15514, 15691), OFFSET(0x185, 0x24C)),  // BipedAnim::AttachBipedTorch
		std::make_pair(RELOCATION_ID(15514, 15691), OFFSET(0x282, 0x342)),  // BipedAnim::AttachBipedTorch
	};

	for (const auto& [id, offset] : targets) {
		REL::Relocation<std::uintptr_t> target{ id, offset };
		stl::write_thunk_call<LoadAndAttachAddOn>(target.address());
	}
}

RE::NiAVObject* Hooks::Head::GetRootNode::thunk(RE::Actor* a_actor)
{
	const auto root = func(a_actor);
	if (a_actor && root && Graphics::ToggleActorHeadParts(a_actor, Slot::State::kHide)) {
		return nullptr;
	}

	return root;
}

void Hooks::Head::UpdateDismemberPartition::thunk(RE::BipedAnim* a_biped, RE::NiAVObject* a_geometry, std::uint32_t a_slot)
{
	if (a_biped) {
		const auto ref = a_biped->actorRef.get();
		const auto actor = ref ? ref->As<RE::Actor>() : nullptr;

		if (actor) {
			const auto slot = static_cast<Biped>(a_slot);
			const auto isFP = Graphics::IsFirstPerson(actor, a_biped);
			if (detail::can_hide_on_equip(actor, slot) && Serialization::GetToggleState(actor, slot, isFP) == Slot::State::kHide) {
				return;
			}
		}
	}

	func(a_biped, a_geometry, a_slot);
}

void Hooks::Head::Install()
{
	REL::Relocation<std::uintptr_t> UpdateHairAndHead{ RELOCATION_ID(24220, 24724), OFFSET(0x1A, 0x19) };
	stl::write_thunk_call<GetRootNode>(UpdateHairAndHead.address());

	REL::Relocation<std::uintptr_t> ProcessArmorDismemberment{ RELOCATION_ID(15539, 15715), OFFSET(0x70, 0x2C2) };  //everything got inlined in AE
	stl::write_thunk_call<UpdateDismemberPartition>(ProcessArmorDismemberment.address());
}

void Hooks::Install()
{
	logger::info("{:*^30}", "HOOKS");

	bool armor = false;
	bool weapons = false;
	bool head = false;

	Settings::GetSingleton()->ForEachSlot([&](const SlotData& a_slotData) {
		if (armor && head && weapons) {
			return false;
		}

		auto& [hotKey, hide, unhide, slots] = a_slotData;

		if (hide.equipped.CanDoToggle()) {
			if (!armor && !slots.empty()) {
				armor = true;
				Armor::Install();
				logger::info("Installed armor hook");
			}

			if (!head && a_slotData.ContainsHeadSlots()) {
				head = true;
				Head::Install();
				logger::info("Installed head/hair hook");
			}

			if (!weapons && !slots.empty()) {
				weapons = true;
				Weapon::Install();
				logger::info("Installed weapons hook");
			}
		}

		return true;
	});
}
