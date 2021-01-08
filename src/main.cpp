﻿#include "version.h"

// GLOBALS
namespace
{
	using Biped = RE::BIPED_OBJECT;
	using Key = RE::BSKeyboardDevice::Key;
	using HeadPart = RE::BGSHeadPart::HeadPartType;

	using SlotKeyData = std::pair<Key, std::set<Biped>>;
	using SlotKeyVec = std::vector<SlotKeyData>;

	static SlotKeyVec armorSlots;
	static SlotKeyVec weaponSlots;

	static std::int32_t automaticToggle = 0;
	static std::int32_t hotkeyToggle = 0;

	static bool autoUnequip = true;
	static bool unhideDuringCombat = true;
	static bool hideAtHome = true;

	static bool toggled = true;
	static bool toggleHair = false;
	static bool toggleHead = false;

	auto constexpr NPC = "ActorTypeNPC"sv;
	auto constexpr Head = "BSFaceGenNiNodeSkinned"sv;
}

//GLOBAL FUNCTIONS
namespace
{
	bool containsSlots(const SlotKeyVec& a_vec, std::function<bool(Biped a_slot)> a_func)
	{
		for (auto& [key, slotSet] : a_vec) {
			for (auto& slot : slotSet) {
				if (a_func(slot)) {
					return true;
				}
			}
		}

		return false;
	}


	bool CanToggleActorEquipment(RE::Actor* a_actor)
	{
		bool result = false;

		switch (automaticToggle) {
		case 0:
			if (a_actor->IsPlayerRef()) {
				result = true;
			}
			break;
		case 1:
			if (a_actor->IsPlayerTeammate() && a_actor->HasKeyword(NPC)) {
				result = true;
			}
			break;
		case 2:
			if (a_actor->IsPlayerRef() || (a_actor->IsPlayerTeammate() && a_actor->HasKeyword(NPC))) {
				result = true;
			}
			break;
		case 3:
			if (a_actor->HasKeyword(NPC)) {
				result = true;
			}
			break;
		default:
			break;
		}

		return result;
	}

	namespace HAIR
	{
		void DoHairDismember(RE::BSGeometry* a_shape, RE::TESRace* a_race, bool a_enable)
		{
			using func_t = decltype(&DoHairDismember);
			REL::Relocation<func_t> func{ REL::ID(24403) };
			return func(a_shape, a_race, a_enable);
		}


		void ToggleHairExtraParts(const RE::BSTArray<RE::BGSHeadPart*>& a_parts, RE::NiAVObject* a_root, RE::TESRace* a_race, bool a_hide)
		{
			for (auto& headpart : a_parts) {
				if (headpart) {
					if (auto node = a_root->GetObjectByName(headpart->formEditorID); node) {
						if (auto shape = node->AsGeometry(); shape) {
							DoHairDismember(shape, a_race, a_hide);
						}
					}
					ToggleHairExtraParts(headpart->extraParts, a_root, a_race, a_hide);
				}
			}
		}


		void ToggleHair(RE::NiAVObject* a_root, RE::TESNPC* a_npc, bool a_hide)
		{
			if (!a_npc && !a_root) {
				return;
			}

			auto race = a_npc->GetRace();
			if (race) {
				auto headPart = a_npc->GetCurrentHeadPartByType(HeadPart::kHair);
				if (!headPart) {
					headPart = race->GetHeadPartByType(HeadPart::kHair, a_npc->GetSex());
				}

				if (headPart) {
					if (auto node = a_root->GetObjectByName(headPart->formEditorID); node) {
						if (auto shape = node->AsDynamicTriShape(); shape) {
							DoHairDismember(shape, race, a_hide);
						}
					}
					ToggleHairExtraParts(headPart->extraParts, a_root, race, a_hide);
				}
			}
		}
	}

	namespace HEAD
	{
		void ToggleExtraParts(const RE::BSTArray<RE::BGSHeadPart*>& a_parts, RE::NiAVObject* a_root, std::uint16_t a_slot, bool a_hide)
		{
			for (auto& headpart : a_parts) {
				if (headpart) {
					if (auto node = a_root->GetObjectByName(headpart->formEditorID); node) {
						if (auto shape = node->AsGeometry(); shape) {
							shape->UpdateDismemberPartion(a_slot, !a_hide);
						}
					}
					ToggleExtraParts(headpart->extraParts, a_root, a_slot, a_hide);
				}
			}
		}


