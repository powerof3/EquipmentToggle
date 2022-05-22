#include "Serialization.h"

#include <iostream>

namespace Serialization
{
	std::optional<Slot::State> AutoToggleMap::GetToggleState(RE::Actor* a_actor, Biped a_slot, bool a_firstPerson)
	{
		Locker locker(_lock);
	    if (auto ptrIt = _map.find(a_actor->CreateRefHandle().native_handle()); ptrIt != _map.end()) {
			if (auto slotIt = ptrIt->second.find(a_slot); slotIt != ptrIt->second.end()) {
				return a_firstPerson ? slotIt->second.firstPerson : slotIt->second.thirdPerson;
			}
		}
		return std::nullopt;
	}

    void AutoToggleMap::Add(RE::Actor* a_actor, Biped a_slot, Slot::State a_toggleState, bool a_firstPerson)
	{
		Locker locker(_lock);
		if (a_firstPerson) {
			_map[a_actor->CreateRefHandle().native_handle()][a_slot].firstPerson = a_toggleState;
		} else {
			_map[a_actor->CreateRefHandle().native_handle()][a_slot].thirdPerson = a_toggleState;
		}
	}

	bool AutoToggleMap::Remove(RE::Actor* a_actor)
	{
		Locker locker(_lock);
		return _map.erase(a_actor->CreateRefHandle().native_handle()) != 0;
	}

	void AutoToggleMap::Clear()
	{
		Locker locker(_lock);
		_map.clear();
	}

    bool AutoToggleMap::Save(nlohmann::json& a_intfc)
    {
		assert(a_intfc);
		Locker locker(_lock);

		a_intfc["Version"] = kSerializationVersion;

		for (auto& [refHandle, slots] : _map) {
			auto& j_actor = a_intfc[std::to_string(refHandle)];
			for (const auto& [slot, state] : slots) {
				auto& j_Slot = j_actor[std::to_string(slot)];

                if (auto fP = state.firstPerson ? stl::to_underlying(*state.firstPerson) : -1; fP != -1) {
					j_Slot["1stPerson"] = fP; 
				}
				j_Slot["3rdPerson"] = state.thirdPerson ? stl::to_underlying(*state.thirdPerson) : -1;
			}
		}

		return true;
    }

    bool AutoToggleMap::Load(const nlohmann::json& a_intfc)
    {
		assert(a_intfc);
		Locker locker(_lock);

		_map.clear();

	    for (auto& [j_refHandle, j_slots] : a_intfc.items()) {
			if (j_slots.is_object()) {
				auto refHandle = string::lexical_cast<RE::RefHandle>(j_refHandle);

				for (const auto& [j_slot, j_state] : j_slots.items()) {
					auto slot = static_cast<Biped>(string::lexical_cast<std::uint32_t>(j_slot));

					ToggleState state{ std::nullopt, std::nullopt };
					if (auto it = j_state.find("1stPerson"); it != j_state.end()) {
						state.firstPerson = it->get<Slot::State>();
					}
					state.thirdPerson = j_state["3rdPerson"].get<Slot::State>();

					_map[refHandle].insert_or_assign(slot, state);
				}
			}
		}

		return true;
    }

    void Save(const std::string& a_savePath)
	{
		const auto path = fmt::format(filePath, a_savePath);

	    std::ofstream ofs(path);
		if (ofs.is_open()) {
			nlohmann::json jsonOut;
			AutoToggleMap::GetSingleton()->Save(jsonOut);
			ofs << std::setw(4) << jsonOut;
		}
		ofs.close();

		logger::info("Saving slot data to {}.json", a_savePath);
	}

    void Load(const std::string& a_savePath)
	{
		const auto path = fmt::format(filePath, a_savePath);

		std::ifstream ifs(path);
		if (ifs.is_open()) {
			nlohmann::json jsonIn = nlohmann::json::parse(ifs);

			auto version = jsonIn["Version"].get<std::uint32_t>();
			if (version != kSerializationVersion) {
				AutoToggleMap::GetSingleton()->Clear();
			    logger::critical("{} : expected {}, got {}", a_savePath, kSerializationVersion, version);
				return;
			}

		    AutoToggleMap::GetSingleton()->Load(jsonIn);

			logger::info("Loading slot data from {}.json", a_savePath);
		}
		ifs.close();
	}

    void Delete(const std::string& a_savePath)
	{
		const auto path = fmt::format(filePath, a_savePath);
		std::filesystem::remove(path);
	}

    void ClearUnreferencedSaveData()
	{
		constexpr auto get_save_directory = []() -> std::optional<std::filesystem::path> {
			wchar_t* buffer{ nullptr };
			const auto result = SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, nullptr, std::addressof(buffer));
			std::unique_ptr<wchar_t[], decltype(&::CoTaskMemFree)> knownPath(buffer, ::CoTaskMemFree);
			if (!knownPath || result != S_OK) {
				logger::error("failed to get My Documents path"sv);
				return std::nullopt;
			}

			std::filesystem::path path = knownPath.get();
			path /= "My Games/Skyrim Special Edition/"sv;
			path /= RE::INISettingCollection::GetSingleton()->GetSetting("sLocalSavePath:General")->GetString();
			return path;
		};

		const auto saveDir = get_save_directory();
		if (!saveDir) {
			return;
		}

		logger::info("{:*^30}", "SERIALIZATION");

		std::filesystem::directory_entry slotDataDir{ folderPath };
		if (!slotDataDir.exists()) {
			std::filesystem::create_directory(slotDataDir);
		} else {
			for (const auto& entry : std::filesystem::directory_iterator(slotDataDir)) {
				if (entry.exists() && !entry.path().empty() && entry.path().extension() == ".json"sv) {
					auto saveFileName = entry.path().stem().string();
					auto savePath = fmt::format("{}{}.ess", saveDir->string(), saveFileName);
					if (!std::filesystem::exists(savePath)) {
						logger::info("Cleaning up unreferenced {} save data", saveFileName);
						const auto serializedSlotsPath = fmt::format(filePath, saveFileName);
						std::filesystem::remove(serializedSlotsPath);
					}
				}
			}
		}
	}

    void SetToggleState(RE::Actor* a_actor, const Biped a_slot, Slot::State a_state, bool a_firstPerson)
	{
		Biped slot;
		if (headSlots.find(a_slot) != headSlots.end()) {
			slot = Biped::kHead;
		} else {
			slot = a_slot;
		}

		AutoToggleMap::GetSingleton()->Add(a_actor, slot, a_state, a_firstPerson);
	}

	Slot::State GetToggleState(RE::Actor* a_actor, const Biped a_slot, bool a_firstPerson)
	{
		Biped slot;
		if (headSlots.find(a_slot) != headSlots.end()) {
			slot = Biped::kHead;
		} else {
			slot = a_slot;
		}

		if (const auto state = AutoToggleMap::GetSingleton()->GetToggleState(a_actor, slot, a_firstPerson); state) {
		    return *state;
		} else {
		    AutoToggleMap::GetSingleton()->Add(a_actor, slot, Slot::State::kHide, a_firstPerson);
			return Slot::State::kHide;
		}
	}
}
