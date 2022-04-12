#pragma once

using Biped = RE::BIPED_OBJECT;
using Slot = RE::BIPED_MODEL::BipedObjectSlot;
using Key = RE::BSKeyboardDevice::Key;
using HeadPart = RE::BGSHeadPart::HeadPartType;

using SlotData = std::pair<bool, std::set<Biped>>;
using SlotKeyData = std::pair<Key, SlotData>;
using SlotKeyVec = std::vector<SlotKeyData>;

inline auto constexpr NPC{ "ActorTypeNPC"sv };
inline auto constexpr PlayerHome{ "LocTypePlayerHouse"sv };
inline auto constexpr Inn{ "LocTypeInn"sv };

inline auto constexpr HeadData{ "EquipToggle:Head" };

inline constexpr frozen::map<std::uint32_t, Biped, 31> bipedMap{
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

inline constexpr frozen::map<std::uint32_t, Slot, 31> slotMap = {
	{ 30, Slot::kHead },
	{ 31, Slot::kHair },
	{ 32, Slot::kBody },
	{ 33, Slot::kHands },
	{ 34, Slot::kForearms },
	{ 35, Slot::kAmulet },
	{ 36, Slot::kRing },
	{ 37, Slot::kFeet },
	{ 38, Slot::kCalves },
	{ 39, Slot::kShield },
	{ 40, Slot::kTail },
	{ 41, Slot::kLongHair },
	{ 42, Slot::kCirclet },
	{ 43, Slot::kEars },
	{ 44, Slot::kModMouth },
	{ 45, Slot::kModNeck },
	{ 46, Slot::kModChestPrimary },
	{ 47, Slot::kModBack },
	{ 48, Slot::kModMisc1 },
	{ 49, Slot::kModPelvisPrimary },
	{ 50, Slot::kDecapitateHead },
	{ 51, Slot::kDecapitate },
	{ 52, Slot::kModPelvisSecondary },
	{ 53, Slot::kModLegRight },
	{ 54, Slot::kModLegLeft },
	{ 55, Slot::kModFaceJewelry },
	{ 56, Slot::kModChestSecondary },
	{ 57, Slot::kModShoulder },
	{ 58, Slot::kModArmLeft },
	{ 59, Slot::kModArmRight },
	{ 60, Slot::kModMisc2 },
	{ 61, Slot::kFX01 },
};

inline constexpr frozen::set<Biped, 5> headSlots{ Biped::kHead, Biped::kHair, Biped::kLongHair, Biped::kEars, Biped::kDecapitateHead };

inline SlotKeyVec armorSlots;
inline SlotKeyVec weaponSlots;