		void ToggleHeadPart(RE::NiAVObject* a_root, RE::TESNPC* a_npc, HeadPart a_type, std::uint16_t a_slot, bool a_hide)
		{
			if (!a_npc && !a_root) {
				return;
			}

			auto race = a_npc->GetRace();
			if (race) {
				auto headPart = a_npc->GetCurrentHeadPartByType(a_type);
				if (!headPart) {
					headPart = race->GetHeadPartByType(a_type, a_npc->GetSex());
				}

				if (headPart) {
					if (auto node = a_root->GetObjectByName(headPart->formEditorID); node) {
						if (auto shape = node->AsDynamicTriShape(); shape) {
							shape->UpdateDismemberPartion(a_slot, !a_hide);
						}
					}
					ToggleExtraParts(headPart->extraParts, a_root, a_slot, a_hide);
				}
			}
		}
	}


	void ToggleSlots(RE::TESNPC* a_npc, RE::BipedAnim* a_biped, RE::NiAVObject* a_root, const SlotKeyData& a_slots, bool a_hide)
	{
		if (!a_npc && !a_root) {
			return;
		}

		auto& [key, slotSet] = a_slots;
		for (auto& slot : slotSet) {
			auto object = a_biped->objects[slot];
			auto node = object.partClone.get();
			if (node) {
				node->ToggleNode(a_hide);
				if (auto face = a_root->GetObjectByName(Head); face) {
					switch (slot) {
					case RE::BIPED_OBJECT::kHead:
						{
							face->SetAppCulled(!a_hide);
							HAIR::ToggleHair(a_root, a_npc, !a_hide);
							HEAD::ToggleHeadPart(a_root, a_npc, HeadPart::kFace, 143, !a_hide);
						}
						break;
					case RE::BIPED_OBJECT::kHair:
					case RE::BIPED_OBJECT::kLongHair:
						{
							a_hide ? face->SetAppCulled(!a_hide) : face->SetAppCulled(a_hide);
							HAIR::ToggleHair(a_root, a_npc, !a_hide);
							HEAD::ToggleHeadPart(a_root, a_npc, HeadPart::kFace, 143, !a_hide);	 //ears
						}
						break;
					default:
						break;
					}
				}
			}
		}
	}


	void ToggleActorEquipment(RE::Actor* a_actor, bool a_hide)
	{
		auto biped = a_actor->biped.get();
		if (biped) {
			for (auto& armorSlot : armorSlots) {
				ToggleSlots(a_actor->GetActorBase(), biped, a_actor->Get3D(), armorSlot, a_hide);
			}
			for (auto& weaponSlot : weaponSlots) {
				ToggleSlots(a_actor->GetActorBase(), biped, a_actor->Get3D(), weaponSlot, a_hide);
			}
		}
	}


	void ToggleActorEquipment(RE::Actor* a_actor, const SlotKeyData& a_slots, bool a_hide)
	{
		auto biped = a_actor->biped.get();
		if (biped) {
			ToggleSlots(a_actor->GetActorBase(), biped, a_actor->Get3D(), a_slots, a_hide);
		}
	}


	void ToggleFollowerEquipment(bool a_hide)
	{
		auto processList = RE::ProcessLists::GetSingleton();
		if (processList) {
			for (auto& handle : processList->highActorHandles) {
				auto actorPtr = handle.get();
				auto actor = actorPtr.get();
				if (actor && actor->IsPlayerTeammate() && actor->HasKeyword(NPC)) {
					auto biped = actor->biped.get();
					if (biped) {
						for (auto& armorSlot : armorSlots) {
							ToggleSlots(actor->GetActorBase(), biped, actor->Get3D(), armorSlot, a_hide);
						}
						for (auto& weaponSlot : weaponSlots) {
							ToggleSlots(actor->GetActorBase(), biped, actor->Get3D(), weaponSlot, a_hide);
						}
					}
				}
			}
		}
	}


	void ToggleFollowerEquipment(const SlotKeyData& a_slots, bool a_hide)
	{
		auto processList = RE::ProcessLists::GetSingleton();
		if (processList) {
			for (auto& handle : processList->highActorHandles) {
				auto actorPtr = handle.get();
				auto actor = actorPtr.get();
				if (actor && actor->IsPlayerTeammate() && actor->HasKeyword(NPC)) {
					auto biped = actor->biped.get();
					if (biped) {
						ToggleSlots(actor->GetActorBase(), biped, actor->Get3D(), a_slots, a_hide);
					}
				}
			}
		}
	}
}

