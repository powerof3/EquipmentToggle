#pragma once

namespace Events
{
	using EventResult = RE::BSEventNotifyControl;

	class Manager final :
		public RE::BSTEventSink<RE::TESCombatEvent>,
		public RE::BSTEventSink<RE::BGSActorCellEvent>,
		public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		static Manager* GetSingleton()
		{
			static Manager singleton;
			return &singleton;
		}

		static void Register()
		{
			const auto settings = Settings::GetSingleton();
			if (settings->unhideDuringCombat) {
				if (settings->autoToggleType != Settings::ToggleType::kFollowerOnly) {
					stl::write_vfunc<RE::PlayerCharacter, 0x0E3, PlayerCombat>();
				}
			    if (settings->autoToggleType != Settings::ToggleType::kPlayerOnly) {
					register_combat_event();
				}
			}
			if (settings->hideAtHome) {
				register_cell_change_event();
			}
			if (settings->hotkeyToggleType != Settings::ToggleType::kDisabled) {
				register_input_event();
			}
		}

	    EventResult ProcessEvent(const RE::TESCombatEvent* evn, RE::BSTEventSource<RE::TESCombatEvent>*) override;
		EventResult ProcessEvent(const RE::BGSActorCellEvent* a_evn, RE::BSTEventSource<RE::BGSActorCellEvent>*) override;
		EventResult ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) override;

	private:
		struct PlayerCombat
		{
			static bool thunk(RE::PlayerCharacter* a_this);
			static inline REL::Relocation<decltype(thunk)> func;
		};

	    static void register_combat_event()
		{
			if (const auto scripts = RE::ScriptEventSourceHolder::GetSingleton()) {
				scripts->AddEventSink<RE::TESCombatEvent>(GetSingleton());
			}
		}

		static void register_input_event()
		{
			if (const auto inputMgr = RE::BSInputDeviceManager::GetSingleton()) {
				inputMgr->AddEventSink(GetSingleton());
			}
		}

		static void register_cell_change_event()
		{
			if (const auto player = RE::PlayerCharacter::GetSingleton()) {
				player->AddEventSink<RE::BGSActorCellEvent>(GetSingleton());
			}
		}

		Manager() = default;
		Manager(const Manager&) = delete;
		Manager(Manager&&) = delete;

		~Manager() override = default;

		Manager& operator=(const Manager&) = delete;
		Manager& operator=(Manager&&) = delete;

		std::atomic_bool playerInHouse{ false };
		std::atomic_bool playerInCombat{ false };
	};
}
