#include "hair.h"
#include "version.h"

namespace
{
	using SlotData = std::pair<bool, std::set<Biped>>;
	using SlotKeyData = std::pair<Key, SlotData>;
	using SlotKeyVec = std::vector<SlotKeyData>;

	SlotKeyVec armorSlots;
	SlotKeyVec weaponSlots;

	std::int32_t automaticToggleVal = 0;
	std::int32_t hotkeyToggleVal = 0;

	bool autoUnequip = true;
	bool unhideDuringCombat = true;
	bool hideAtHome = true;

	auto constexpr NPC = "ActorTypeNPC"sv;
	auto constexpr PlayerHome = "LocTypePlayerHouse"sv;
	auto constexpr Inn = "LocTypeInn"sv;


	auto GetSlotInfo(const SlotKeyVec& a_vec, std::function<bool(Biped a_slot)> a_func) -> std::tuple<bool, bool, Biped>
	{
		for (auto& [key, data] : a_vec) {
			auto [autoToggle, slots] = data;
			for (auto& slot : slots) {
				if (a_func(slot)) {
					return { true, autoToggle, slot };
				}
			}
		}
		return { false, false, Biped::kNone };
	}

	auto CanToggleActorEquipment(RE::Actor* a_actor) -> bool
	{
		switch (automaticToggleVal) {
		case 0:
			return a_actor->IsPlayerRef();
		case 1:
			return a_actor->IsPlayerTeammate() && a_actor->HasKeyword(NPC);
		case 2:
			return a_actor->IsPlayerRef() || a_actor->IsPlayerTeammate() && a_actor->HasKeyword(NPC);
		case 3:
			return a_actor->HasKeyword(NPC);
		default:
			return false;
		}
	}
}


namespace SlotManager
{
	void SetData(RE::NiAVObject* a_object, const Biped a_slot, bool a_hide)
	{
		const auto name("EquipToggle - " + std::to_string(to_underlying(a_slot)));

		if (const auto extra = a_object->GetExtraData<RE::NiBooleanExtraData>(name); extra) {
			extra->data = a_hide;
		} else {
			const auto newExtra = RE::NiBooleanExtraData::Create(name, a_hide);
			if (newExtra) {
				a_object->AddExtraData(newExtra);
			}
		}
	}


	bool GetData(RE::NiAVObject* a_object, const Biped a_slot, bool a_default)
	{
		const auto name("EquipToggle - " + std::to_string(to_underlying(a_slot)));

		if (const auto extra = a_object->GetExtraData<RE::NiBooleanExtraData>(name); extra) {
			return extra->data;
		}

		const auto newExtra = RE::NiBooleanExtraData::Create(name, a_default);
		if (newExtra) {
			a_object->AddExtraData(newExtra);
		}

		return a_default;
	}
}


namespace Manager
{
	void ToggleDecal(RE::NiAVObject* a_root, RE::NiAVObject* a_node, bool a_hide)
	{
		const auto decalNode = netimmerse_cast<RE::BGSDecalNode*>(a_root->GetObjectByName(RE::FixedStrings::GetSingleton()->skinnedDecalNode));
		if (decalNode) {
			RE::BSVisit::TraverseScenegraphGeometries(a_node, [&](RE::BSGeometry* a_geometry) -> RE::BSVisit::BSVisitControl {
				for (auto& tempEffectPtr : decalNode->decals) {
					const auto tempEffect = tempEffectPtr.get();
					if (tempEffect) {
						const auto geometryDecalEffect = tempEffect->As<RE::BSTempEffectGeometryDecal>();
						if (geometryDecalEffect && geometryDecalEffect->attachedGeometry.get() == a_geometry) {
							auto decal = geometryDecalEffect->Get3D();
							if (decal) {
								decal->ToggleNode(a_hide);
							}
						}
					}
				}
				return RE::BSVisit::BSVisitControl::kContinue;
			});
		}
	}

