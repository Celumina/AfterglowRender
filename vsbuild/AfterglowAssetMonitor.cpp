#include "AfterglowAssetMonitor.h"

#include <filesystem>
#include <unordered_set>
#include <thread>
#include <mutex>

#include "AfterglowTicker.h"
#include "LocalClock.h"
#include "DebugUtilities.h"

struct AfterglowAssetMonitor::Impl {
	Impl(AfterglowAssetMonitor& inMonitor) : monitor(inMonitor) {}

	void startAssetThread();
	void stopAssetThread();

	void monitorAssetLoop(std::stop_token stopToken);
	void recordModifiedAssets();

	void applyModifiedCaches();
	void applyRegisterCaches();

	AfterglowAssetMonitor& monitor;

	// Unit seconds.
	// @note: minimum interval 0.0333f for 30 fps update.
	float checkInterval = 0.1f;
	float lastCheckTime = 0.0f;
	MonitoredAssets monitoredAssets;

	ModifiedCallbacks modifiedCallbacks;
	DeletedCallbacks deletedCallbacks;

	// Handling register / unregister asset were happened inside these callbacks.
	MonitoredAssets registerAssetCaches;
	std::unordered_set<std::string> unregisterPathCaches;
	std::mutex externalThreadMutex;

	// Independent thread inside this class only. 
	std::unique_ptr<std::jthread> assetThread;
	std::unordered_set<const std::string*> deletedPathCaches;
	std::unordered_set<const std::string*> modifiedPathCaches;
	std::mutex assetThreadMutex;

	AfterglowTicker ticker;
};

AfterglowAssetMonitor::AfterglowAssetMonitor() : 
	_impl(std::make_unique<Impl>(*this)) {
	_impl->ticker.setMaximumFPS(30.0f);
	_impl->startAssetThread();
}

AfterglowAssetMonitor::~AfterglowAssetMonitor() {
	_impl->stopAssetThread();
}

void AfterglowAssetMonitor::update() {
	_impl->applyModifiedCaches();
	_impl->applyRegisterCaches();
}

AfterglowAssetMonitor::AssetInfo* AfterglowAssetMonitor::registerAsset(AssetType type, const std::string& path, const TagInfos& tagInfos) {
	if (!std::filesystem::exists(path)) {
		DEBUG_CLASS_WARNING(std::format("Failed to register asset due to the file is not exists: {}", path));
		return nullptr;
	}
	std::lock_guard lock{ _impl->externalThreadMutex };
	return &_impl->registerAssetCaches.emplace(
		path, 
		AssetInfo {
			.type = type,
			.tagInfos = tagInfos,
			.lastModifiedTime = std::filesystem::last_write_time(path)
		}
	).first->second;
}

bool AfterglowAssetMonitor::unregisterAsset(const std::string& path) {
	std::lock_guard lock{ _impl->externalThreadMutex };
	auto iterator = _impl->monitoredAssets.find(path);
	if (iterator == _impl->monitoredAssets.end()) {
		DEBUG_CLASS_WARNING(std::format("Failed to unregister asset due to this path is not registered: {}", path));
		return false;
	}
	_impl->unregisterPathCaches.insert(path);
	return  true;
}

void AfterglowAssetMonitor::setCheckInterval(float sec) {
	_impl->checkInterval = sec;
}

void AfterglowAssetMonitor::registerModifiedCallback(AssetType type, std::function<ModifiedCallback> callback) {
	_impl->modifiedCallbacks[type] = callback;
}

void AfterglowAssetMonitor::registerDeletedCallback(AssetType type, std::function<DeletedCallback> callback) {
	_impl->deletedCallbacks[type] = callback;
}

void AfterglowAssetMonitor::unregisterModifiedCallback(AssetType type) {
	_impl->modifiedCallbacks.erase(type);
}

void AfterglowAssetMonitor::unregisterDeleteCallback(AssetType type) {
	_impl->deletedCallbacks.erase(type);
}

