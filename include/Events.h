#pragma once

namespace Events
{
	using EventResult = RE::BSEventNotifyControl;

	class Manager final :
		public RE::BSTEventSink<RE::TESCombatEvent>,      // npc combat
		public RE::BSTEventSink<RE::BGSActorCellEvent>,   // cell home
		public RE::BSTEventSink<RE::MenuOpenCloseEvent>,  // dialogue
		public RE::BSTEventSink<RE::InputEvent*>          // hotkey
	{
	public:
		static Manager* GetSingleton();

		static void Register();

		EventResult ProcessEvent(const RE::TESCombatEvent* evn, RE::BSTEventSource<RE::TESCombatEvent>*) override;
		EventResult ProcessEvent(const RE::BGSActorCellEvent* a_evn, RE::BSTEventSource<RE::BGSActorCellEvent>*) override;
		EventResult ProcessEvent(RE::InputEvent* const* a_evn, RE::BSTEventSource<RE::InputEvent*>*) override;
		EventResult ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

	private:
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

	class AnimationManager final :
		public RE::BSTEventSink<RE::TESObjectLoadedEvent>,
		public RE::BSTEventSink<RE::TESSwitchRaceCompleteEvent>,
		public RE::BSTEventSink<RE::BSAnimationGraphEvent>
	{
	public:
		static AnimationManager* GetSingleton();

		static void Register();

		EventResult ProcessEvent(const RE::TESObjectLoadedEvent* a_evn, RE::BSTEventSource<RE::TESObjectLoadedEvent>*) override;
		EventResult ProcessEvent(const RE::TESSwitchRaceCompleteEvent* a_evn, RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>*) override;
		EventResult ProcessEvent(const RE::BSAnimationGraphEvent* a_evn, RE::BSTEventSource<RE::BSAnimationGraphEvent>*) override;

	private:
        void RegisterForAnimationEventSink(RE::Actor* a_actor);

	    AnimationManager() = default;
		AnimationManager(const AnimationManager&) = delete;
		AnimationManager(AnimationManager&&) = delete;

		~AnimationManager() override = default;

		AnimationManager& operator=(const AnimationManager&) = delete;
		AnimationManager& operator=(AnimationManager&&) = delete;
	};
}
