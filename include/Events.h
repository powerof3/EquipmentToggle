#pragma once

namespace Events
{
	using EventResult = RE::BSEventNotifyControl;

	class Manager final :
		public RE::BSTEventSink<RE::TESCombatEvent>,
		public RE::BSTEventSink<RE::BGSActorCellEvent>,
		public RE::BSTEventSink<RE::MenuOpenCloseEvent>,
		public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		static Manager* GetSingleton();

        static void Register();

        EventResult ProcessEvent(const RE::TESCombatEvent* evn, RE::BSTEventSource<RE::TESCombatEvent>*) override;
		EventResult ProcessEvent(const RE::BGSActorCellEvent* a_evn, RE::BSTEventSource<RE::BGSActorCellEvent>*) override;
		EventResult ProcessEvent(RE::InputEvent* const* a_evn, RE::BSTEventSource<RE::InputEvent*>*) override;
		EventResult ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

	private:
		struct detail
		{
			static bool is_cell_home(const RE::TESObjectCELL* a_cell)
			{
				if (a_cell->IsInteriorCell()) {
					if (const auto loc = a_cell->GetLocation(); loc) {
						return loc->HasKeywordString(PlayerHome) || loc->HasKeywordString(Inn);
					}
				}
				return false;
			}
		};

	    struct UpdatePlayerCombat
		{
			static void thunk(RE::PlayerCharacter* a_this, float a_delta);
			static inline REL::Relocation<decltype(thunk)> func;

		private:
            static inline std::atomic_bool playerInCombat{ false };
		};

		Manager() = default;
		Manager(const Manager&) = delete;
		Manager(Manager&&) = delete;

		~Manager() override = default;

		Manager& operator=(const Manager&) = delete;
		Manager& operator=(Manager&&) = delete;

		std::atomic_bool playerInHouse{ false };
	};
}