//ARMOR ATTACH
namespace Attach
{
	class Armor
	{
	public:
		static void Hook()
		{
			REL::Relocation<std::uintptr_t> AttachArmorAddon{ REL::ID(15501) };

			auto& trampoline = SKSE::GetTrampoline();
			_ProcessBiped = trampoline.write_call<5>(AttachArmorAddon.address() + 0xD53, ProcessBiped);
		}

	private:
		static void ProcessBiped(RE::BipedAnim* a_biped, std::int32_t a_slot, RE::TESObjectREFR* a_ref)
		{
			_ProcessBiped(a_biped, a_slot, a_ref);

			auto slot = static_cast<Biped>(a_slot);
			if (!a_biped || !a_ref || !toggled) {
				return;
			}

			if (containsSlots(armorSlots, [&](auto a_slot) { return a_slot == slot; }) || containsSlots(weaponSlots, [&](auto a_slot) { return a_slot == slot; })) {
				RE::Actor* actor = nullptr;
				if (actor = a_ref->As<RE::Actor>(); !actor) {
					return;
				}

				if (!CanToggleActorEquipment(actor)) {
					return;
				}

				auto biped = a_biped->objects[slot];
				auto node = biped.partClone.get();
				if (node) {
					node->ToggleNode(true);
				}
			}
		}
		static inline REL::Relocation<decltype(ProcessBiped)> _ProcessBiped;
	};

	class HeadHair	//there's too many functions that dismember stuff so we'll pick two and renable the slots ourselves
	{
	public:
		static void Hook()
		{
			auto& trampoline = SKSE::GetTrampoline();

			REL::Relocation<std::uintptr_t> Func1{ REL::ID(24236) };
			_PerformDismember = trampoline.write_call<5>(Func1.address() + 0x33E, PerformDismember);

			//other instances might not be even needed

			/*REL::Relocation<std::uintptr_t> Func2{ REL::ID(24221) };
			trampoline.write_call<5>(Func2.address() + 0x1DF, PerformDismember);

			REL::Relocation<std::uintptr_t> Func3{ REL::ID(24228) };
			trampoline.write_call<5>(Func3.address() + 0x1E0, PerformDismember);

			REL::Relocation<std::uintptr_t> Func4{ REL::ID(24237) };
			trampoline.write_call<5>(Func4.address() + 0x104, PerformDismember);*/

			REL::Relocation<std::uintptr_t> Func5{ REL::ID(15538) };
			_PerformDismember2 = trampoline.write_call<5>(Func5.address() + 0x261, PerformDismember2);	//if hair and head objects are the same? without this, hair partitions will stay hidden
		}

	private:
		static void PerformDismember(RE::TESNPC* a_npc, RE::Actor* a_actor, RE::NiAVObject* a_node)
		{
			_PerformDismember(a_npc, a_actor, a_node);

			if (toggled && (toggleHead || toggleHair)) {
				if (a_actor && a_npc && CanToggleActorEquipment(a_actor)) {
					if (auto root = a_actor->Get3D(); root) {
						if (auto face = root->GetObjectByName(Head); face) {
							if (toggleHead) {
								face->SetAppCulled(false);
								HAIR::ToggleHair(root, a_npc, false);
								HEAD::ToggleHeadPart(root, a_npc, HeadPart::kFace, 143, false);
							}
							if (toggleHair) {
								if (face->GetAppCulled()) {
									face->SetAppCulled(false);
								}
								HAIR::ToggleHair(root, a_npc, false);
								HEAD::ToggleHeadPart(root, a_npc, HeadPart::kFace, 143, false);
							}
						}
					}
				}
			}
		}
		static inline REL::Relocation<decltype(PerformDismember)> _PerformDismember;


		static void PerformDismember2(RE::BipedAnim* a_biped, std::uint32_t a_slot)
		{
			if (toggled && (toggleHead || toggleHair) && a_biped) {
				auto refPtr = a_biped->actorRef.get();
				if (auto ref = refPtr.get(); ref) {
					if (auto actor = ref->As<RE::Actor>(); actor && CanToggleActorEquipment(actor)) {
						return;
					}
				}
			}
			_PerformDismember2(a_biped, a_slot);
		}
		static inline REL::Relocation<decltype(PerformDismember2)> _PerformDismember2;
	};