	void ToggleSlots(RE::Actor* a_actor, RE::BipedAnim* a_biped, RE::NiAVObject* a_root, const std::set<Biped>& a_slots, bool a_hide)
	{
		if (!a_actor || !a_root) {
			return;
		}

		const auto task = SKSE::GetTaskInterface();
		task->AddTask([a_biped, a_slots, a_actor, a_root, a_hide]() {
			for (auto& slot : a_slots) {
				auto object = a_biped->objects[slot];
				auto node = object.partClone.get();
				if (node) {
					node->ToggleNode(a_hide);
					if (slot < Biped::kEditorTotal) {
						if (const auto parent = node->parent; parent) {
							ToggleDecal(parent, node, a_hide);
						}
						switch (slot) {
						case Biped::kHead:
						case Biped::kHair:
						case Biped::kLongHair:
						case Biped::kCirclet:
						case Biped::kEars:
						case Biped::kDecapitateHead:
							{
								Dismemberment::Update(a_actor, a_root, object.addon, HeadPart::kFace, a_hide);
								Dismemberment::Update(a_actor, a_root, object.addon, HeadPart::kHair, a_hide);
							}
							break;
						default:
							break;
						}
					}
				}
			}
		});
	}

	void ToggleSlots(RE::Actor* a_actor, RE::BipedAnim* a_biped, RE::NiAVObject* a_root, const SlotData& a_slotData)
	{
		if (!a_actor || !a_root) {
			return;
		}

		const auto task = SKSE::GetTaskInterface();
		task->AddTask([a_biped, a_slotData, a_actor, a_root]() {
			auto& [autoToggle, slots] = a_slotData;
			for (auto& slot : slots) {
				const auto hiddenState = SlotManager::GetData(a_root, slot, autoToggle);
				SlotManager::SetData(a_root, slot, !hiddenState);

				auto object = a_biped->objects[slot];
				auto node = object.partClone.get();
				if (node) {
					node->ToggleNode(!hiddenState);
					if (slot < Biped::kEditorTotal) {
						if (const auto parent = node->parent; parent) {
							ToggleDecal(parent, node, !hiddenState);
						}
						switch (slot) {
						case Biped::kHead:
						case Biped::kHair:
						case Biped::kLongHair:
						case Biped::kCirclet:
						case Biped::kEars:
						case Biped::kDecapitateHead:
							{
								Dismemberment::Update(a_actor, a_root, object.addon, HeadPart::kFace, !hiddenState);
								Dismemberment::Update(a_actor, a_root, object.addon, HeadPart::kHair, !hiddenState);
							}
							break;
						default:
							break;
						}
					}
				}
			}
		});
	}

	void ToggleActorEquipment(RE::Actor* a_actor, const bool a_hide)
	{
		const auto biped = a_actor->GetCurrentBiped().get();
		if (biped) {
			std::array<RE::NiAVObject*, 2> skeletonRoot = { a_actor->Get3D(true), a_actor->Get3D(false) };
			if (skeletonRoot[0] == skeletonRoot[1]) {
				skeletonRoot[1] = nullptr;
			}

			for (auto& [key, armorData] : armorSlots) {
				auto& [autoToggle, slots] = armorData;

				for (auto& skeleton : skeletonRoot) {
					if (skeleton) {
						for (auto& slot : slots) {
							SlotManager::SetData(skeleton, slot, a_hide);
						}
						ToggleSlots(a_actor, biped, skeleton, slots, a_hide);
					}
				}
			}

			for (auto& [key, weaponData] : weaponSlots) {
				auto& [autoToggle, slots] = weaponData;

				for (auto& skeleton : skeletonRoot) {
					if (skeleton) {
						for (auto& slot : slots) {
							SlotManager::SetData(skeleton, slot, a_hide);
						}
						ToggleSlots(a_actor, biped, skeleton, slots, a_hide);
					}
				}
			}
		}
	}

	void ToggleActorEquipment(RE::Actor* a_actor, const SlotData& a_slotData)
	{
		const auto biped = a_actor->GetCurrentBiped().get();
		if (biped) {
			std::array<RE::NiAVObject*, 2> skeletonRoot = { a_actor->Get3D(false), a_actor->Get3D(true) };
			if (skeletonRoot[0] == skeletonRoot[1]) {
				skeletonRoot[1] = nullptr;
			}

			for (auto& skeleton : skeletonRoot) {
				ToggleSlots(a_actor, biped, skeleton, a_slotData);
			}
		}
	}

