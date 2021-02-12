#pragma once

inline constexpr frozen::map<std::uint16_t, Slot, 31> slotMap = {
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

class Dismemberment
{
public:
	static void UpdateHeadPart(RE::Actor* a_actor, RE::NiAVObject* a_root, RE::TESObjectARMA* a_arma, HeadPart a_type, bool a_hide);
	static void UpdateHair(RE::Actor* a_actor, RE::NiAVObject* a_root, bool a_hide);
	static void UpdateArmor(RE::Actor* a_actor, RE::NiAVObject* a_root, std::uint32_t a_slot, bool a_hide);

private:
	static void TogglePartition(RE::BSGeometry& a_shape, RE::TESObjectARMA& a_arma, bool a_hide);
	static void TogglePartition(RE::BSGeometry& a_shape, RE::TESRace& a_race, bool a_hide);
	static void TogglePartition(RE::BSGeometry& a_shape, bool a_hide);

	static void ToggleExtraParts(const RE::BSTArray<RE::BGSHeadPart*>& a_parts, RE::NiAVObject& a_root, RE::TESObjectARMA& a_arma, bool a_hide);
	static void ToggleExtraParts(const RE::BSTArray<RE::BGSHeadPart*>& a_parts, RE::NiAVObject& a_root, RE::TESRace& a_race, bool a_hide);
	static void ToggleExtraParts(const RE::BSTArray<RE::BGSHeadPart*>& a_parts, RE::NiAVObject& a_root, bool a_hide);
};