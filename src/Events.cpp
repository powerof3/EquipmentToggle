#include "Events.h"

#include "Graphics.h"
#include "Settings.h"

namespace Events
{
	Manager* Manager::GetSingleton()
	{
		static Manager singleton;
		return std::addressof(singleton);
	}

	void Manager::Register()
	{
		logger::info("{:*^30}", "EVENTS");

		bool playerCombat = false;
		bool npcCombat = false;
		bool homeHide = false;
		bool dialogue = false;
		bool useHotKey = false;

		const auto scripts = RE::ScriptEventSourceHolder::GetSingleton();

		Settings::GetSingleton()->ForEachSlot([&](const SlotData& a_slotData) {
			if (playerCombat && npcCombat && homeHide && dialogue && useHotKey) {
				return false;
			}

			auto& [hotKey, hide, unhide, slots] = a_slotData;

			if (!playerCombat && unhide.combat.CanDoPlayerToggle()) {
				playerCombat = true;

				std::array targets{
					std::make_pair(RELOCATION_ID(45569, 46869), 0xB0),                  // CombatManager::Update
					std::make_pair(RELOCATION_ID(39465, 40542), OFFSET(0x193, 0x17C)),  // PlayerCharacter::FinishLoadGame
					std::make_pair(RELOCATION_ID(39378, 40450), OFFSET(0x183, 0x18C)),  // PlayerCharacter::UpdateFreeCamera
				};

				for (const auto& [id, offset] : targets) {
					REL::Relocation<std::uintptr_t> target{ id, offset };
					stl::write_thunk_call<UpdatePlayerCombat>(target.address());
				}

				logger::info("Registered for player combat hook");
			}
			if (!npcCombat && unhide.combat.CanDoFollowerToggle()) {
				npcCombat = true;
				if (scripts) {
					scripts->AddEventSink<RE::TESCombatEvent>(GetSingleton());

					logger::info("Registered for NPC combat");
				}
			}
			if (!homeHide && hide.home.CanDoToggle()) {
				homeHide = true;
				if (const auto player = RE::PlayerCharacter::GetSingleton()) {
					player->AddEventSink<RE::BGSActorCellEvent>(GetSingleton());

					logger::info("Registered for player cell change event");
				}
			}
			if (!dialogue && hide.dialogue.CanDoToggle()) {
				dialogue = true;
				if (const auto menuMgr = RE::UI::GetSingleton()) {
					menuMgr->AddEventSink<RE::MenuOpenCloseEvent>(GetSingleton());

					logger::info("Registered for dialogue menu event");
				}
			}
			if (!useHotKey && hotKey.key != Key::kNone && hotKey.type.CanDoToggle()) {
				useHotKey = true;
				if (const auto inputMgr = RE::BSInputDeviceManager::GetSingleton()) {
					inputMgr->AddEventSink(GetSingleton());

					logger::info("Registered for hotkey event");
				}
			}

			return true;
		});
	}

	void Manager::UpdatePlayerCombat::thunk(RE::PlayerCharacter* a_this, float a_delta)
	{
		func(a_this, a_delta);

		const bool isInCombat = a_this->IsInCombat();

		if (isInCombat) {
			if (!playerInCombat) {
				playerInCombat = true;
				Graphics::ToggleActorEquipment(
					a_this, [](const SlotData& a_slotData) {
						return a_slotData.unhide.combat.CanDoPlayerToggle();
					},
					Slot::State::kUnhide);
			}
		} else {
			if (playerInCombat) {
				playerInCombat = false;
				Graphics::ToggleActorEquipment(
					a_this, [](const SlotData& a_slotData) {
						return a_slotData.unhide.combat.CanDoPlayerToggle();
					},
					Slot::State::kHide);
			}
		}
	}

	EventResult Manager::ProcessEvent(const RE::TESCombatEvent* evn, RE::BSTEventSource<RE::TESCombatEvent>*)
	{
		if (!evn || !evn->actor) {
			return EventResult::kContinue;
		}

		const auto actor = evn->actor->As<RE::Actor>();
		if (!actor) {
			return EventResult::kContinue;
		}

		const auto can_toggle = [&](const SlotData& a_slotData) {
			return a_slotData.unhide.combat.CanDoToggle(actor);
		};

		switch (*evn->newState) {
		case RE::ACTOR_COMBAT_STATE::kCombat:
			Graphics::ToggleActorEquipment(actor, can_toggle, Slot::State::kUnhide);
			break;
		case RE::ACTOR_COMBAT_STATE::kNone:
			Graphics::ToggleActorEquipment(actor, can_toggle, Slot::State::kHide);
			break;
		default:
			break;
		}

		return EventResult::kContinue;
	}