	void ToggleFollowerEquipment(const bool a_hide)
	{
		auto processList = RE::ProcessLists::GetSingleton();
		if (processList) {
			for (auto& handle : processList->highActorHandles) {
				auto actorPtr = handle.get();
				const auto actor = actorPtr.get();
				if (actor && actor->IsPlayerTeammate() && actor->HasKeyword(NPC)) {
					const auto biped = actor->GetCurrentBiped().get();
					const auto root = actor->Get3D(false);
					if (biped && root) {
						for (auto& [key, armorData] : armorSlots) {
							auto& [autoToggle, slots] = armorData;
							for (auto& slot : slots) {
								SlotManager::SetData(root, slot, a_hide);
							}
							ToggleSlots(actor, biped, actor->Get3D(false), slots, a_hide);
						}
						for (auto& [key, weaponData] : weaponSlots) {
							auto& [autoToggle, slots] = weaponData;
							for (auto& slot : slots) {
								SlotManager::SetData(root, slot, a_hide);
							}
							ToggleSlots(actor, biped, actor->Get3D(false), slots, a_hide);
						}
					}
				}
			}
		}
	}

	void ToggleFollowerEquipment(const SlotData& a_slotData)
	{
		auto processList = RE::ProcessLists::GetSingleton();
		if (processList) {
			for (auto& handle : processList->highActorHandles) {
				auto actorPtr = handle.get();
				const auto actor = actorPtr.get();
				if (actor && actor->IsPlayerTeammate() && actor->HasKeyword(NPC)) {
					const auto biped = actor->GetCurrentBiped().get();
					if (biped) {
						ToggleSlots(actor, biped, actor->Get3D(false), a_slotData);
					}
				}
			}
		}
	}
}


namespace SaveManager
{
	class Reset
	{
	public:
		static void Hook()
		{
			auto& trampoline = SKSE::GetTrampoline();

			REL::Relocation<std::uintptr_t> ActorInitLoadGame{ REL::ID(36643) };
			_GetNiNode = trampoline.write_call<5>(ActorInitLoadGame.address() + 0x119, GetNiNode);
		}
	private:
		static RE::NiAVObject* GetNiNode(RE::Actor* a_actor)
		{
			const auto root = _GetNiNode(a_actor);

			if (a_actor && CanToggleActorEquipment(a_actor))
			{
				logger::info("resetting {}", a_actor->GetName());

				if (root) {
					for (auto& [key, armorData] : armorSlots) {
						auto& [autoToggle, slots] = armorData;
						for (auto& slot : slots) {
							SlotManager::SetData(root, slot, autoToggle);
						}
					}
					for (auto& [key, weaponData] : weaponSlots) {
						auto& [autoToggle, slots] = weaponData;
						for (auto& slot : slots) {
							SlotManager::SetData(root, slot, autoToggle);
						}
					}
				}								
			}

			return root;
		}
		static inline REL::Relocation<decltype(GetNiNode)> _GetNiNode;
		
	};

	void Install()
	{
		Reset::Hook();
	}
}


namespace Attach
{
	class Armor
	{
	public:
		static void Hook()
		{
			auto& trampoline = SKSE::GetTrampoline();
			REL::Relocation<std::uintptr_t> AttachGeometry{ REL::ID(15535) };
			_ProcessGeometry = trampoline.write_call<5>(AttachGeometry.address() + 0x79A, ProcessGeometry);

			REL::Relocation<std::uintptr_t> AttachArmorAddon{ REL::ID(15501) };
			_ProcessObject = trampoline.write_call<5>(AttachArmorAddon.address() + 0x1EA, ProcessObject);  //armor2
		}

	private:
		static void ProcessGeometry(RE::BipedAnim* a_biped, RE::BSGeometry* a_object, RE::BSDismemberSkinInstance* a_dismemberInstance, std::int32_t a_slot, bool a_unk05)
		{
			_ProcessGeometry(a_biped, a_object, a_dismemberInstance, a_slot, a_unk05);

			if (a_biped && a_object) {
				const auto refPtr = a_biped->actorRef.get();
				const auto ref = refPtr.get();

				if (ref) {
					const auto actor = ref->As<RE::Actor>();
					if (actor && CanToggleActorEquipment(actor)) {
						auto slot = static_cast<Biped>(a_slot);
						const auto root = actor->GetCurrent3D();
						if (root) {
							auto [contains, autoToggle, matchingSlot] = GetSlotInfo(armorSlots, [&](const auto& b_slot) { return b_slot == slot; });
							if (contains && autoToggle && SlotManager::GetData(root, slot, autoToggle)) {
								a_object->ToggleNode(true);
							}
						}
					}
				}
			}
		}
		static inline REL::Relocation<decltype(ProcessGeometry)> _ProcessGeometry;

