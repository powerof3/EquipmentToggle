#include "Settings.h"

bool Toggle::CanDoToggle() const
{
	return toggle != Type::kDisabled;
}

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

bool SlotData::ContainsHeadSlots() const
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
	nlohmann::json json = nlohmann::json::parse(ifs, nullptr, true, true);

	logger::info("{:*^30}", "SLOT DATA");

	LoadSettingsFromJSON_Impl(json, "armors");
	LoadSettingsFromJSON_Impl(json, "weapons");

	ifs.close();
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
			try {
				hotKey.key = j_hotKey.at("key").get<Key>();
			} catch (...) {}
			try {
				hotKey.type.toggle = j_hotKey.at("type").get<Toggle::Type>();
			} catch (...) {}

			logger::info("	Key : {}", hotKey.key);
			logger::info("		toggle type : {}", stl::to_underlying(hotKey.type.toggle));
		} catch (...) {
			logger::info("	Key : not found");
		}

		SlotData::Hide hide;
		try {
			auto& j_hide = equipment.at("hide");
			try {
				hide.equipped.toggle = j_hide.at("whenEquipped").get<Toggle::Type>();
			} catch (...) {}
			try {
				hide.home.toggle = j_hide.at("atHome").get<Toggle::Type>();
			} catch (...) {}
			try {
				hide.dialogue.toggle = j_hide.at("duringDialogue").get<Toggle::Type>();
			} catch (...) {}

			logger::info("	Hide");
			logger::info("		whenEquipped : {}", stl::to_underlying(hide.equipped.toggle));
			logger::info("		atHome : {}", stl::to_underlying(hide.home.toggle));
			logger::info("		duringDialogue : {}", stl::to_underlying(hide.dialogue.toggle));
		} catch (...) {
			logger::info("	Hide : settings not found");
		}

		SlotData::Unhide unhide;
		try {
			auto& j_hide = equipment.at("unhide");
			try {
				unhide.combat.toggle = j_hide.at("duringCombat").get<Toggle::Type>();
			} catch (...) {}
			try {
				unhide.weaponDraw.toggle = j_hide.at("onWeaponDraw").get<Toggle::Type>();
			} catch (...) {}

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
				case 40:
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