	void Install()
	{
		Armor::Hook();
		HeadHair::Hook();

		logger::info("Hooked armor attach (hide when equipped)");
	}
}

//COMBAT
namespace Combat
{
	static bool playerInCombat = false;


	class NPCCombat : public RE::BSTEventSink<RE::TESCombatEvent>
	{
	public:
		using EventResult = RE::BSEventNotifyControl;

		static NPCCombat* GetSingleton()
		{
			static NPCCombat singleton;
			return &singleton;
		}

	protected:
		virtual EventResult ProcessEvent(const RE::TESCombatEvent* evn, RE::BSTEventSource<RE::TESCombatEvent>* a_eventSource) override
		{
			if (!evn) {
				return EventResult::kContinue;
			}

			auto ref = evn->actor.get();
			if (!ref) {
				return EventResult::kContinue;
			}

			auto actor = ref->As<RE::Actor>();
			if (!actor) {
				return EventResult::kContinue;
			}

			if (!CanToggleActorEquipment(actor)) {
				return EventResult::kContinue;
			}

			switch (*evn->newState) {
			case RE::ACTOR_COMBAT_STATE::kCombat:
				ToggleActorEquipment(actor, false);
				break;
			case RE::ACTOR_COMBAT_STATE::kNone:
				ToggleActorEquipment(actor, true);
				break;
			default:
				break;
			}

			return EventResult::kContinue;
		}

	private:
		NPCCombat() = default;
		NPCCombat(const NPCCombat&) = delete;
		NPCCombat(NPCCombat&&) = delete;
		virtual ~NPCCombat() = default;

		NPCCombat& operator=(const NPCCombat&) = delete;
		NPCCombat& operator=(NPCCombat&&) = delete;
	};


	class PlayerCombat
	{
	public:
		static void Install()
		{
			REL::Relocation<std::uintptr_t> vtbl{ REL::ID(261916) };
			_IsInCombat = vtbl.write_vfunc(0x0E3, IsInCombat);
		}

	private:
		static bool IsInCombat(RE::PlayerCharacter* a_this)
		{
			auto result = _IsInCombat(a_this);
			if (result) {
				if (!playerInCombat) {
					playerInCombat = true;
					ToggleActorEquipment(a_this, false);
				}
			} else {
				if (playerInCombat) {
					playerInCombat = false;
					ToggleActorEquipment(a_this, true);
				}
			}
			return result;
		}

		using IsInCombat_t = decltype(&RE::Actor::IsInCombat);	// 5E
		static inline REL::Relocation<IsInCombat_t> _IsInCombat;
	};


	void Install()
	{
		PlayerCombat::Install();
		logger::info("Registered for player combat (unhide during combat)");
		if (automaticToggle != 0) {
			auto srcHolder = RE::ScriptEventSourceHolder::GetSingleton();
			if (srcHolder) {
				srcHolder->AddEventSink(Combat::NPCCombat::GetSingleton());
				logger::info("Registered for npc combat (unhide during combat)");
			}
		}
	}
}

//LOC CHANGE
namespace Location
{
	auto constexpr PlayerHome = "LocTypePlayerHouse"sv;
	auto constexpr Inn = "LocTypeInn"sv;

	static bool playerInHouse = false;

	class LocChange : public RE::BSTEventSink<RE::BGSActorCellEvent>
	{
	public:
		using EventResult = RE::BSEventNotifyControl;

		static LocChange* GetSingleton()
		{
			static LocChange singleton;
			return &singleton;
		}