		static void ProcessObject(RE::BipedAnim* a_biped, RE::NiAVObject* a_object, std::int32_t a_slot, bool a_unk04)
		{
			using ShaderType = RE::BSShaderMaterial::Feature;

			_ProcessObject(a_biped, a_object, a_slot, a_unk04);

			if (a_biped && a_object) {
				const auto refPtr = a_biped->actorRef.get();
				const auto ref = refPtr.get();

				if (ref) {
					const auto actor = ref->As<RE::Actor>();
					if (actor && CanToggleActorEquipment(actor)) {
						auto slot = static_cast<Biped>(a_slot);
						const auto root = actor->GetCurrent3D();
						if (root) {
							auto [contains, autoToggle, matchingSlot] = GetSlotInfo(armorSlots, [&](const auto& b_slot) { return b_slot == slot; });
							if (contains && SlotManager::GetData(root, slot, autoToggle)) {
								a_object->ToggleNode(true);
							}
						}
					}
				}
			}
		}
		static inline REL::Relocation<decltype(ProcessObject)> _ProcessObject;
	};

	class Weapon
	{
	public:
		static void Hook()
		{
			REL::Relocation<std::uintptr_t> AttachWeapon{ REL::ID(15506) };
			auto& trampoline = SKSE::GetTrampoline();

			switch (automaticToggleVal) {
			case 0:
				_AttachWeaponToActor_Player = trampoline.write_call<5>(AttachWeapon.address() + 0x17F, AttachWeaponToActor_Player);
				break;
			case 1:
				_AttachWeaponToActor_NPC = trampoline.write_call<5>(AttachWeapon.address() + 0x1D0, AttachWeaponToActor_NPC);
				break;
			case 2:
			case 3:
				{
					_AttachWeaponToActor_Player = trampoline.write_call<5>(AttachWeapon.address() + 0x17F, AttachWeaponToActor_Player);
					_AttachWeaponToActor_NPC = trampoline.write_call<5>(AttachWeapon.address() + 0x1D0, AttachWeaponToActor_NPC);
				}
				break;
			default:
				break;
			}
		}

	private:
		static RE::NiAVObject* AttachWeaponToActor_Player(RE::TESModel* a_model, std::int32_t a_slot, RE::TESObjectREFR* a_ref, RE::BipedAnim** a_biped, RE::NiAVObject* a_root3D)
		{
			const auto object = _AttachWeaponToActor_Player(a_model, a_slot, a_ref, a_biped, a_root3D);

			if (a_ref && a_root3D && object) {
				const auto actor = a_ref->As<RE::Actor>();
				if (actor && CanToggleActorEquipment(actor)) {
					auto slot = static_cast<Biped>(a_slot);
					auto [contains, autoToggle, matchingSlot] = GetSlotInfo(weaponSlots, [&](const auto& b_slot) { return b_slot == slot; });
					if (contains && SlotManager::GetData(a_root3D, slot, autoToggle)) {
						object->ToggleNode(true);
					}
				}
			}

			return object;
		}
		static inline REL::Relocation<decltype(AttachWeaponToActor_Player)> _AttachWeaponToActor_Player;

		static RE::NiAVObject* AttachWeaponToActor_NPC(RE::TESModel* a_model, std::int32_t a_slot, RE::TESObjectREFR* a_ref, RE::BipedAnim** a_biped, RE::NiAVObject* a_root3D)
		{
			const auto object = _AttachWeaponToActor_NPC(a_model, a_slot, a_ref, a_biped, a_root3D);

			if (a_ref && a_root3D && object) {
				const auto actor = a_ref->As<RE::Actor>();
				if (actor && CanToggleActorEquipment(actor)) {
					auto slot = static_cast<Biped>(a_slot);
					auto [contains, autoToggle, matchingSlot] = GetSlotInfo(weaponSlots, [&](const auto& b_slot) { return b_slot == slot; });
					if (contains && SlotManager::GetData(a_root3D, slot, autoToggle)) {
						object->ToggleNode(true);
					}
				}
			}

			return object;
		}
		static inline REL::Relocation<decltype(AttachWeaponToActor_NPC)> _AttachWeaponToActor_NPC;
	};

