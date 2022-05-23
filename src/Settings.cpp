#include "Settings.h"

bool Toggle::CanDoToggle() const
{
	return toggle != Type::kDisabled;
}

bool Toggle::CanDoToggle(RE::Actor* a_actor) const
{
	const auto is_follower = [](RE::Actor* a_actor) {
		if (a_actor->HasKeywordString(NPC)) {
			if (a_actor->IsPlayerTeammate()) {
				return true;
			}
			if (a_actor->IsCommandedActor()) {
				auto commander = a_actor->GetCommandingActor();
				return commander && commander->IsPlayerRef();
			}
		}
		return false;
	};

	switch (toggle) {
	case Type::kPlayerOnly:
		return a_actor->IsPlayerRef();
	case Type::kFollowerOnly:
		return is_follower(a_actor);
	case Type::kPlayerAndFollower:
		return a_actor->IsPlayerRef() || is_follower(a_actor);
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

bool SlotData::ContainsHeadSlots() const
{
	return std::ranges::any_of(slots, [](const auto& slot) {
		return slot == Biped::kHead || slot == Biped::kHair || slot == Biped::kLongHair || slot == Biped::kEars;
	});
}

Settings* Settings::GetSingleton()
{
	static Settings singleton;
	return std::addressof(singleton);
}

bool Settings::LoadSettings()
{
	bool result = true;

	constexpr auto path = L"Data/EquipmentToggle/Config.json";

	std::ifstream ifs(path);
	if (ifs.is_open()) {
		try {
			const auto json = nlohmann::json::parse(ifs, nullptr, true, true);

			logger::info("{:*^30}", "SLOT DATA");

			LoadSettingsFromJSON_Impl(json, "armors");
			LoadSettingsFromJSON_Impl(json, "weapons");

		} catch (nlohmann::json::exception& e) {
		    logger::critical("Unable to parse config file! Use a JSON validator to fix any mistakes");
			logger::critical("	Error : {}", e.what());

			result = false;
		}
	}
	ifs.close();

	return result;
}

void Settings::LoadSettingsFromJSON_Impl(const nlohmann::json& a_json, const std::string& a_type)
{
	if (!a_json.contains(a_type) || a_json[a_type].empty() || !a_json[a_type].is_array()) {
		return;
	}

	logger::info("{}", a_type);
	for (auto& equipment : a_json[a_type]) {
		SlotData::HotKey hotKey;
		try {
			auto& j_hotKey = equipment.at("hotKey");

			load_json_setting(j_hotKey, hotKey.key, "key");
			load_json_setting(j_hotKey, hotKey.type.toggle, "type");

			logger::info("	Key : {}", hotKey.key);
			logger::info("		toggle type : {}", stl::to_underlying(hotKey.type.toggle));
		} catch (...) {
			logger::info("	Key : not found");
		}

		SlotData::Hide hide;
		try {
			auto& j_hide = equipment.at("hide");

			load_json_setting(j_hide, hide.equipped.toggle, "whenEquipped");
			load_json_setting(j_hide, hide.home.toggle, "atHome");
			load_json_setting(j_hide, hide.dialogue.toggle, "duringDialogue");
			load_json_setting(j_hide, hide.weaponDraw.toggle, "onWeaponDraw");

			logger::info("	Hide");
			logger::info("		whenEquipped : {}", stl::to_underlying(hide.equipped.toggle));
			logger::info("		atHome : {}", stl::to_underlying(hide.home.toggle));
			logger::info("		duringDialogue : {}", stl::to_underlying(hide.dialogue.toggle));
			logger::info("		onWeaponDraw : {}", stl::to_underlying(hide.weaponDraw.toggle));
		} catch (...) {
			logger::info("	Hide : settings not found");
		}

		SlotData::Unhide unhide;
		try {
			auto& j_unhide = equipment.at("unhide");

			load_json_setting(j_unhide, unhide.combat.toggle, "duringCombat");
			load_json_setting(j_unhide, unhide.weaponDraw.toggle, "onWeaponDraw");

			logger::info("	Unhide");
			logger::info("		duringCombat : {}", stl::to_underlying(unhide.combat.toggle));
			logger::info("		onWeaponDraw : {}", stl::to_underlying(unhide.weaponDraw.toggle));
		} catch (...) {
			logger::info("	Unhide : settings not found");
		}

		if (!equipment.contains("slots") || equipment["slots"].empty()) {
			logger::critical("	Slots : missing!");
			continue;
		}

		logger::info("	Slots");

		Slot::Set slotSet;

		if (a_type == "armors") {
			for (auto& j_slot : equipment["slots"]) {
				auto slot = j_slot.get<std::uint32_t>();
				switch (slot) {
				case 32:
				case 33:
				case 34:
				case 37:
				case 38:
					logger::info("		unreplaceable slot {}", slot);
					continue;
				default:
					break;
				}
				if (slot > 61) {
					logger::info("		invalid slot {}", slot);
					continue;
				}
				logger::info("		slot {}", slot);
				slotSet.insert(bipedMap.at(slot));
			}
		} else {
			for (auto& j_slot : equipment["slots"]) {
				logger::info("		slot {}", j_slot.get<Biped>());
				slotSet.insert(j_slot.get<Biped>());
			}
		}

		equipmentSlots.emplace_back(SlotData{ hotKey, hide, unhide, slotSet });
	}
}

void Settings::ForEachSlot(std::function<bool(const SlotData& a_slotData)> a_callback) const
{
	for (auto& slotData : equipmentSlots) {
		if (!a_callback(slotData)) {
			return;
		}
	}
}
