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
	std::unordered_set<const std::string*> unregisterPathCaches;
};

AfterglowAssetMonitor::AfterglowAssetMonitor() : 
	_context(std::make_unique<Context>()) {
}

AfterglowAssetMonitor::~AfterglowAssetMonitor() {
}

void AfterglowAssetMonitor::update(const LocalClock& clock) {
	// Register / Unregister new assets.
	// DEBUG_COST_INFO_BEGIN("MonitorRegisterCache");
	for (auto& [path, info] : _context->registerAssetCaches) {
		_context->monitoredAssets[path] = info;
	}
	std::erase_if(_context->monitoredAssets, [this](const auto& item){
		return _context->unregisterPathCaches.contains(&(item.first));
	});

	_context->registerAssetCaches.clear();
	_context->unregisterPathCaches.clear();
	// DEBUG_COST_INFO_END;

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

bool AfterglowAssetMonitor::registerAsset(AssetType type, const std::string& path, const TagInfos& tagInfos) {
	if (!std::filesystem::exists(path)) {
		DEBUG_CLASS_WARNING(std::format("Failed to register asset due to the file is not exists: {}", path));
		return false;
	}
	_context->registerAssetCaches[path] = AssetInfo{
		.type = type,
		.tagInfos = tagInfos,
		.lastModifiedTime = std::filesystem::last_write_time(path)
	};
	return  true;
}

bool AfterglowAssetMonitor::unregisterAsset(const std::string& path) {
	auto iterator = _context->monitoredAssets.find(path);
	if (iterator == _context->monitoredAssets.end()) {
		DEBUG_CLASS_WARNING(std::format("Failed to unregister asset due to this path is not registered: {}", path));
		return false;
	}
	_context->unregisterPathCaches.insert(&path);
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
