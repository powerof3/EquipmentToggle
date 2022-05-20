#pragma once

using Biped = RE::BIPED_OBJECT;
using BipedSlot = RE::BIPED_MODEL::BipedObjectSlot;
using Key = RE::BSKeyboardDevice::Key;
using HeadPart = RE::BGSHeadPart::HeadPartType;

using SlotSet = std::set<Biped>;

inline auto constexpr NPC{ "ActorTypeNPC"sv };
inline auto constexpr PlayerHome{ "LocTypePlayerHouse"sv };
inline auto constexpr Inn{ "LocTypeInn"sv };

inline constexpr frozen::map<std::uint32_t, Biped, Biped::kEditorTotal> bipedMap{
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

inline constexpr frozen::map<std::uint32_t, BipedSlot, Biped::kEditorTotal> slotMap = {
	{ 30, BipedSlot::kHead },
	{ 31, BipedSlot::kHair },
	{ 32, BipedSlot::kBody },
	{ 33, BipedSlot::kHands },
	{ 34, BipedSlot::kForearms },
	{ 35, BipedSlot::kAmulet },
	{ 36, BipedSlot::kRing },
	{ 37, BipedSlot::kFeet },
	{ 38, BipedSlot::kCalves },
	{ 39, BipedSlot::kShield },
	{ 40, BipedSlot::kTail },
	{ 41, BipedSlot::kLongHair },
	{ 42, BipedSlot::kCirclet },
	{ 43, BipedSlot::kEars },
	{ 44, BipedSlot::kModMouth },
	{ 45, BipedSlot::kModNeck },
	{ 46, BipedSlot::kModChestPrimary },
	{ 47, BipedSlot::kModBack },
	{ 48, BipedSlot::kModMisc1 },
	{ 49, BipedSlot::kModPelvisPrimary },
	{ 50, BipedSlot::kDecapitateHead },
	{ 51, BipedSlot::kDecapitate },
	{ 52, BipedSlot::kModPelvisSecondary },
	{ 53, BipedSlot::kModLegRight },
	{ 54, BipedSlot::kModLegLeft },
	{ 55, BipedSlot::kModFaceJewelry },
	{ 56, BipedSlot::kModChestSecondary },
	{ 57, BipedSlot::kModShoulder },
	{ 58, BipedSlot::kModArmLeft },
	{ 59, BipedSlot::kModArmRight },
	{ 60, BipedSlot::kModMisc2 },
	{ 61, BipedSlot::kFX01 },
};

inline constexpr frozen::set<Biped, 5> headSlots{
    Biped::kHead,
    Biped::kHair,
    Biped::kLongHair,
    Biped::kEars,
    Biped::kDecapitateHead
};