	EventResult Manager::ProcessEvent(const RE::BGSActorCellEvent* a_evn, RE::BSTEventSource<RE::BGSActorCellEvent>*)
	{
		if (!a_evn || a_evn->flags == RE::BGSActorCellEvent::CellFlag::kLeave) {
			return EventResult::kContinue;
		}

		const auto cell = RE::TESForm::LookupByID<RE::TESObjectCELL>(a_evn->cellID);
		if (!cell) {
			return EventResult::kContinue;
		}

		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player || !player->Is3DLoaded()) {
			return EventResult::kContinue;
		}

		const auto is_cell_home = [&]() {
			if (cell->IsInteriorCell()) {
				if (const auto loc = cell->GetLocation(); loc) {
					return loc->HasKeywordString(PlayerHome) || loc->HasKeywordString(Inn);
				}
			}
			return false;
		};

		bool result = false;
		if (is_cell_home()) {
			playerInHouse = true;
			result = true;
		} else if (playerInHouse) {
			playerInHouse = false;
			result = true;
		}

		if (result) {
			auto state = playerInHouse ? Slot::State::kHide : Slot::State::kUnhide;
		    Graphics::ToggleActorEquipment(
				player, [&](const SlotData& a_slotData) {
					return a_slotData.hide.home.CanDoPlayerToggle();
				},
				state);

			Graphics::ToggleFollowerEquipment([](const SlotData& a_slotData) {
				return a_slotData.hide.home.CanDoFollowerToggle();
			},
				state);
		}

