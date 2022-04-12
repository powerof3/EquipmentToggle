#include "Settings.h"

Settings* Settings::GetSingleton()
{
	static Settings singleton;
	return std::addressof(singleton);
}

void Settings::LoadSettings()
{
	constexpr auto path = L"Data/SKSE/Plugins/po3_EquipmentToggle.ini";

	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path);

	detail::get_value(ini, autoToggleType, "Settings", "Auto Toggle Type", ";-1 : disabled | 0 : player only | 1 : followers | 2 : player and followers | 3 : every NPC\n");
	detail::get_value(ini, hotkeyToggleType, "Settings", "Hotkey Toggle Type", nullptr);

	detail::get_value(ini, hideWhenEquipped, "Settings", "Hide When Equipped", nullptr);
	detail::get_value(ini, unhideDuringCombat, "Settings", "Unhide During Combat", nullptr);
	detail::get_value(ini, hideAtHome, "Settings", "Hide At Home", nullptr);

	get_data_from_ini(ini, "Armor Slots", "Armor", armorSlots, DATA::kArmor);
	get_data_from_ini(ini, "Weapon Slots", "Weapon", weaponSlots, DATA::kWeapon);

	(void)ini.SaveFile(path);
}

bool Settings::CanToggleEquipment(RE::Actor* a_actor) const
{
	switch (autoToggleType) {
	case ToggleType::kPlayerOnly:
		return a_actor->IsPlayerRef();
	case ToggleType::kFollowerOnly:
		return a_actor->IsPlayerTeammate() && a_actor->HasKeywordString(NPC);
	case ToggleType::kPlayerAndFollower:
		return a_actor->IsPlayerRef() || a_actor->IsPlayerTeammate() && a_actor->HasKeywordString(NPC);
	case ToggleType::kEveryone:
		return a_actor->HasKeywordString(NPC);
	default:
		return false;
	}
}

std::optional<SlotKeyData> Settings::parse_data(const std::string& a_value, const DATA a_type) const
{
	auto sections = string::split(a_value, "|");
	if (sections.empty()) {
		return std::nullopt;
	}

	for (auto& str : sections) {
		string::trim(str);
	}

	auto key = Key::kNone;
	try {
		key = string::lexical_cast<Key>(sections.at(0));
		logger::info("		Key : {}", key);
	} catch (...) {
	}

	if (key == Key::kNone) {
		logger::info("		Hotkey toggle disabled");
	}

	auto setAutoToggled = true;
	try {
		const auto& toggle = sections.at(1);
		setAutoToggled = toggle != "false";
	} catch (...) {
	}
	logger::info("		Auto Toggle : {}", setAutoToggled);

	if (sections.size() < 2) {
		logger::error("		Slots : missing!");
		return std::nullopt;
	}

	std::set<Biped> slotSet;
	if (const auto& slots = sections.at(2); !slots.empty()) {
		const auto vec = string::split(slots, " , ");
		if (a_type == DATA::kArmor) {
			for (auto& slotStr : vec) {
				auto slot = string::lexical_cast<std::uint32_t>(slotStr);
				switch (slot) {
				case 32:
				case 33:
				case 34:
				case 37:
				case 38:
				case 40:
					logger::info("			Armor : unreplaceable slot {}", slot);
					continue;
				default:
					break;
				}
				if (slot > 61) {
					logger::info("			Armor : invalid slot {}", slot);
					continue;
				}
				logger::info("			Armor : slot {}", slot);
				slotSet.insert(bipedMap.at(slot));
			}
		} else {
			for (auto& slotStr : vec) {
				logger::info("			Weapon : slot {}", slotStr);
				slotSet.insert(static_cast<Biped>(string::lexical_cast<std::uint32_t>(slotStr)));
			}
		}
	}

	if (slotSet.empty()) {
		logger::error("		Slots : missing!");
		return std::nullopt;
	}

	return std::make_pair(key, std::make_pair(hideWhenEquipped && setAutoToggled, slotSet));
}

void Settings::get_data_from_ini(const CSimpleIniA& ini, const char* a_section, const char* a_type, SlotKeyVec& a_INIDataVec, DATA a_dataType) const
{
	CSimpleIniA::TNamesDepend values;
	ini.GetAllValues(a_section, a_type, values);
	values.sort(CSimpleIniA::Entry::LoadOrder());

	logger::info("	{} entries found : {}", a_type, values.size());

	a_INIDataVec.reserve(values.size());
	for (const auto& value : values) {
		if (auto data = parse_data(value.pItem, a_dataType)) {
			a_INIDataVec.emplace_back(data.value());
		}
	}
}