AfterglowAssetMonitor::AssetInfo* AfterglowAssetMonitor::registeredAssetInfo(const std::string& path) {
	auto iterator = _impl->monitoredAssets.find(path);
	// Excluding asset info which will be unregistered.
	if (iterator != _impl->monitoredAssets.end() && !_impl->unregisterPathCaches.contains(path)) {
		return &iterator->second;
	}
	return nullptr;
}

const AfterglowAssetMonitor::AssetInfo* AfterglowAssetMonitor::registeredAssetInfo(const std::string& path) const {
	return const_cast<AfterglowAssetMonitor*>(this)->registeredAssetInfo(path);
}

void AfterglowAssetMonitor::Impl::startAssetThread() {
	if (!assetThread) {
		assetThread = std::make_unique<std::jthread>([this](std::stop_token stopToken){ monitorAssetLoop(stopToken); });
	}
}

void AfterglowAssetMonitor::Impl::stopAssetThread() {
	assetThread.reset();
}

void AfterglowAssetMonitor::Impl::monitorAssetLoop(std::stop_token stopToken) {
	while(!stopToken.stop_requested()) {
		recordModifiedAssets();
	}
}

void AfterglowAssetMonitor::Impl::recordModifiedAssets() {
	ticker.tick();

	// Fixed interval modified callbacks.
	if (ticker.clock().timeSec() - lastCheckTime < checkInterval) {
		return;
	}
	lastCheckTime = static_cast<float>(ticker.clock().timeSec());

	std::unordered_set<const std::string*> tempDeletedPathCaches;
	std::unordered_set<const std::string*> tempModifiedPathCaches;

	for (auto& [path, info] : monitoredAssets) {
		if (!std::filesystem::exists(path)) {
			tempDeletedPathCaches.insert(&path);
			continue;
		}
		auto modifiedTime = std::filesystem::last_write_time(path);
		if (modifiedTime != info.lastModifiedTime) {
			tempModifiedPathCaches.insert(&path);
			info.lastModifiedTime = modifiedTime;
		}
	}

	std::lock_guard lock{ assetThreadMutex };
	deletedPathCaches = std::move(tempDeletedPathCaches);
	modifiedPathCaches = std::move(tempModifiedPathCaches);
}

void AfterglowAssetMonitor::Impl::applyModifiedCaches() {
	std::lock_guard lock{ assetThreadMutex };

	for (const auto* path : deletedPathCaches) {
		auto& info = monitoredAssets[*path];
		auto& deletedCallback = deletedCallbacks[info.type];
		if (deletedCallback) {
			deletedCallback(*path, info.tagInfos);
		}
		monitor.unregisterAsset(*path);
	}
	for (const auto* path : modifiedPathCaches) {
		auto& info = monitoredAssets[*path];
		auto& modifiedCallback = modifiedCallbacks[info.type];
		if (modifiedCallback) {
			modifiedCallback(*path, info.tagInfos);
		}
	}
	deletedPathCaches.clear();
	modifiedPathCaches.clear();
}

void AfterglowAssetMonitor::Impl::applyRegisterCaches() {
	std::lock_guard lock{ externalThreadMutex };
	// Register / Unregister new assets.
	//DEBUG_COST_BEGIN("MonitorRegisterCache");
	 if (!unregisterPathCaches.empty()) {
		std::erase_if(monitoredAssets, [this](const auto& item) {
			 bool shouldUnregister = unregisterPathCaches.contains(item.first);
			 if (shouldUnregister) {
				 DEBUG_CLASS_INFO(std::format("Asset is unregistered: {}", item.first));
			 }
			 return shouldUnregister;
		});
	 }

	for (auto& [path, info] : registerAssetCaches) {
		DEBUG_CLASS_INFO(std::format("Asset is registered: {}", path));
		// TODO: Check exists to prevent overwrite
		// TODO: Due to shared material only update one of them when the shader was updated.
		monitoredAssets[path] = info;
	}
	registerAssetCaches.clear();
	unregisterPathCaches.clear();
	//DEBUG_COST_END;
}