		return EventResult::kContinue;
	}

	EventResult Manager::ProcessEvent(RE::InputEvent* const* a_evn, RE::BSTEventSource<RE::InputEvent*>*)
	{
		using InputType = RE::INPUT_EVENT_TYPE;
		using Keyboard = RE::BSWin32KeyboardDevice::Key;

		if (!a_evn) {
			return EventResult::kContinue;
		}

		if (const auto UI = RE::UI::GetSingleton(); UI->GameIsPaused() || UI->IsModalMenuOpen() || UI->IsApplicationMenuOpen()) {
			return EventResult::kContinue;
		}

		auto player = RE::PlayerCharacter::GetSingleton();
		if (!player || !player->Is3DLoaded()) {
			return EventResult::kContinue;
		}

		for (auto event = *a_evn; event; event = event->next) {
			if (event->eventType != InputType::kButton) {
				continue;
			}

			const auto button = static_cast<RE::ButtonEvent*>(event);
			if (!button->IsDown() || button->device != RE::INPUT_DEVICE::kKeyboard) {
				continue;
			}

			const auto key = static_cast<Key>(button->idCode);
			Graphics::ToggleAllEquipment([&](RE::Actor* a_actor, const SlotData& a_slotData) {
				auto& [hotKey, hide, unhide, slots] = a_slotData;
				return hotKey.key == key && hotKey.type.CanDoToggle(a_actor);
			});
		}

		return EventResult::kContinue;
	}

	EventResult Manager::ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		if (!a_evn) {
			return EventResult::kContinue;
		}

		if (a_evn->menuName == RE::DialogueMenu::MENU_NAME) {
			if (const auto player = RE::PlayerCharacter::GetSingleton(); player && player->Is3DLoaded()) {
				auto state = a_evn->opening ? Slot::State::kHide : Slot::State::kUnhide;

			    Graphics::ToggleActorEquipment(
					player, [](const SlotData& a_slotData) {
						return a_slotData.hide.dialogue.CanDoPlayerToggle();
					},
					state);

				const auto dialogueTarget = RE::MenuTopicManager::GetSingleton()->speaker.get();

				if (const auto dialogueTargetActor = dialogueTarget ? dialogueTarget->As<RE::Actor>() : nullptr) {
					Graphics::ToggleActorEquipment(
						dialogueTargetActor, [&](const SlotData& a_slotData) {
							return a_slotData.hide.dialogue.CanDoToggle(dialogueTargetActor);
						},
						state);
				}
			}
		}

		return EventResult::kContinue;
	}

	AnimationManager* AnimationManager::GetSingleton()
	{
		static AnimationManager singleton;
		return std::addressof(singleton);
	}

	void AnimationManager::Register()
	{
		bool weaponDraw = false;

		const auto scripts = RE::ScriptEventSourceHolder::GetSingleton();

		Settings::GetSingleton()->ForEachSlot([&](const SlotData& a_slotData) {
			if (weaponDraw) {
				return false;
			}

			auto& [hotKey, hide, unhide, slots] = a_slotData;

			if (!weaponDraw && unhide.weaponDraw.CanDoToggle()) {
				weaponDraw = true;

				if (scripts) {
					scripts->AddEventSink<RE::TESObjectLoadedEvent>(GetSingleton());
					scripts->AddEventSink<RE::TESSwitchRaceCompleteEvent>(GetSingleton());

					logger::info("Registered for weapon draw event");
				}
			}

			return true;
		});
	}

    void AnimationManager::RegisterForAnimationEventSink(RE::Actor* a_actor)
	{
		bool registeredForAnimEvent = false;

		Settings::GetSingleton()->ForEachSlot([&](const SlotData& a_slotData) {
			if (registeredForAnimEvent) {
				return false;
			}

			if (!registeredForAnimEvent && a_slotData.unhide.weaponDraw.CanDoToggle(a_actor)) {
				registeredForAnimEvent = true;
				a_actor->AddAnimationGraphEventSink(GetSingleton());
			}

			return true;
		});
	}

	EventResult AnimationManager::ProcessEvent(const RE::TESObjectLoadedEvent* a_evn, RE::BSTEventSource<RE::TESObjectLoadedEvent>*)
	{
		if (!a_evn) {
			return EventResult::kContinue;
		}

		const auto actor = RE::TESForm::LookupByID<RE::Actor>(a_evn->formID);
		if (!actor) {
			return EventResult::kContinue;
		}

		RegisterForAnimationEventSink(actor);

		return EventResult::kContinue;
	}

	EventResult AnimationManager::ProcessEvent(const RE::TESSwitchRaceCompleteEvent* a_evn, RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>*)
	{
		if (!a_evn || !a_evn->subject) {
			return EventResult::kContinue;
		}

		const auto actor = a_evn->subject->As<RE::Actor>();
		if (!actor || !actor->HasKeywordString(NPC)) {
			return EventResult::kContinue;
		}

		bool registeredForAnimEvent = false;

		Settings::GetSingleton()->ForEachSlot([&](const SlotData& a_slotData) {
			if (registeredForAnimEvent) {
				return false;
			}

			if (!registeredForAnimEvent && a_slotData.unhide.weaponDraw.CanDoToggle(actor)) {
				registeredForAnimEvent = true;
				if (actor->AddAnimationGraphEventSink(GetSingleton())) {
					logger::info("registered animation event for {} on race change", actor->GetName());
				}
			}

			return true;
		});

		return EventResult::kContinue;
	}

	EventResult AnimationManager::ProcessEvent(const RE::BSAnimationGraphEvent* a_evn, RE::BSTEventSource<RE::BSAnimationGraphEvent>*)
	{
		if (!a_evn || !a_evn->holder) {
			return EventResult::kContinue;
		}

		const auto actor = const_cast<RE::Actor*>(a_evn->holder->As<RE::Actor>());
		if (!actor) {
			return EventResult::kContinue;
		}

		if (a_evn->tag == "weapondraw") {
			Graphics::ToggleActorEquipment(
				actor, [&](const SlotData& a_slotData) {
					return a_slotData.unhide.weaponDraw.CanDoToggle(actor);
				},
				Slot::State::kUnhide);
		} else if (a_evn->tag == "weaponsheathe") {
			Graphics::ToggleActorEquipment(
				actor, [&](const SlotData& a_slotData) {
					return a_slotData.unhide.weaponDraw.CanDoToggle(actor);
				},
				Slot::State::kHide);
		}

		return EventResult::kContinue;
	}
}
