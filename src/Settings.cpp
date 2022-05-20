#include "Settings.h"

bool Toggle::CanDoToggle(RE::Actor* a_actor) const
{
	switch (toggle) {
	case Type::kPlayerOnly:
		return a_actor->IsPlayerRef();
	case Type::kFollowerOnly:
		return a_actor->IsPlayerTeammate() && a_actor->HasKeywordString(NPC);
	case Type::kPlayerAndFollower:
		return a_actor->IsPlayerRef() || a_actor->IsPlayerTeammate() && a_actor->HasKeywordString(NPC);
	case Type::kEveryone:
		return a_actor->HasKeywordString(NPC);
	default:
		return false;
	}
}

bool Toggle::CanDoPlayerToggle() const
{
	if (toggle == Type::kDisabled) {
		return false;
	}

	return toggle != Type::kFollowerOnly;
}

bool Toggle::CanDoFollowerToggle() const
{
	if (toggle == Type::kDisabled) {
		return false;
	}

	return toggle != Type::kPlayerOnly;
}

bool SlotData::ContainsHeadSlots()
{
	return std::ranges::any_of(slots, [](const auto& slot) {
		return slot == Biped::kHead || slot == Biped::kHair || slot == Biped::kLongHair || slot == Biped::kEars || slot == Biped::kDecapitateHead;
	});
}

Settings* Settings::GetSingleton()
{
	static Settings singleton;
	return std::addressof(singleton);
}

void Settings::LoadSettings()
{
	constexpr auto path = L"Data/SKSE/Plugins/po3_EquipmentToggle.json";

	std::ifstream ifs(path);
	nlohmann::json json = nlohmann::json::parse(ifs);

	LoadSettingsFromJSON_Impl(json, "armors");
	LoadSettingsFromJSON_Impl(json, "weapons");

	ifs.close();
}

void Settings::LoadSettingsFromJSON_Impl(const nlohmann::json& a_json, const std::string& a_type)
{
	if (!a_json.contains(a_type) || a_json[a_type].empty()) {
		return;
	}

    logger::info("	{}", a_type);
    for (auto& equipment : a_json[a_type]) {
		SlotData::HotKey hotKey;
        if (equipment.contains("hotKey")) {
			auto& j_hotKey = equipment["hotKey"];
			hotKey.key = j_hotKey["key"].get<Key>();
			hotKey.type.toggle = j_hotKey["type"].get<Toggle::Type>();

            logger::info("		Key : {}", hotKey.key);
			logger::info("			Key Toggle : {}", stl::to_underlying(hotKey.type.toggle));
		}
		SlotData::Hide hide;
        if (equipment.contains("hide")) {
			auto& j_hide = equipment["hide"];
			hide.equipped.toggle = j_hide["whenEquipped"].get<Toggle::Type>();
			hide.home.toggle = j_hide["atHome"].get<Toggle::Type>();
			hide.dialogue.toggle = j_hide["duringDialogue"].get<Toggle::Type>();

			logger::info("		Hide");
			logger::info("			whenEquipped : {}", stl::to_underlying(hide.equipped.toggle));
			logger::info("			atHome : {}", stl::to_underlying(hide.home.toggle));
			logger::info("			duringDialogue : {}", stl::to_underlying(hide.dialogue.toggle));
		}
		SlotData::Unhide unhide;
        if (equipment.contains("unhide")) {
			auto& j_unhide = equipment["unhide"];
			unhide.combat.toggle = j_unhide["duringCombat"].get<Toggle::Type>();
			unhide.weaponDraw.toggle = j_unhide["onWeaponDraw"].get<Toggle::Type>();

			logger::info("		Unhide");
			logger::info("			duringCombat : {}", stl::to_underlying(unhide.combat.toggle));
			logger::info("			onWeaponDraw : {}", stl::to_underlying(unhide.weaponDraw.toggle));
		}

        if (!equipment.contains("slots")) {
			logger::critical("		Slots : missing!");
            continue;
		}

        logger::info("		Slots");

		SlotSet slotSet;
		if (a_type == "armors") {
			for (auto& j_slot : equipment["slots"]) {
				auto slot = j_slot.get<std::uint32_t>();
				switch (slot) {
				case 32:
				case 33:
				case 34:
				case 37:
				case 38:
				case 40:
					logger::info("			unreplaceable slot {}", slot);
					continue;
				default:
					break;
				}
				if (slot > 61) {
					logger::info("			invalid slot {}", slot);
					continue;
				}
				logger::info("			slot {}", slot);
				slotSet.insert(bipedMap.at(slot));
			}
			armorSlots.emplace_back(SlotData{ hotKey, hide, unhide, slotSet });
		} else {
			for (auto& j_slot : equipment["slots"]) {
				slotSet.insert(j_slot.get<Biped>());
			}
			weaponSlots.emplace_back(SlotData{ hotKey, hide, unhide, slotSet });
		}
	}
}

void Settings::ForEachSlot(std::function<bool(const SlotData& a_slotData)> a_callback) const
{
	for (auto& slotData : armorSlots) {
		if (!a_callback(slotData)) {
			return;
		}
	}
	for (auto& slotData : weaponSlots) {
		if (!a_callback(slotData)) {
			return;
		}
	}
}
