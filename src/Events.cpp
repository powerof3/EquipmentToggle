#include "Events.h"
#include "Graphics.h"

namespace Events
{
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

		static bool is_in_menu()
		{
			const auto UI = RE::UI::GetSingleton();
			return UI && (UI->GameIsPaused() || UI->IsModalMenuOpen() || UI->IsApplicationMenuOpen());
		}
	};

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

		Settings::GetSingleton()->ForEachSlot([&](const SlotData& a_slotData) {
			if (playerCombat && npcCombat && homeHide && dialogue && useHotKey) {
				return false;
			}

			auto& [hotKey, hide, unhide, slots] = a_slotData;

			if (!playerCombat && unhide.combat.CanDoPlayerToggle()) {
				playerCombat = true;
				stl::write_vfunc<RE::PlayerCharacter, 0x0E3, PlayerCombat>();

				logger::info("Registered for player combat hook");
			}
			if (!npcCombat && unhide.combat.CanDoFollowerToggle()) {
				npcCombat = true;
				if (const auto scripts = RE::ScriptEventSourceHolder::GetSingleton()) {
					scripts->AddEventSink<RE::TESCombatEvent>(GetSingleton());

					logger::info("Registered for NPC combat");
				}
			}
			if (!homeHide && hide.home.toggle != Toggle::Type::kDisabled) {
				homeHide = true;
				if (const auto player = RE::PlayerCharacter::GetSingleton()) {
					player->AddEventSink<RE::BGSActorCellEvent>(GetSingleton());

					logger::info("Registered for player cell change event");
				}
			}
			if (!dialogue && hide.dialogue.toggle != Toggle::Type::kDisabled) {
				dialogue = true;
				if (const auto menuMgr = RE::UI::GetSingleton()) {
					menuMgr->AddEventSink<RE::MenuOpenCloseEvent>(GetSingleton());

					logger::info("Registered for dialogue menu event");
				}
			}
			if (!useHotKey && hotKey.key != Key::kNone && hotKey.type.toggle != Toggle::Type::kDisabled) {
				useHotKey = true;
				if (const auto inputMgr = RE::BSInputDeviceManager::GetSingleton()) {
					inputMgr->AddEventSink(GetSingleton());

					logger::info("Registered for hotkey event");
				}
			}

			return true;
		});
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
			Graphics::ToggleActorEquipment(actor, can_toggle, false);
			break;
		case RE::ACTOR_COMBAT_STATE::kNone:
			Graphics::ToggleActorEquipment(actor, can_toggle, true);
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

		bool result = false;
		if (detail::is_cell_home(cell)) {
			playerInHouse = true;
			result = true;
		} else if (playerInHouse) {
			playerInHouse = false;
			result = true;
		}

		if (result) {
			Graphics::ToggleActorEquipment(
				player, [&](const SlotData& a_slotData) {
					return a_slotData.hide.home.CanDoPlayerToggle();
				},
				playerInHouse);

			Graphics::ToggleFollowerEquipment([](const SlotData& a_slotData) {
				return a_slotData.hide.home.CanDoFollowerToggle();
			},
				playerInHouse);
		}

		return EventResult::kContinue;
	}

	EventResult Manager::ProcessEvent(RE::InputEvent* const* a_evn, RE::BSTEventSource<RE::InputEvent*>*)
	{
		using InputType = RE::INPUT_EVENT_TYPE;
		using Keyboard = RE::BSWin32KeyboardDevice::Key;

		if (!a_evn || detail::is_in_menu()) {
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
				Graphics::ToggleActorEquipment(
					player, [](const SlotData& a_slotData) {
						return a_slotData.hide.dialogue.CanDoPlayerToggle();
					},
					a_evn->opening);

				const auto dialogueTarget = RE::MenuTopicManager::GetSingleton()->speaker.get();

				if (const auto dialogueTargetActor = dialogueTarget ? dialogueTarget->As<RE::Actor>() : nullptr) {
					Graphics::ToggleActorEquipment(
						dialogueTargetActor, [&](const SlotData& a_slotData) {
							return a_slotData.hide.dialogue.CanDoToggle(dialogueTargetActor);
						},
						a_evn->opening);
				}
			}
		}

		return EventResult::kContinue;
	}

	bool Manager::PlayerCombat::thunk(RE::PlayerCharacter* a_this)
	{
		const auto isInCombat = func(a_this);
		if (isInCombat) {
			if (!playerInCombat) {
				playerInCombat = true;
				Graphics::ToggleActorEquipment(
					a_this, [](const SlotData& a_slotData) {
						return a_slotData.unhide.combat.CanDoPlayerToggle();
					},
					true);
			}
		} else {
			if (playerInCombat) {
				playerInCombat = false;
				Graphics::ToggleActorEquipment(
					a_this, [](const SlotData& a_slotData) {
						return a_slotData.unhide.combat.CanDoPlayerToggle();
					},
					false);
			}
		}
		return isInCombat;
	}
}
