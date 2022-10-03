#include "Events.h"
#include "Hooks.h"
#include "Serialization.h"
#include "Settings.h"

void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
	static bool result = false;

	switch (a_message->type) {
	case SKSE::MessagingInterface::kPostLoad:
		{
			if (result = Settings::GetSingleton()->LoadSettings(); result) {
				SKSE::AllocTrampoline(112);
				Hooks::Install();
			}
		}
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		{
			if (result) {
				Events::Manager::Register();
				Events::AnimationManager::Register();
				Serialization::Manager::Register();
			} else {
				if (const auto console = RE::ConsoleLog::GetSingleton(); console) {
					console->Print("[Equipment Toggle] Unable to parse config file! Use a JSON validator to fix any errors\n");
				}
			}

			Serialization::ClearUnreferencedSlotData();
		}
		break;
	case SKSE::MessagingInterface::kSaveGame:
		{
			std::string savePath = { static_cast<char*>(a_message->data), a_message->dataLen };
			Serialization::Save(savePath);
		}
		break;
	case SKSE::MessagingInterface::kPreLoadGame:
		{
			std::string savePath({ static_cast<char*>(a_message->data), a_message->dataLen });
			string::replace_last_instance(savePath, ".ess", "");
			Serialization::Load(savePath);
		}
		break;
	case SKSE::MessagingInterface::kDeleteGame:
		{
			std::string savePath({ static_cast<char*>(a_message->data), a_message->dataLen });
			Serialization::Delete(savePath);
		}
		break;
	default:
		break;
	}
}

#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::MAJOR);
	v.PluginName("Equipment Toggle");
	v.AuthorName("powerofthree");
	v.UsesAddressLibrary();
	v.UsesUpdatedStructs();
	v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

	return v;
}();
#else
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "Equipment Toggle";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}
#endif

void InitializeLog()
{
	auto path = logger::log_directory();
	if (!path) {
		stl::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();

	logger::info("Game version : {}", a_skse->RuntimeVersion().string());

	SKSE::Init(a_skse);

	const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(MessageHandler);

	return true;
}
