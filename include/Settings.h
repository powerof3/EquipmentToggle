#pragma once

class Settings
{
public:
	enum class ToggleType : std::int32_t
	{
		kDisabled = -1,
	    kPlayerOnly = 0,
		kFollowerOnly = 1,
		kPlayerAndFollower = 2,
		kEveryone = 3
	};

    static Settings* GetSingleton();

	void LoadSettings();

    bool CanToggleEquipment(RE::Actor* a_actor) const;

    ToggleType autoToggleType{ ToggleType::kPlayerOnly };
	ToggleType hotkeyToggleType{ ToggleType::kPlayerOnly };

	bool hideWhenEquipped{ true };
	bool hideAtHome{ false };
	bool hideWhenSpeaking{ false };

	bool unhideDuringCombat{ false };
	bool unhideDuringWeaponDraw{ false };

private:
	enum class DATA
	{
		kArmor = 0,
		kWeapon = 1
	};

	struct detail
	{
		template <class T>
		static void get_value(CSimpleIniA& a_ini, T& a_value, const char* a_section, const char* a_key, const char* a_comment)
		{
			if constexpr (std::is_same_v<bool, T>) {
				a_value = a_ini.GetBoolValue(a_section, a_key, a_value);
				a_ini.SetBoolValue(a_section, a_key, a_value, a_comment);
			} else if constexpr (std::is_floating_point_v<T>) {
				a_value = static_cast<T>(a_ini.GetDoubleValue(a_section, a_key, a_value));
				a_ini.SetDoubleValue(a_section, a_key, a_value, a_comment);
			} else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
				a_value = string::lexical_cast<T>(a_ini.GetValue(a_section, a_key, std::to_string(std::to_underlying(a_value)).c_str()));
				a_ini.SetValue(a_section, a_key, std::to_string(std::to_underlying(a_value)).c_str(), a_comment);
			} else {
				a_value = a_ini.GetValue(a_section, a_key, a_value.c_str());
				a_ini.SetValue(a_section, a_key, a_value.c_str(), a_comment);
			}
		}
	};

	std::optional<SlotKeyData> parse_data(const std::string& a_value, DATA a_type) const;

	void get_data_from_ini(const CSimpleIniA& ini, const char* a_section, const char* a_type, SlotKeyVec& a_INIDataVec, DATA a_dataType) const;
};
