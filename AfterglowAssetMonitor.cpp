#include "AfterglowAssetMonitor.h"

#include <filesystem>
#include <unordered_set>

#include "DebugUtilities.h"

struct AfterglowAssetMonitor::Context {
	// Unit: second.
	float checkInterval = 1.0f;
	float lastCheckTime = 0.0f;
	MonitoredAssets monitoredAssets;

	ModifiedCallbacks modifiedCallbacks;
	DeletedCallbacks deletedCallbacks;

	// Handling register / unregister asset were happened inside these callbacks.
	MonitoredAssets registerAssetCaches;
	std::unordered_set<std::string> unregisterPathCaches;

	void unloadRegisterCaches();
};

AfterglowAssetMonitor::AfterglowAssetMonitor() : 
	_context(std::make_unique<Context>()) {
}

AfterglowAssetMonitor::~AfterglowAssetMonitor() {
}

void AfterglowAssetMonitor::update(const LocalClock& clock) {
	_context->unloadRegisterCaches();

	// Fixed interval modified callbacks.
	if (clock.timeSec() - _context->lastCheckTime < _context->checkInterval) {
		return;
	}
	_context->lastCheckTime = static_cast<float>(clock.timeSec());

	for (auto& [path, info] : _context->monitoredAssets) {
		if (!std::filesystem::exists(path)) {
		auto& deletedCallback = _context->deletedCallbacks[info.type];
			if (deletedCallback) {
				deletedCallback(path, info.tagInfos);
			}
			unregisterAsset(path);
			continue;
		}
		auto modifiedTime = std::filesystem::last_write_time(path);
		if (modifiedTime != info.lastModifiedTime) {
			auto& modifiedCallback = _context->modifiedCallbacks[info.type];
			if (modifiedCallback) {
				modifiedCallback(path, info.tagInfos);
			}
			info.lastModifiedTime = modifiedTime;
		}
	}
}

AfterglowAssetMonitor::AssetInfo* AfterglowAssetMonitor::registerAsset(AssetType type, const std::string& path, const TagInfos& tagInfos) {
	if (!std::filesystem::exists(path)) {
		DEBUG_CLASS_WARNING(std::format("Failed to register asset due to the file is not exists: {}", path));
		return nullptr;
	}
	return &_context->registerAssetCaches.emplace(
		path, 
		AssetInfo {
			.type = type,
			.tagInfos = tagInfos,
			.lastModifiedTime = std::filesystem::last_write_time(path)
		}
	).first->second;
}

bool AfterglowAssetMonitor::unregisterAsset(const std::string& path) {
	auto iterator = _context->monitoredAssets.find(path);
	if (iterator == _context->monitoredAssets.end()) {
		DEBUG_CLASS_WARNING(std::format("Failed to unregister asset due to this path is not registered: {}", path));
		return false;
	}
	_context->unregisterPathCaches.insert(path);
	return  true;
}

void AfterglowAssetMonitor::setCheckInterval(float sec) {
	_context->checkInterval = sec;
}

void AfterglowAssetMonitor::registerModifiedCallback(AssetType type, std::function<ModifiedCallback> callback) {
	_context->modifiedCallbacks[type] = callback;
}

void AfterglowAssetMonitor::registerDeletedCallback(AssetType type, std::function<DeletedCallback> callback) {
	_context->deletedCallbacks[type] = callback;
}

void AfterglowAssetMonitor::unregisterModifiedCallback(AssetType type) {
	_context->modifiedCallbacks.erase(type);
}

void AfterglowAssetMonitor::unregisterDeleteCallback(AssetType type) {
	_context->deletedCallbacks.erase(type);
}

AfterglowAssetMonitor::AssetInfo* AfterglowAssetMonitor::registeredAssetInfo(const std::string& path) {
	auto iterator = _context->monitoredAssets.find(path);
	// Excluding asset info which will be unregistered.
	if (iterator != _context->monitoredAssets.end() && !_context->unregisterPathCaches.contains(path)) {
		return &iterator->second;
	}
	return nullptr;
}

const AfterglowAssetMonitor::AssetInfo* AfterglowAssetMonitor::registeredAssetInfo(const std::string& path) const {
	return const_cast<AfterglowAssetMonitor*>(this)->registeredAssetInfo(path);
}

void AfterglowAssetMonitor::Context::unloadRegisterCaches() {
	// Register / Unregister new assets.
	// DEBUG_COST_INFO_BEGIN("MonitorRegisterCache");
	std::erase_if(monitoredAssets, [this](const auto& item) {
		bool shouldUnregister = unregisterPathCaches.contains(item.first);
		if (shouldUnregister) {
			DEBUG_CLASS_INFO(std::format("Asset is unregistered: {}", item.first));
		}
		return shouldUnregister;
	});

	for (auto& [path, info] : registerAssetCaches) {
		DEBUG_CLASS_INFO(std::format("Asset is registered: {}", path));
		// TODO: Check exists to prevent overwrite
		// TODO: Due to shared material only update one of them when the shader was updated.
		monitoredAssets[path] = info;
	}
	registerAssetCaches.clear();
	unregisterPathCaches.clear();
	// DEBUG_COST_INFO_END;
}
