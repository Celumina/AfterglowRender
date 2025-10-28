#pragma once

#include <string>
#include <memory>

class AfterglowMaterialManager;
class AfterglowAssetMonitor;

class AfterglowMaterialAssetRegistrar {
public:
	AfterglowMaterialAssetRegistrar(AfterglowMaterialManager& materialManager, AfterglowAssetMonitor& assetMonitor);
	~AfterglowMaterialAssetRegistrar();

	std::string registerMaterialAsset(const std::string& materialPath);
	std::string registerMaterialInstanceAsset(const std::string& materialInstancePath);

	void unregisterMaterialAsset(const std::string& materialPath);
	void unregisterMaterialInstanceAsset(const std::string& materialInstancePath);

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;

};

