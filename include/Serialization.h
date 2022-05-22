#pragma once

namespace Serialization
{
	constexpr std::uint32_t kSerializationVersion = 1;
	constexpr std::string_view folderPath = "Data/EquipmentToggle/SlotData/"sv;
    constexpr std::string_view filePath = "Data/EquipmentToggle/SlotData/{}.json"sv;

	class AutoToggleMap
	{
	public:
		struct ToggleState
		{
			std::optional<Slot::State> firstPerson{ std::nullopt };
			std::optional<Slot::State> thirdPerson{ std::nullopt };
		};

		static AutoToggleMap* GetSingleton()
		{
			static AutoToggleMap singleton;
			return &singleton;
		}

		std::optional<Slot::State> GetToggleState(RE::Actor* a_actor, Biped a_slot, bool a_firstPerson);
	    void Add(RE::Actor* a_actor, Biped a_slot, Slot::State a_toggleState, bool a_firstPerson);
		bool Remove(RE::Actor* a_actor);

		bool Save(nlohmann::json& a_intfc);
		bool Load(const nlohmann::json& a_intfc);
		void Clear();

	protected:
		AutoToggleMap() = default;
		AutoToggleMap(const AutoToggleMap&) = delete;
		AutoToggleMap(AutoToggleMap&&) = delete;
		~AutoToggleMap() = default;

		AutoToggleMap& operator=(const AutoToggleMap&) = delete;
		AutoToggleMap& operator=(AutoToggleMap&&) = delete;

		using Lock = std::recursive_mutex;
		using Locker = std::lock_guard<Lock>;

		mutable Lock _lock{};
		std::map<RE::RefHandle, std::map<Biped, ToggleState>> _map{};
	};

	void Save(const std::string& a_savePath);
	void Load(const std::string& a_savePath);
	void Delete(const std::string& a_savePath);

    void ClearUnreferencedSaveData();

	void SetToggleState(RE::Actor* a_actor, Biped a_slot, Slot::State a_state, bool a_firstPerson);
	Slot::State GetToggleState(RE::Actor* a_actor, Biped a_slot, bool a_firstPerson);
}
