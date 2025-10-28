#pragma once

#include "IndexableTree.h"
#include "AfterglowEntity.h"
#include "AfterglowUtilities.h"

class AfterglowScene : public AfterglowObject {
public:
	using EntityTree = IndexableTree<std::string, AfterglowEntity>;

	AfterglowScene(const std::string& sceneName = "");

	// @return: AfterglowEntity which is created.
	AfterglowEntity& createEntity(const std::string& name, util::OptionalRef<AfterglowEntity> parent = std::nullopt);

	// @return: Destroy entity successfully.
	bool destroyEntity(AfterglowEntity& entity);

	// @return: all entity if named as parameter.
	std::vector<AfterglowEntity*> findEntities(const std::string name);

	// @return: If entity is in this scene.
	bool isExists(AfterglowEntity& entity) const;
	bool hasChild(AfterglowEntity& entity) const;

	const std::string& sceneName() const noexcept;
	void setSceneName(const std::string& sceneName) noexcept;

	// @brief: transverse all EntityType entities and put them into the callback function. 
	// @params:
	//		func: The format of callback function: bool(EntityType&, ParameterTypes...);
	// @desc:
	//		Note that derived class also be found, so template type AfterglowEntity will foreach all entities of this scene.
	//		Callback function return bool to indicate if want to break the loop. if give a void, loops will not be interrupted.
	template<typename FuncType, typename ...ParameterTypes>
	void forEachEntity(FuncType func, ParameterTypes&& ...parameters);

	template<typename FuncType, typename ...ParameterTypes>
	void forEachEntityDFS(FuncType func, ParameterTypes&& ...parameters);

private:
	template<typename FuncType, typename ...ParameterTypes>
	static inline auto entityCallbackWarpper(AfterglowEntity& entity, std::type_index typeIndex, FuncType& callback, ParameterTypes &&...parameters);

	std::string _sceneName;
	EntityTree _entities;
};

template<typename FuncType, typename ...ParameterTypes>
inline void AfterglowScene::forEachEntity(FuncType func, ParameterTypes && ...parameters) {
	_entities.forEach(entityCallbackWarpper<FuncType, ParameterTypes...>, func, std::forward<ParameterTypes>(parameters)...);
}

template<typename FuncType, typename ...ParameterTypes>
inline void AfterglowScene::forEachEntityDFS(FuncType func, ParameterTypes && ...parameters) {
	_entities.forEachDFS(entityCallbackWarpper<FuncType, ParameterTypes...>, func, std::forward<ParameterTypes>(parameters)...);
}

template<typename FuncType, typename ...ParameterTypes>
inline auto AfterglowScene::entityCallbackWarpper(AfterglowEntity& entity, std::type_index typeIndex, FuncType& callback, ParameterTypes &&...parameters) {
	if constexpr (std::is_same_v<std::invoke_result_t<FuncType, AfterglowEntity&, ParameterTypes...>, bool>) {
		return callback(entity, std::forward<ParameterTypes>(parameters)...);
	}
	else {
		callback(entity, std::forward<ParameterTypes>(parameters)...);
	}
}