	class HeadHair
	{
	public:
		static void Hook()
		{
			auto& trampoline = SKSE::GetTrampoline();

			REL::Relocation<std::uintptr_t> UpdateHairAndHead{ REL::ID(24220) };
			_GetRootNode = trampoline.write_call<5>(UpdateHairAndHead.address() + 0x1A, GetRootNode);

			REL::Relocation<std::uintptr_t> ProcessArmorDismemberment{ REL::ID(15539) };
			_UpdateDismemberPartion = trampoline.write_call<5>(ProcessArmorDismemberment.address() + 0x70, UpdateDismemberPartion);	 //armor dismemberment function
		}

	private:
		static auto GetRootNode(RE::Actor* a_actor) -> RE::NiAVObject*
		{
			const auto root = _GetRootNode(a_actor);

			if (a_actor && root && CanToggleActorEquipment(a_actor)) {
				auto [contains, autoToggle, matchingSlot] = GetSlotInfo(armorSlots, [&](auto a_slot) { return a_slot == Biped::kHead || a_slot == Biped::kHair || a_slot == Biped::kLongHair; });
				if (contains && SlotManager::GetData(root, matchingSlot, autoToggle)) {
					Dismemberment::UpdateHair(a_actor, a_actor->Get3D(false), true);
					return nullptr;
				}
			}

			return root;
		}
		static inline REL::Relocation<decltype(GetRootNode)> _GetRootNode;


		static void UpdateDismemberPartion(RE::BipedAnim* a_biped, RE::NiAVObject* a_geometry, std::uint32_t a_slot)
		{
			if (a_biped) {
				const auto refPtr = a_biped->actorRef.get();
				if (auto ref = refPtr.get(); ref) {
					const auto actor = ref->As<RE::Actor>();
					if (actor && CanToggleActorEquipment(actor)) {
						auto slot = static_cast<Biped>(a_slot);
						const auto root = actor->GetCurrent3D();
						auto [contains, autoToggle, matchingSlot] = GetSlotInfo(armorSlots, [&slot](const auto& b_slot) { return b_slot == slot; });
						if (contains && root && SlotManager::GetData(root, slot, autoToggle)) {
							Dismemberment::UpdateArmor(actor, a_geometry, a_slot, true);
							return;
						}
					}
				}
			}

			_UpdateDismemberPartion(a_biped, a_geometry, a_slot);
		}
		static inline REL::Relocation<decltype(UpdateDismemberPartion)> _UpdateDismemberPartion;
	};


	void Install()
	{
		if (!armorSlots.empty()) {
			Armor::Hook();
		}
		if (!weaponSlots.empty()) {
			Weapon::Hook();
		}

		auto [contains, autoToggle, hidden] = GetSlotInfo(armorSlots, [&](auto a_slot) { return a_slot == Biped::kHead || a_slot == Biped::kHair || a_slot == Biped::kLongHair; });
		if (contains) {
			HeadHair::Hook();
		}

		logger::info("Hooked armor attach");
	}
}


namespace Combat
{
	static bool playerInCombat = false;

	using COMBAT_STATE = RE::ACTOR_COMBAT_STATE;

	class NPCCombat : public RE::BSTEventSink<RE::TESCombatEvent>
	{
	public:
		using EventResult = RE::BSEventNotifyControl;

		static auto GetSingleton() -> NPCCombat*
		{
			static NPCCombat singleton;
			return &singleton;
		}

