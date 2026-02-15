#pragma once

#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include <chrono>

#include "AfterglowObject.h"

// TODO: Multi callbacks for one asset type support.
// @brief: This class use for hot-recompilation of materials and its shaders.
class AfterglowAssetMonitor : public AfterglowObject {
public:
	//	Custom additional infos [tag, context]
	using TagInfos = std::unordered_map<std::string, std::string>;

	enum class AssetType : uint32_t {
		Undefined, 
		Image, 
		Model, 
		Material, 
		MaterialInstance, 
		Shader
	};

	struct AssetInfo {
		AssetType type = AssetType::Undefined;
		TagInfos tagInfos;
		std::chrono::time_point<std::chrono::file_clock> lastModifiedTime;
	};

	using MonitoredAssets = std::unordered_map<std::string, AssetInfo>;
	using ModifiedCallback = void(const std::string&, TagInfos&);
	using DeletedCallback = void(const std::string&, TagInfos&);

	using ModifiedCallbacks = std::unordered_map<AssetType, std::function<ModifiedCallback>>;
	using DeletedCallbacks = std::unordered_map<AssetType, std::function<DeletedCallback>>;

	AfterglowAssetMonitor();
	~AfterglowAssetMonitor();

	// @note: invoke this function in target thread which are thread-safe for asset callback functions.
	void update();
	/**
	* @return: Success to register.
	* @thread-safety
	*/ 
	AssetInfo* registerAsset(AssetType type, const std::string& path, const TagInfos& tagInfos = {});
	/**
	* @return: Success to unregister.
	* @thread-safety
	*/
	bool unregisterAsset(const std::string& path);

	void setCheckInterval(float sec);
	void registerModifiedCallback(AssetType type, std::function<ModifiedCallback> callback);
	void registerDeletedCallback(AssetType type, std::function<DeletedCallback> callback);

	void unregisterModifiedCallback(AssetType type);
	void unregisterDeleteCallback(AssetType type);

	// @return: nullptr if this asset path is not registered.
	AssetInfo* registeredAssetInfo(const std::string& path);
	const AssetInfo* registeredAssetInfo(const std::string& path) const;

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
};

