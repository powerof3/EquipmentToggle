#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include <xbyak/xbyak.h>  // must be between these two

#include <SimpleIni.h>
#include <frozen/map.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <shared_mutex>

#ifndef NDEBUG
#include <spdlog/sinks/msvc_sink.h>
#endif

namespace logger = SKSE::log;
using namespace SKSE::util;
using namespace std::string_view_literals;

namespace stl
{
	using nonstd::span;
	using SKSE::stl::report_and_fail;
}

using Biped = RE::BIPED_OBJECT;
using Slot = RE::BIPED_MODEL::BipedObjectSlot;
using Key = RE::BSKeyboardDevice::Key;
using HeadPart = RE::BGSHeadPart::HeadPartType;

#define DLLEXPORT __declspec(dllexport)
