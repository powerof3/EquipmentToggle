#include "Hooks.h"
#include "Settings.h"

void Hooks::Install()
{
	if (Settings::GetSingleton()->autoToggleType == Settings::ToggleType::kDisabled) {
	    return;
	}

    if (!armorSlots.empty()) {
		Armor::Install();
		logger::info("Installed armor hook");
	}

	if (!weaponSlots.empty()) {
		Weapon::Install();
		logger::info("Installed weapons hook");
	}

	if (auto [contains, autoToggle, hidden] = detail::get_slot_info(armorSlots, [&](auto a_slot) {
			return a_slot == Biped::kHead || a_slot == Biped::kHair || a_slot == Biped::kLongHair || a_slot == Biped::kEars || a_slot == Biped::kCirclet || a_slot == Biped::kDecapitateHead;
		});
		contains) {
		Head::Install();
		logger::info("Installed head/hair hook");
	}
}
