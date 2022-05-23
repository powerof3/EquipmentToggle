#include "Serialization.h"

#include <iostream>

namespace Serialization
{
	Manager* Manager::GetSingleton()
	{
		static Manager singleton;
		return &singleton;
	}

	void Manager::Register()
	{
		auto scripts = RE::ScriptEventSourceHolder::GetSingleton();
		if (scripts) {
			scripts->AddEventSink(GetSingleton());
			logger::info("Registered form deletion event handler"sv);
		}
	}

	RE::BSEventNotifyControl Manager::ProcessEvent(const RE::TESFormDeleteEvent* a_event, RE::BSTEventSource<RE::TESFormDeleteEvent>*)
	{
		if (a_event && a_event->formID != 0) {
			Remove(a_event->formID);
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	std::optional<Slot::State> Manager::GetToggleState(RE::Actor* a_actor, Biped a_slot, bool a_firstPerson)
	{
		Locker locker(_lock);
		if (auto ptrIt = _map.find(a_actor->GetFormID()); ptrIt != _map.end()) {
			if (auto slotIt = ptrIt->second.find(a_slot); slotIt != ptrIt->second.end()) {
				return a_firstPerson ? slotIt->second.firstPerson : slotIt->second.thirdPerson;
			}
		}
		return std::nullopt;
	}

	void Manager::Add(RE::Actor* a_actor, Biped a_slot, Slot::State a_toggleState, bool a_firstPerson)
	{
		Locker locker(_lock);
		if (a_firstPerson) {
			_map[a_actor->GetFormID()][a_slot].firstPerson = a_toggleState;
		} else {
			_map[a_actor->GetFormID()][a_slot].thirdPerson = a_toggleState;
		}
	}

	bool Manager::Remove(RE::FormID a_formID)
	{
		Locker locker(_lock);
		return _map.erase(a_formID) != 0;
	}

	void Manager::Clear()
	{
		_savedModIndexMap.clear();

		Locker locker(_lock);
		_map.clear();
	}

	void Manager::SavePluginList(nlohmann::ordered_json& a_intfc)
	{
		if (const auto dataHandler = RE::TESDataHandler::GetSingleton(); dataHandler) {
			for (auto& mod : dataHandler->files) {
				if (auto compileIndex = mod ? mod->GetCompileIndex() : 0xFF; compileIndex != 0xFF) {
					auto& j_plugin = a_intfc[mod->GetFilename().data()];
					if (compileIndex == 0xFE) {
						j_plugin["smallFileCompileIndex"] = mod->GetSmallFileCompileIndex();
					} else {
						j_plugin["compileIndex"] = compileIndex;
					}
				}
			}
		}
	}

	void Manager::LoadPluginList(const nlohmann::ordered_json& a_intfc)
	{
		_savedModIndexMap.clear();

		if (const auto dataHandler = RE::TESDataHandler::GetSingleton(); dataHandler) {
			for (auto& [modName, compileIndices] : a_intfc.items()) {
				std::uint32_t oldIndex = 0xFF;
				std::uint32_t newIndex = 0xFF;

				if (auto it = compileIndices.find("smallFileCompileIndex"); it != compileIndices.end()) {
					const auto smallFileCompileIndex = it->get<std::uint32_t>();
					oldIndex = (0xFE000 | smallFileCompileIndex);
				} else {
					oldIndex = compileIndices["compileIndex"].get<std::uint32_t>();
				}

				if (const auto mod = dataHandler->LookupModByName(modName); mod) {
					newIndex = mod->GetPartialIndex();
				}

				_savedModIndexMap[oldIndex] = newIndex;
			}
		}
	}

	bool Manager::ResolveFormID(RE::FormID a_formIDIn, RE::FormID& a_formIDOut)
	{
		auto modIndex = a_formIDIn >> 24;
		if (modIndex == 0xFF) {
			a_formIDOut = a_formIDIn;
			return true;
		}

		if (modIndex == 0xFE) {
			modIndex = a_formIDIn >> 12;
		}

		std::uint32_t loadedModIndex = 0xFF;
		if (const auto it = _savedModIndexMap.find(modIndex); it != _savedModIndexMap.end()) {
			loadedModIndex = it->second;
		}

		if (loadedModIndex < 0xFF) {
			a_formIDOut = (a_formIDIn & 0x00FFFFFF) | (loadedModIndex << 24);
			return true;
		} else if (loadedModIndex > 0xFF) {
			a_formIDOut = (loadedModIndex << 12) | (a_formIDIn & 0x00000FFF);
			return true;
		}

		return false;
	}

	bool Manager::Save(nlohmann::ordered_json& a_intfc)
	{
		Locker locker(_lock);

		for (auto& [formID, slots] : _map) {
			auto& j_actor = a_intfc[fmt::format("0x{:X}", formID)];
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

	bool Manager::Load(const nlohmann::ordered_json& a_intfc)
	{
		Locker locker(_lock);

		_map.clear();

		for (auto& [j_formID, j_slots] : a_intfc.items()) {
			auto formID = string::lexical_cast<RE::FormID>(j_formID, true);
			if (ResolveFormID(formID, formID)) {
				for (const auto& [j_slot, j_state] : j_slots.items()) {
					auto slot = static_cast<Biped>(string::lexical_cast<std::uint32_t>(j_slot));

					ToggleState state{ std::nullopt, std::nullopt };
					if (auto it = j_state.find("1stPerson"); it != j_state.end()) {
						state.firstPerson = it->get<Slot::State>();
					}
					state.thirdPerson = j_state["3rdPerson"].get<Slot::State>();

					_map[formID].insert_or_assign(slot, state);
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
			nlohmann::ordered_json jsonOut;

			Manager::GetSingleton()->SavePluginList(jsonOut["pluginList"]);
			Manager::GetSingleton()->Save(jsonOut["slotList"]);
			jsonOut["Version"] = kSerializationVersion;

			ofs << std::setw(4) << jsonOut;
		}
		ofs.close();

		logger::info("Saving data to {}.json", a_savePath);
	}

	void Load(const std::string& a_savePath)
	{
		const auto path = fmt::format(filePath, a_savePath);

		std::ifstream ifs(path);
		if (ifs.is_open()) {
			nlohmann::ordered_json jsonIn = nlohmann::ordered_json::parse(ifs);

			auto version = jsonIn["Version"].get<std::uint32_t>();
			if (version != kSerializationVersion) {
				Manager::GetSingleton()->Clear();
				logger::critical("{} : expected {}, got {}", a_savePath, kSerializationVersion, version);
				return;
			}

			Manager::GetSingleton()->LoadPluginList(jsonIn["pluginList"]);
			Manager::GetSingleton()->Load(jsonIn["slotList"]);

			logger::info("Loading data from {}.json", a_savePath);
		}
		ifs.close();
	}

	void Delete(const std::string& a_savePath)
	{
		const auto path = fmt::format(filePath, a_savePath);
		std::filesystem::remove(path);
	}

	void ClearUnreferencedSlotData()
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
						logger::info("Cleaning up unreferenced {} slot data", saveFileName);
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

		Manager::GetSingleton()->Add(a_actor, slot, a_state, a_firstPerson);
	}

	Slot::State GetToggleState(RE::Actor* a_actor, const Biped a_slot, bool a_firstPerson)
	{
		Biped slot;
		if (headSlots.find(a_slot) != headSlots.end()) {
			slot = Biped::kHead;
		} else {
			slot = a_slot;
		}

		if (const auto state = Manager::GetSingleton()->GetToggleState(a_actor, slot, a_firstPerson); state) {
			return *state;
		} else {
			Manager::GetSingleton()->Add(a_actor, slot, Slot::State::kHide, a_firstPerson);
			return Slot::State::kHide;
		}
	}
}