	protected:
		virtual EventResult ProcessEvent(const RE::BGSActorCellEvent* a_evn, RE::BSTEventSource<RE::BGSActorCellEvent>* a_eventSource) override
		{
			using CellFlag = RE::BGSActorCellEvent::CellFlag;

			if (!a_evn || a_evn->flags == CellFlag::kLeave) {
				return EventResult::kContinue;
			}

			auto cell = RE::TESForm::LookupByID<RE::TESObjectCELL>(a_evn->cellID);
			if (!cell) {
				return EventResult::kContinue;
			}

			auto player = RE::PlayerCharacter::GetSingleton();
			if (player) {
				auto loc = cell->GetLocation();
				if (loc && (loc->HasKeywordString(PlayerHome) || loc->HasKeywordString(Inn))) {
					playerInHouse = true;
					ToggleActorEquipment(player, true);
					switch (automaticToggle) {
					case 1:
					case 2:
						ToggleFollowerEquipment(true);
						break;
					default:
						break;
					}
				} else if (playerInHouse) {
					playerInHouse = false;
					ToggleActorEquipment(player, false);
					switch (automaticToggle) {
					case 1:
					case 2:
						ToggleFollowerEquipment(false);
						break;
					default:
						break;
					}
				}
			}

			return EventResult::kContinue;
		}

	private:
		LocChange() = default;
		LocChange(const LocChange&) = delete;
		LocChange(LocChange&&) = delete;
		virtual ~LocChange() = default;

		LocChange& operator=(const LocChange&) = delete;
		LocChange& operator=(LocChange&&) = delete;
	};


	void Install()
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		if (player) {
			player->AddEventSink(Location::LocChange::GetSingleton());
			logger::info("registered for location change (hide at home)");
		}
	}
}

//HOTKEY
namespace Toggle
{
	bool ProcessKey(Key a_key, const SlotKeyVec& a_armor, const SlotKeyVec& a_weapon, std::function<void(const SlotKeyData& a_slots)> a_func)
	{
		bool result = false;

		for (auto& data : a_armor) {
			auto& [key, slots] = data;
			if (key == a_key) {
				result = true;
				a_func(data);
			}
		}

		if (!result) {
			for (auto& data : a_weapon) {
				auto& [key, slots] = data;
				if (key == a_key) {
					result = true;
					a_func(data);
				}
			}
		}

		return false;
	}


	void ToggleNPCEquipment(const SlotKeyData& a_slots, bool a_hide)
	{
		auto processList = RE::ProcessLists::GetSingleton();
		if (processList) {
			for (auto& handle : processList->highActorHandles) {
				auto actorPtr = handle.get();
				auto actor = actorPtr.get();
				if (actor && actor->HasKeyword(NPC)) {
					auto biped = actor->biped.get();
					if (biped) {
						ToggleSlots(actor->GetActorBase(), biped, actor->Get3D(), a_slots, a_hide);
					}
				}
			}
		}
	}


	class InputHandler : public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		static InputHandler* GetSingleton()
		{
			static InputHandler singleton;
			return &singleton;
		}

	protected:
		using EventResult = RE::BSEventNotifyControl;

		EventResult ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) override
		{
			using InputType = RE::INPUT_EVENT_TYPE;
			using Keyboard = RE::BSWin32KeyboardDevice::Key;

			if (!a_event) {
				return EventResult::kContinue;
			}

			for (auto event = *a_event; event; event = event->next) {
				if (event->eventType != InputType::kButton) {
					continue;
				}

				auto button = static_cast<RE::ButtonEvent*>(event);
				if (!button->IsDown() || button->device != RE::INPUT_DEVICE::kKeyboard) {
					continue;
				}

				auto key = static_cast<Key>(button->idCode);
				ProcessKey(key, armorSlots, weaponSlots, [&](const auto& a_slots) {
					toggled = !toggled;
					switch (hotkeyToggle) {
					case 0:
						ToggleActorEquipment(RE::PlayerCharacter::GetSingleton(), a_slots, toggled);
						break;
					case 1:
						ToggleFollowerEquipment(a_slots, toggled);
						break;
					case 2:
						{
							ToggleActorEquipment(RE::PlayerCharacter::GetSingleton(), toggled);
							ToggleFollowerEquipment(a_slots, toggled);
						}
						break;
					case 3:
						{
							ToggleActorEquipment(RE::PlayerCharacter::GetSingleton(), toggled);
							ToggleNPCEquipment(a_slots, toggled);
						}
						break;
					default:
						break;
					}
				});
			}

			return EventResult::kContinue;
		}

	private:
		InputHandler() = default;
		InputHandler(const InputHandler&) = delete;
		InputHandler(InputHandler&&) = delete;
		virtual ~InputHandler() = default;

		InputHandler& operator=(const InputHandler&) = delete;
		InputHandler& operator=(InputHandler&&) = delete;
	};


	void Install()
	{
		auto inputHolder = RE::BSInputDeviceManager::GetSingleton();
		if (inputHolder) {
			inputHolder->AddEventSink(Toggle::InputHandler::GetSingleton());
			logger::info("Registered input sink (toggle)");
		}
	}
}

