#pragma once

struct Toggle
{
	enum class Type : std::int32_t
	{
		kDisabled = -1,
		kPlayerOnly = 0,
		kFollowerOnly,
		kPlayerAndFollower,
		kEveryone
	};

	[[nodiscard]] bool CanDoToggle() const;
	[[nodiscard]] bool CanDoToggle(RE::Actor* a_actor) const;
	[[nodiscard]] bool CanDoPlayerToggle() const;
	[[nodiscard]] bool CanDoFollowerToggle() const;

private:
	friend class Settings;

	Type toggle{ Type::kDisabled };
};

struct SlotData
{
	bool ContainsHeadSlots() const;

	struct HotKey
	{
		Key key{ Key::kNone };
		Toggle type{};
	} hotKey;

	struct Hide
	{
		Toggle equipped{};
		Toggle home{};
		Toggle dialogue{};
		Toggle weaponDraw{};
	} hide;

	struct Unhide
	{
		Toggle combat{};
		Toggle weaponDraw{};
	} unhide;

	Slot::Set slots{};
};

class Settings
{
public:
	static Settings* GetSingleton();

	bool LoadSettings();

	void ForEachSlot(std::function<bool(const SlotData& a_slotData)> a_callback) const;

private:
	void LoadSettingsFromJSON_Impl(const nlohmann::json& a_json, const std::string& a_type);

	template <class T>
    static void load_json_setting(const nlohmann::json& a_json, T& a_setting, const std::string& a_type)
	{
		if (const auto it = a_json.find(a_type); it != a_json.end()) {
			a_setting = it->get<T>();
		}
	}

	std::vector<SlotData> equipmentSlots;
};
