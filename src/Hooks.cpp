#include "Hooks.h"

void Hooks::Install()
{
	logger::info("{:*^30}", "HOOKS");

    bool armor = false;
	bool weapons = false;
	bool head = false;

	Settings::GetSingleton()->ForEachArmorSlot([&](const SlotData& a_slotData) {
		if (armor && head) {
			return false;
		}

		auto& [hotKey, hide, unhide, slots] = a_slotData;

		if (hide.equipped.toggle != Toggle::Type::kDisabled) {
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
		}

		return true;
	});

	Settings::GetSingleton()->ForEachWeaponSlot([&](const SlotData& a_slotData) {
		if (weapons) {
			return false;
		}

		auto& [hotKey, hide, unhide, slots] = a_slotData;

		if (hide.equipped.toggle != Toggle::Type::kDisabled) {
			if (!weapons && !slots.empty()) {
				weapons = true;
				Weapon::Install();
				logger::info("Installed weapons hook");
			}
		}

		return true;
	});
}