//INI
namespace INI
{
	namespace STRING = SKSE::UTIL::STRING;

	constexpr frozen::map<std::uint32_t, Biped, 31> bipedMap = {
		{ 30, Biped::kHead },
		{ 31, Biped::kHair },
		{ 32, Biped::kBody },
		{ 33, Biped::kHands },
		{ 34, Biped::kForearms },
		{ 35, Biped::kAmulet },
		{ 36, Biped::kRing },
		{ 37, Biped::kFeet },
		{ 38, Biped::kCalves },
		{ 39, Biped::kShield },
		{ 40, Biped::kTail },
		{ 41, Biped::kLongHair },
		{ 42, Biped::kCirclet },
		{ 43, Biped::kEars },
		{ 44, Biped::kModMouth },
		{ 45, Biped::kModNeck },
		{ 46, Biped::kModChestPrimary },
		{ 47, Biped::kModBack },
		{ 48, Biped::kModMisc1 },
		{ 49, Biped::kModPelvisPrimary },
		{ 50, Biped::kDecapitateHead },
		{ 51, Biped::kDecapitate },
		{ 52, Biped::kModPelvisSecondary },
		{ 53, Biped::kModLegRight },
		{ 54, Biped::kModLegLeft },
		{ 55, Biped::kModFaceJewelry },
		{ 56, Biped::kModChestSecondary },
		{ 57, Biped::kModShoulder },
		{ 58, Biped::kModArmLeft },
		{ 59, Biped::kModArmRight },
		{ 60, Biped::kModMisc2 },
		{ 61, Biped::kFX01 },
	};


	std::optional<SlotKeyData> GetArmorINIData(const std::string& a_value)
	{
		auto sections = STRING::split(a_value, " | ");
		if (sections.empty()) {
			return std::nullopt;
		}

		Key key = Key::kNone;
		try {
			key = static_cast<Key>(STRING::to_int<std::uint32_t>(sections.at(0)));
			logger::info("		Key : {}", key);
		} catch (...) {
		}

		std::set<Biped> slotSet;
		try {
			if (!sections.at(1).empty()) {
				auto vec = STRING::split(sections.at(1), " , ");
				for (auto& slotStr : vec) {
					auto slot = STRING::to_int<std::uint32_t>(slotStr);
					switch (slot) {
					case 32:
					case 33:
					case 34:
					case 37:
					case 38:
					case 40:
						{
							logger::info("			armor : unreplaceable slot {}", slot);
							continue;
						}
					default:
						break;
					}
					if (slot > 61) {
						logger::info("			armor : invalid slot {}", slot);
						continue;
					}
					logger::info("			armor : slot {}", slot);
					slotSet.insert(bipedMap.at(slot));
				}
			}
		} catch (...) {
			logger::error("unable to parse {}", a_value);
			return std::nullopt;
		}

		return std::make_pair(key, slotSet);
	}


	std::optional<SlotKeyData> GetWeaponINIData(const std::string& a_value)
	{
		auto sections = STRING::split(a_value, " | ");
		if (sections.empty()) {
			return std::nullopt;
		}

		Key key = Key::kNone;
		try {
			key = static_cast<Key>(STRING::to_int<std::uint32_t>(sections.at(0)));
			logger::info("		Key : {}", key);
		} catch (...) {
		}

		std::set<Biped> slotSet;
		try {
			if (!sections.at(1).empty()) {
				auto vec = STRING::split(sections.at(1), " , ");
				for (auto& slotStr : vec) {
					logger::info("			weapon : slot {}", slotStr);
					slotSet.insert(static_cast<Biped>(STRING::to_int<std::uint32_t>(slotStr)));
				}
			}
		} catch (...) {
			logger::error("unable to parse {}", a_value);
			return std::nullopt;
		}

		return std::make_pair(key, slotSet);
	}