	protected:
		auto ProcessEvent(const RE::TESCombatEvent* evn, RE::BSTEventSource<RE::TESCombatEvent>* /*a_eventSource*/) -> EventResult override
		{
			if (!evn) {
				return EventResult::kContinue;
			}

			auto ref = evn->actor.get();
			if (!ref) {
				return EventResult::kContinue;
			}

			const auto actor = ref->As<RE::Actor>();
			if (!actor) {
				return EventResult::kContinue;
			}

			if (!CanToggleActorEquipment(actor)) {
				return EventResult::kContinue;
			}

			switch (*evn->newState) {
			case COMBAT_STATE::kCombat:
				Manager::ToggleActorEquipment(actor, false);
				break;
			case COMBAT_STATE::kNone:
				Manager::ToggleActorEquipment(actor, true);
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

		auto operator=(const NPCCombat&) -> NPCCombat& = delete;
		auto operator=(NPCCombat&&) -> NPCCombat& = delete;
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
		static auto IsInCombat(RE::PlayerCharacter* a_this) -> bool
		{
			const auto result = _IsInCombat(a_this);
			if (result) {
				if (!playerInCombat && a_this) {
					playerInCombat = true;
					Manager::ToggleActorEquipment(a_this, false);
				}
			} else {
				if (playerInCombat && a_this) {
					playerInCombat = false;
					Manager::ToggleActorEquipment(a_this, true);
				}
			}
			return result;
		}

		using IsInCombat_t = decltype(&RE::Actor::IsInCombat);	// 5E
		static inline REL::Relocation<IsInCombat_t> _IsInCombat;
	};


	void Install()
	{
		if (automaticToggleVal != 1) {
			PlayerCombat::Install();
			logger::info("Registered for player combat (unhide during combat)");
		}
		if (automaticToggleVal != 0) {
			auto srcHolder = RE::ScriptEventSourceHolder::GetSingleton();
			if (srcHolder) {
				srcHolder->AddEventSink(NPCCombat::GetSingleton());
				logger::info("Registered for npc combat (unhide during combat)");
			}
		}
	}
}


namespace Location
{
	static bool playerInHouse = false;

	auto IsCellHome(RE::TESObjectCELL* a_cell) -> bool
	{
		if (a_cell->IsInteriorCell()) {
			const auto loc = a_cell->GetLocation();
			if (loc && (loc->HasKeywordString(PlayerHome) || loc->HasKeywordString(Inn))) {
				return true;
			}
		}

		return false;
	}

	class LocChange : public RE::BSTEventSink<RE::BGSActorCellEvent>
	{
	public:
		using EventResult = RE::BSEventNotifyControl;

		static auto GetSingleton() -> LocChange*
		{
			static LocChange singleton;
			return &singleton;
		}

	protected:
		auto ProcessEvent(const RE::BGSActorCellEvent* a_evn, RE::BSTEventSource<RE::BGSActorCellEvent>* /*a_eventSource*/) -> EventResult override
		{
			using CellFlag = RE::BGSActorCellEvent::CellFlag;

			if (!a_evn || a_evn->flags == CellFlag::kLeave) {
				return EventResult::kContinue;
			}

			const auto playerPtr = a_evn->actor.get();
			const auto player = playerPtr.get();

			if (!player) {
				return EventResult::kContinue;
			}

			const auto cell = RE::TESForm::LookupByID<RE::TESObjectCELL>(a_evn->cellID);
			if (!cell) {
				return EventResult::kContinue;
			}

			if (IsCellHome(cell)) {
				playerInHouse = true;

				switch (automaticToggleVal) {
				case 0:
					Manager::ToggleActorEquipment(player, playerInHouse);
					break;
				case 1:
					Manager::ToggleFollowerEquipment(playerInHouse);
					break;
				case 2:
					{
						Manager::ToggleActorEquipment(player, playerInHouse);
						Manager::ToggleFollowerEquipment(playerInHouse);
					}
					break;
				default:
					break;
				}

			} else if (playerInHouse) {
				playerInHouse = false;

				switch (automaticToggleVal) {
				case 0:
					Manager::ToggleActorEquipment(player, playerInHouse);
					break;
				case 1:
					Manager::ToggleFollowerEquipment(playerInHouse);
					break;
				case 2:
					{
						Manager::ToggleActorEquipment(player, playerInHouse);
						Manager::ToggleFollowerEquipment(playerInHouse);
					}
					break;
				default:
					break;
				}
			}

			return EventResult::kContinue;
		}

