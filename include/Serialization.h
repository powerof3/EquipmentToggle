#pragma once

namespace Serialization
{
	constexpr std::uint32_t kSerializationVersion = 1;
	constexpr std::uint32_t kEquipmentToggle = 'ETOG';
	constexpr std::uint32_t kAutoToggle = 'TOGG';

    class AutoToggleMap
	{
	public:
		static AutoToggleMap* GetSingleton()
		{
			static AutoToggleMap singleton;
			return &singleton;
		}

		bool Add(const RE::Actor* a_actor, Biped a_slot, bool a_toggleState);
		bool Remove(const RE::Actor* a_actor);
		std::int32_t GetToggleState(const RE::Actor* a_actor, Biped a_slot);

		void Clear();
		bool Save(SKSE::SerializationInterface* a_intfc, std::uint32_t a_type, std::uint32_t a_version);
		bool Save(SKSE::SerializationInterface* a_intfc);
		bool Load(SKSE::SerializationInterface* a_intfc);

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
		std::map<RE::FormID, std::map<Biped, bool>> _map{};
	};

	std::string DecodeTypeCode(std::uint32_t a_typeCode);

	void SaveCallback(SKSE::SerializationInterface* a_intfc);
	void LoadCallback(SKSE::SerializationInterface* a_intfc);
	void RevertCallback(SKSE::SerializationInterface* a_intfc);
}