	void GetDataFromINI(const CSimpleIniA& ini, const char* a_section, const char* a_type, SlotKeyVec& a_INIDataVec, bool a_armor)
	{
		CSimpleIniA::TNamesDepend values;
		ini.GetAllValues(a_section, a_type, values);
		values.sort(CSimpleIniA::Entry::LoadOrder());

		logger::info("	{} entries found : {}", a_type, values.size());

		for (auto& value : values) {
			auto data = a_armor ? GetArmorINIData(value.pItem) : GetWeaponINIData(value.pItem);
			if (data.has_value()) {
				a_INIDataVec.emplace_back(data.value());
			}
		}
	}

	bool Read()
	{
		auto pluginPath = SKSE::GetPluginConfigPath("po3_EquipmentToggle");

		CSimpleIniA ini;
		ini.SetUnicode();
		ini.SetMultiKey();

		SI_Error rc = ini.LoadFile(pluginPath.c_str());
		if (rc < 0) {
			logger::error("Can't load 'po3_EquipmentToggle.ini'");
			return false;
		}

		automaticToggle = STRING::to_int<std::int32_t>(ini.GetValue("Settings", "Auto Toggle Type", "0"));
		hotkeyToggle = STRING::to_int<std::int32_t>(ini.GetValue("Settings", "Toggle Key Type", "0"));

		autoUnequip = ini.GetBoolValue("Settings", "Hide When Equipped", true);
		unhideDuringCombat = ini.GetBoolValue("Settings", "Unhide During Combat", false);
		hideAtHome = ini.GetBoolValue("Settings", "Hide At Home", false);

		GetDataFromINI(ini, "Armor", "Armor", armorSlots, true);
		GetDataFromINI(ini, "Weapon", "Weapon", weaponSlots, false);

		toggleHair = containsSlots(armorSlots, [&](auto a_slot) { return a_slot == Biped::kHair || a_slot == Biped::kLongHair; });
		toggleHead = containsSlots(armorSlots, [&](Biped a_slot) { return a_slot == Biped::kHead; });

		return true;
	}
}


void OnInit(SKSE::MessagingInterface::Message* a_msg)
{
	using Slot = RE::BIPED_MODEL::BipedObjectSlot;

	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		{
			if (automaticToggle != -1) {
				if (unhideDuringCombat) {
					Combat::Install();
				}
				if (hideAtHome) {
					Location::Install();
				}
			}
			Toggle::Install();
		}
		break;
	}
}


extern "C" DLLEXPORT bool APIENTRY SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	try {
		auto path = logger::log_directory().value() / "po3_EquipmentToggle.log";
		auto log = spdlog::basic_logger_mt("global log", path.string(), true);
		log->flush_on(spdlog::level::info);

#ifndef NDEBUG
		log->set_level(spdlog::level::debug);
		log->sinks().push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
#else
		log->set_level(spdlog::level::info);

#endif
		spdlog::set_default_logger(log);
		spdlog::set_pattern("[%H:%M:%S] [%l] %v");

		logger::info("Equipment Toggle {}", SOS_VERSION_VERSTRING);

		a_info->infoVersion = SKSE::PluginInfo::kVersion;
		a_info->name = "Equipment Toggle";
		a_info->version = SOS_VERSION_MAJOR;

		if (a_skse->IsEditor()) {
			logger::critical("Loaded in editor, marking as incompatible");
			return false;
		}

		const auto ver = a_skse->RuntimeVersion();
		if (ver < SKSE::RUNTIME_1_5_39) {
			logger::critical("Unsupported runtime version {}", ver.string());
			return false;
		}
	} catch (const std::exception& e) {
		logger::critical(e.what());
		return false;
	} catch (...) {
		logger::critical("caught unknown exception");
		return false;
	}

	return true;
}


extern "C" DLLEXPORT bool APIENTRY SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	try {
		logger::info("Equipment Toggle loaded");

		SKSE::Init(a_skse);

		INI::Read();

		auto messaging = SKSE::GetMessagingInterface();
		if (!messaging->RegisterListener("SKSE", OnInit)) {
			return false;
		}

		if (!armorSlots.empty() && !weaponSlots.empty()) {
			logger::critical("no valid slot entries found!");
			return false;
		}

		if (autoUnequip) {
			SKSE::AllocTrampoline(1 << 6);
			Attach::Install();
		}

	} catch (const std::exception& e) {
		logger::critical(e.what());
		return false;
	} catch (...) {
		logger::critical("caught unknown exception");
		return false;
	}

	return true;
}