	private:
		LocChange() = default;
		LocChange(const LocChange&) = delete;
		LocChange(LocChange&&) = delete;
		virtual ~LocChange() = default;

		auto operator=(const LocChange&) -> LocChange& = delete;
		auto operator=(LocChange&&) -> LocChange& = delete;
	};


	void Install()
	{
		if (auto player = RE::PlayerCharacter::GetSingleton(); player) {
			player->AddEventSink(LocChange::GetSingleton());
			logger::info("registered for location change (hide at home)");
		}
	}
}


namespace Hotkey
{
	auto ProcessKey(const Key a_key, SlotKeyVec& a_armor, SlotKeyVec& a_weapon, std::function<void(SlotData& a_slots)> a_func) -> bool
	{
		auto result = false;

		for (auto& [key, data] : a_armor) {
			if (key == a_key) {
				result = true;
				a_func(data);
				break;
			}
		}

		for (auto& [key, data] : a_weapon) {
			if (key == a_key) {
				result = true;
				a_func(data);
				break;
			}
		}

		return result;
	}

	class InputHandler : public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		static auto GetSingleton() -> InputHandler*
		{
			static InputHandler singleton;
			return &singleton;
		}

	protected:
		using EventResult = RE::BSEventNotifyControl;

		auto ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) -> EventResult override
		{
			using InputType = RE::INPUT_EVENT_TYPE;
			using Keyboard = RE::BSWin32KeyboardDevice::Key;

			if (!a_event || RE::UI::IsInMenuMode() || !RE::PlayerCharacter::GetSingleton()->Is3DLoaded()) {
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

				const auto key = static_cast<Key>(button->idCode);
				ProcessKey(key, armorSlots, weaponSlots, [&](auto& a_slotData) {
					switch (hotkeyToggleVal) {
					case 0:
						Manager::ToggleActorEquipment(RE::PlayerCharacter::GetSingleton(), a_slotData);
						break;
					case 1:
						Manager::ToggleFollowerEquipment(a_slotData);
						break;
					case 2:
					case 3:
						{
							Manager::ToggleActorEquipment(RE::PlayerCharacter::GetSingleton(), a_slotData);
							Manager::ToggleFollowerEquipment(a_slotData);
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

		auto operator=(const InputHandler&) -> InputHandler& = delete;
		auto operator=(InputHandler&&) -> InputHandler& = delete;
	};


	void Install()
	{
		auto inputHolder = RE::BSInputDeviceManager::GetSingleton();
		if (inputHolder) {
			inputHolder->AddEventSink(InputHandler::GetSingleton());
			logger::info("Registered input sink (toggle)");
		}
	}
}


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

	auto GetArmorINIData(const std::string& a_value) -> std::optional<SlotKeyData>
	{
		auto sections = STRING::split(a_value, "|");
		if (sections.empty()) {
			return std::nullopt;
		}

		for (auto& str : sections) {
			STRING::trim(str);
		}

		auto key = Key::kNone;
		try {
			auto& keyStr = sections.at(0);
			key = static_cast<Key>(STRING::to_num<std::uint32_t>(keyStr));
			logger::info("		Key : {}", key);
		} catch (...) {
		}

		auto setAutoToggled = true;
		try {
			auto& toggle = sections.at(1);
			setAutoToggled = toggle != "false";
			logger::info("		Auto Toggle : {}", setAutoToggled);
		} catch (...) {
		}

		std::set<Biped> slotSet;
		try {
			auto& slots = sections.at(2);
			if (!slots.empty()) {
				auto vec = STRING::split(slots, " , ");
				for (auto& slotStr : vec) {
					auto slot = STRING::to_num<std::uint32_t>(slotStr);
					switch (slot) {
					case 32:
					case 33:
					case 34:
					case 37:
					case 38:
					case 40:
						{
							logger::info("			Armor : unreplaceable slot {}", slot);
							continue;
						}
					default:
						break;
					}
					if (slot > 61) {
						logger::info("			Armor : invalid slot {}", slot);
						continue;
					}
					logger::info("			Armor : slot {}", slot);
					slotSet.insert(bipedMap.at(slot));
				}
			}
		} catch (...) {
			logger::error("		Slots : unable to parse {}", a_value);
			return std::nullopt;
		}

		return std::make_pair(key, std::make_pair(setAutoToggled, slotSet));
	}

	auto GetWeaponINIData(const std::string& a_value) -> std::optional<SlotKeyData>
	{
		auto sections = STRING::split(a_value, " | ");
		if (sections.empty()) {
			return std::nullopt;
		}

		auto key = Key::kNone;
		try {
			key = static_cast<Key>(STRING::to_num<std::uint32_t>(sections.at(0)));
			logger::info("		Key : {}", key);
		} catch (...) {
		}

		auto setAutoToggled = true;
		try {
			auto& toggle = sections.at(1);
			STRING::trim(toggle);
			setAutoToggled = toggle != "false";
			logger::info("		Auto Toggle : {}", setAutoToggled);
		} catch (...) {
		}

		std::set<Biped> slotSet;
		try {
			if (!sections.at(2).empty()) {
				auto vec = STRING::split(sections.at(2), " , ");
				for (auto& slotStr : vec) {
					logger::info("			Weapon : slot {}", slotStr);
					slotSet.insert(static_cast<Biped>(STRING::to_num<std::uint32_t>(slotStr)));
				}
			}
		} catch (...) {
			logger::error("		Slots : unable to parse {}", a_value);
			return std::nullopt;
		}

		return std::make_pair(key, std::make_pair(setAutoToggled, slotSet));
	}

	void GetDataFromINI(const CSimpleIniA& ini, const char* a_section, const char* a_type, SlotKeyVec& a_INIDataVec, const bool a_armor)
	{
		CSimpleIniA::TNamesDepend values;
		ini.GetAllValues(a_section, a_type, values);
		values.sort(CSimpleIniA::Entry::LoadOrder());

		logger::info("	{} entries found : {}", a_type, values.size());

		a_INIDataVec.reserve(values.size());
		for (auto& value : values) {
			auto data = a_armor ? GetArmorINIData(value.pItem) : GetWeaponINIData(value.pItem);
			if (data.has_value()) {
				a_INIDataVec.emplace_back(data.value());
			}
		}
	}

	auto Read() -> bool
	{
		try {
			const auto pluginPath = SKSE::GetPluginConfigPath("po3_EquipmentToggle");

			CSimpleIniA ini;
			ini.SetUnicode();
			ini.SetMultiKey();

			const auto rc = ini.LoadFile(pluginPath.c_str());
			if (rc < 0) {
				logger::error("Can't load 'po3_EquipmentToggle.ini'");
				return false;
			}

			automaticToggleVal = STRING::to_num<std::int32_t>(ini.GetValue("Settings", "Auto Toggle Type", "0"));
			logger::info("Auto toggle type : {}", automaticToggleVal);
			hotkeyToggleVal = STRING::to_num<std::int32_t>(ini.GetValue("Settings", "Toggle Key Type", "0"));
			logger::info("Hotkey toggle type : {}", hotkeyToggleVal);

			autoUnequip = ini.GetBoolValue("Settings", "Hide When Equipped", true);
			unhideDuringCombat = ini.GetBoolValue("Settings", "Unhide During Combat", false);
			hideAtHome = ini.GetBoolValue("Settings", "Hide At Home", false);

			GetDataFromINI(ini, "Armor", "Armor", armorSlots, true);
			GetDataFromINI(ini, "Weapon", "Weapon", weaponSlots, false);
		} catch (...) {
			logger::error("ini failed");
		}

		return true;
	}
}


void OnInit(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		{
			if (automaticToggleVal != -1) {
				if (unhideDuringCombat) {
					Combat::Install();
				}
				if (hideAtHome) {
					Location::Install();
				}
			}
			if (hotkeyToggleVal != -1) {
				Hotkey::Install();
			}
		}
		break;
	default:
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
		set_default_logger(log);
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

		Init(a_skse);

		INI::Read();

		auto messaging = SKSE::GetMessagingInterface();
		if (!messaging->RegisterListener("SKSE", OnInit)) {
			return false;
		}

		if (automaticToggleVal != -1) {
			SKSE::AllocTrampoline(1 << 7);
			Attach::Install();
			//SaveManager::Install();
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