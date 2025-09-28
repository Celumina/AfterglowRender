#include "AfterglowMaterialInstanceAsset.h"

#include <fstream>
#include <json.hpp>
#include "DebugUtilities.h"

struct AfterglowMaterialInstanceAsset::Context {
	nlohmann::json data;
};


AfterglowMaterialInstanceAsset::AfterglowMaterialInstanceAsset(const std::string& path) : 
	_context(std::make_unique<Context>()) {
	std::ifstream file(path);

	if (!file.is_open()) {
		DEBUG_TYPE_ERROR(AfterglowMaterialInstanceAsset, "Failed to load material instance file: " + path);
		throw std::runtime_error("[AfterglowMaterialInstanceAsset] Failed to open material instance file.");
	}
	try {
		file >> _context->data;
	}
	catch (const nlohmann::json::parse_error& error) {
		DEBUG_TYPE_ERROR(AfterglowMaterialInstanceAsset, "Failed to parse material instance file " + path + " to json, due to: " + error.what());
		throw std::runtime_error("[AfterglowMaterialInstanceAsset] Failed to parse material instance file to json.");
	}
}

AfterglowMaterialInstanceAsset::~AfterglowMaterialInstanceAsset() {
}

std::string AfterglowMaterialInstanceAsset::materialnstanceName() const {
	auto& data = _context->data;
	if (data.contains("name") && data["name"].is_string()) {
		return data["name"];
	}
	DEBUG_CLASS_ERROR("Failed to acquire mateiral instance name, make sure asset file including corrent \"name\" segment.");
	return "";
}

std::string AfterglowMaterialInstanceAsset::parentMaterialName() const {
	auto& data = _context->data;
	if (data.contains("parentMaterialName") && data["parentMaterialName"].is_string()) {
		return data["parentMaterialName"];
	}
	DEBUG_CLASS_ERROR("Failed to acquire parent mateiral name, make sure asset file including corrent \"name\" segment.");
	return "";
}

void AfterglowMaterialInstanceAsset::fill(AfterglowMaterialInstance& destMaterialInstance) const {
	auto& data = _context->data;
	if (data.contains("scalars") && data["scalars"].is_array()) {
		for (const auto& scalar : data["scalars"]) {
			if (!scalar.contains("name") || !scalar["name"].is_string()
				|| !scalar.contains("value") || !scalar["value"].is_number()
				|| !scalar.contains("stage") || !scalar["stage"].is_number_integer()) {
					continue;
				}
			destMaterialInstance.setScalar(scalar["stage"], scalar["name"], scalar["value"]);
		}
	}

	if (data.contains("vectors") && data["vectors"].is_array()) {
		for (const auto& vector : data["vectors"]) {
			if (!vector.contains("name") || !vector["name"].is_string()
				|| !vector.contains("value") || !vector["value"].is_array()
				|| !vector.contains("stage") || !vector["stage"].is_number_integer()) {
				continue;
			}
			auto& value = vector["value"];
			if (value.size() < 4 || !value[0].is_number() || !value[1].is_number() || !value[2].is_number() || !value[3].is_number()) {
				continue;	
			}
			destMaterialInstance.setVector(vector["stage"], vector["name"], { value[0], value[1], value[2], value[3]});
		}
	}

	if (data.contains("textures") && data["textures"].is_array()) {
		for (const auto& texture : data["textures"]) {
			if (!texture.contains("name") || !texture["name"].is_string()
				|| !texture.contains("value") || !texture["value"].is_object()
				|| !texture.contains("stage") || !texture["stage"].is_number_integer()) {
				continue; 
			}
			auto& value = texture["value"];
			if (!value.contains("path") || !value["path"].is_string()) {
				continue;
			}
			// If material instance input a undifined format or color space, it will keep parent settings.
			auto format = img::Format::Undefined;
			auto colorSpace = img::ColorSpace::Undefined;
			if (value.contains("format") && value["format"].is_number_integer()) {
				format = value["format"];
			}
			if (value.contains("colorSpace") && value["colorSpace"].is_number_integer()) {
				colorSpace = value["colorSpace"];
			}
			destMaterialInstance.setTexture(
				texture["stage"], texture["name"], {format, colorSpace, value["path"]}
			);
		}
	}
}
