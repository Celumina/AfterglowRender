#pragma once
// Settings: Global
#define ENABLE_DEBUG_UTILITIES true

// Message info level enums.
#define MESSAGE_LEVEL_ENUM_INFO 4
#define MESSAGE_LEVEL_ENUM_WARNING 3
#define MESSAGE_LEVEL_ENUM_ERROR 2
#define MESSAGE_LEVEL_ENUM_FATAL 1
#define MESSAGE_LEVEL_ENUM_NONE 0

// Settings: Message Level.
#define MESSAGE_LEVEL MESSAGE_LEVEL_ENUM_INFO


// #if ENABLE_DEBUG_UTILITIES
// Here we just use compiler marco, instead of a custom marco.
#ifdef _DEBUG

	#if MESSAGE_LEVEL > MESSAGE_LEVEL_ENUM_NONE
	#include <iostream>
	#include <chrono>
	#include "ASNIColor.h"
	namespace {
		static const std::string infoTag = asniColor::Colorize("[Info]", asniColor::foreBrightBlack);
		static const std::string warningTag = asniColor::Colorize("[Warning]", asniColor::foreYellow);
		static const std::string errorTag = asniColor::Colorize("[Error]", asniColor::foreBrightRed);
		static const std::string fatalTag = asniColor::Colorize("[Fatal]", asniColor::foreRed);
	}
	#endif

	#if MESSAGE_LEVEL >= MESSAGE_LEVEL_ENUM_INFO
	#define DEBUG_INFO(str) std::cout << std::format("{} {}\n", infoTag, (str));
	#define DEBUG_CLASS_INFO(str) std::cout << std::format("{} [{}] {}\n", infoTag, typeid(decltype(*(this))).name(), (str));
	#define DEBUG_TYPE_INFO(type,str) std::cout << std::format("{} [{}] {}\n", infoTag, typeid(type).name(), (str));
	#else 
	#define DEBUG_INFO(str)
	#define DEBUG_CLASS_INFO(str)
	#define DEBUG_TYPE_INFO(str)
	#endif

	#if MESSAGE_LEVEL >= MESSAGE_LEVEL_ENUM_WARNING
	#define DEBUG_WARNING(str) std::cout << std::format("{} {}\n", warningTag, (str));
	#define DEBUG_CLASS_WARNING(str) std::cout << std::format("{} [{}] {}\n", warningTag, typeid(decltype(*(this))).name(), (str));
	#define DEBUG_TYPE_WARNING(type,str) std::cout << std::format("{} [{}] {}\n", warningTag, typeid(type).name(), (str));
	#else 
	#define DEBUG_WARNING(str)
	#define DEBUG_CLASS_WARNING(str) 
	#define DEBUG_TYPE_WARNING(str) 
	#endif

	#if MESSAGE_LEVEL >= MESSAGE_LEVEL_ENUM_ERROR
	#define DEBUG_ERROR(str) std::cout << std::format("{} {}\n", errorTag, (str));
	#define DEBUG_CLASS_ERROR(str) std::cout << std::format("{} [{}] {}\n", errorTag, typeid(decltype(*(this))).name(), (str));
	#define DEBUG_TYPE_ERROR(type,str) std::cout << std::format("{} [{}] {}\n", errorTag, typeid(type).name(), (str));
	#else 
	#define DEBUG_ERROR(str)
	#define DEBUG_CLASS_ERROR(str) 
	#define DEBUG_TYPE_ERROR(str) 
	#endif

	#if MESSAGE_LEVEL >= MESSAGE_LEVEL_ENUM_FATAL
	#define DEBUG_FATAL(str) std::cout << std::format("{} {}\n", fatalTag, (str));
	#define DEBUG_CLASS_FATAL(str) std::cout << std::format("{} [{}] {}\n", fatalTag, typeid(decltype(*(this))).name(), (str));
	#define DEBUG_TYPE_FATAL(type,str) std::cout << std::format("{} [{}] {}\n", fatalTag, typeid(type).name(), (str));
	#else 
	#define DEBUG_FATAL(str)
	#define DEBUG_CLASS_FATAL(str) 
	#define DEBUG_TYPE_FATAL(str) 
	#endif

	
	#define DEBUG_COST_BEGIN(str) \
		{ std::string __DEBUG_infoStr(str); \
		auto __DEBUG_beginTime = std::chrono::high_resolution_clock::now();

	#define DEBUG_COST_END \
		auto __DEBUG_endTime = std::chrono::high_resolution_clock::now(); \
		auto __DEBUG_duration = std::chrono::duration_cast<std::chrono::microseconds>(__DEBUG_endTime - __DEBUG_beginTime).count(); \
		std::cout << std::format("{} [{}]  Elapsed time: {} μs.\n", infoTag, __DEBUG_infoStr, __DEBUG_duration); }

#else
	#define DEBUG_INFO(str)
	#define DEBUG_CLASS_INFO(str)
	#define DEBUG_TYPE_INFO(type,str)
	#define DEBUG_COST_BEGIN(str)
	#define DEBUG_COST_END 
	#define DEBUG_WARNING(str)
	#define DEBUG_CLASS_WARNING(str)
	#define DEBUG_TYPE_WARNING(type,str)
	#define DEBUG_ERROR(str)
	#define DEBUG_CLASS_ERROR(str)
	#define DEBUG_TYPE_ERROR(type,str)
	#define DEBUG_FATAL(str)
	#define DEBUG_CLASS_FATAL(str)
	#define DEBUG_TYPE_FATAL(type,str)
#endif
