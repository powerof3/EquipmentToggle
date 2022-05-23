#pragma once

namespace Serialization
{
	constexpr std::uint32_t kSerializationVersion = 1;
	constexpr std::string_view folderPath = "Data/EquipmentToggle/SlotData/"sv;
	constexpr std::string_view filePath = "Data/EquipmentToggle/SlotData/{}.json"sv;

	class Manager : public RE::BSTEventSink<RE::TESFormDeleteEvent>
	{
	public:
		struct ToggleState
		{
			std::optional<Slot::State> firstPerson{ std::nullopt };
			std::optional<Slot::State> thirdPerson{ std::nullopt };
		};

		static Manager* GetSingleton();

		static void Register();

		RE::BSEventNotifyControl ProcessEvent(const RE::TESFormDeleteEvent* a_event, RE::BSTEventSource<RE::TESFormDeleteEvent>*) override;

		std::optional<Slot::State> GetToggleState(RE::Actor* a_actor, Biped a_slot, bool a_firstPerson);
		void Add(RE::Actor* a_actor, Biped a_slot, Slot::State a_toggleState, bool a_firstPerson);
		bool Remove(RE::FormID a_formID);

		bool Save(nlohmann::ordered_json& a_intfc);
		bool Load(const nlohmann::ordered_json& a_intfc);
		void Clear();

		void SavePluginList(nlohmann::ordered_json& a_intfc);
		void LoadPluginList(const nlohmann::ordered_json& a_intfc);

	protected:
		using Lock = std::recursive_mutex;
		using Locker = std::lock_guard<Lock>;

		Manager() = default;
		Manager(const Manager&) = delete;
		Manager(Manager&&) = delete;
		~Manager() override = default;

		Manager& operator=(const Manager&) = delete;
		Manager& operator=(Manager&&) = delete;

		bool ResolveFormID(RE::FormID a_formIDIn, RE::FormID& a_formIDOut);

		mutable Lock _lock{};
		std::map<RE::FormID, std::map<Biped, ToggleState>> _map{};
		std::unordered_map<std::uint32_t, std::uint32_t> _savedModIndexMap{};
	};

	void Save(const std::string& a_savePath);
	void Load(const std::string& a_savePath);
	void Delete(const std::string& a_savePath);

	void ClearUnreferencedSlotData();

	void SetToggleState(RE::Actor* a_actor, Biped a_slot, Slot::State a_state, bool a_firstPerson);
	Slot::State GetToggleState(RE::Actor* a_actor, Biped a_slot, bool a_firstPerson);
}
