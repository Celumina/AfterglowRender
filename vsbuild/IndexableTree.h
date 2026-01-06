#pragma once

#include <unordered_set>
#include <unordered_map>

#include "IndexableNode.h"
#include "DebugUtilities.h"

template<typename TagType, typename Type>
class IndexableTree {
public: 
	using Node = IndexableNode<TagType, Type>;
	using NodeID = Node::ID;
	// TODO: If tag change...
	using TagIDs = std::unordered_map<TagType, std::unordered_set<NodeID>>;
	using IDReferences = std::unordered_map<NodeID, typename Node::WeakReference>;

	IndexableTree();

	// Return data only, do not return node handle..
	template<typename DataRawType = Type>
	DataRawType* get(Node::ID id);

	NodeID root() const noexcept;
	static constexpr NodeID invalidID() noexcept;

	/** 
	* @params callback: 
		bool(Type&, std::type_index, parameters...);  
	* @desc:
	*	[UNORDERED] for each, if return true, break out the loop.
	*	function return type can be void, means it will not interrupt this loops.
	*/
	template<typename FuncType, typename ...ParameterTypes>
	void forEach(FuncType& callback, ParameterTypes&& ...parameters);

	// @brief: For each node with depth first search.
	template<typename FuncType, typename ...ParameterTypes>
	void forEachDFS(FuncType& callback, ParameterTypes&& ...parameters);

	template<typename FuncType, typename ...ParameterTypes>
	void forEachDFS(Node& root, FuncType& callback, ParameterTypes&& ...parameters);

	std::unordered_set<NodeID> find(Node::Tag tag);
	bool isExists(Node::ID id) const;
	uint64_t numChildren(Node::ID id) const;

	// @returns Node::ID: If append successfully, return new nodeID, else return 0.
	template<typename DataRawType>
	Node::ID append(const TagType& tag, std::unique_ptr<DataRawType>&& data, Node::ID dstParentID = Node::invalidID());

	bool move(Node::ID id, Node::ID dstParentID);
	bool remove(Node::ID id);

private:
	void registerNode(Node::WeakReference node);
	void unregisterNode(Node::WeakReference node);

	Node _root;
	TagIDs _tagIDs;
	IDReferences _idReferences;
};

template<typename TagType, typename Type>
inline IndexableTree<TagType, Type>::IndexableTree() : 
	_root("ROOT_NODE") {
	registerNode(&_root);
}

template<typename TagType, typename Type>
inline IndexableTree<TagType, Type>::NodeID IndexableTree<TagType, Type>::root() const noexcept {
	return _root.id();
}

template<typename TagType, typename Type>
inline constexpr IndexableTree<TagType, Type>::NodeID IndexableTree<TagType, Type>::invalidID() noexcept {
	return Node::invalidID();
}

template<typename TagType, typename Type>
inline std::unordered_set<typename IndexableTree<TagType, Type>::Node::ID> IndexableTree<TagType, Type>::find(Node::Tag tag) {
	auto tagIDIterator = _tagIDs.find(tag);
	if (tagIDIterator != _tagIDs.end()) {
		return tagIDIterator->second;
	}
	return std::unordered_set<NodeID>{};
}

template<typename TagType, typename Type>
inline bool IndexableTree<TagType, Type>::isExists(Node::ID id) const {
	if (_idReferences.find(id) != _idReferences.end()) {
		return true;
	}
	return false;
}

template<typename TagType, typename Type>
inline uint64_t IndexableTree<TagType, Type>::numChildren(Node::ID id) const {
	auto iterator = _idReferences.find(id);
	if (iterator == _idReferences.end()) {
		return 0;
	}
	return iterator->second->numChildren();
}

template<typename TagType, typename Type>
template<typename DataRawType>
inline IndexableTree<TagType, Type>::Node::ID IndexableTree<TagType, Type>::append(const TagType& tag, std::unique_ptr<DataRawType>&& data, Node::ID dstParentID) {
	if (dstParentID == Node::invalidID()) {
		dstParentID = _root.id();
	}
	auto parentIDRefIterator = _idReferences.find(dstParentID);
	if (parentIDRefIterator != _idReferences.end()) {
		typename Node::WeakReference node = parentIDRefIterator->second->createChild(tag, std::move(data));
		registerNode(node);
		return node->id();
	}
	return Node::invalidID();
}

template<typename TagType, typename Type>
inline bool IndexableTree<TagType, Type>::move(Node::ID id, Node::ID dstParentID) {
	auto idRefIterator = _idReferences.find(id);
	auto parentIDRefIterator = _idReferences.find(dstParentID);
	if (idRefIterator != _idReferences.end() && parentIDRefIterator != _idReferences.end()) {
		auto& node = idRefIterator->second;
		if (node->parent()) {
			node->parent()->moveChild(id, parentIDRefIterator->second);
			return true;
		}
		else {
			DEBUG_CLASS_WARNING("Root node can not be moved.");
			return false;
		}
	}
	return false;
}

template<typename TagType, typename Type>
inline bool IndexableTree<TagType, Type>::remove(Node::ID id) {
	auto idRefIterator = _idReferences.find(id);
	if (idRefIterator != _idReferences.end()) {
		auto& node = idRefIterator->second;
		auto* parent = node->parent();
		if (parent) {
			// Unregister should early than remove node, due to it depnent on node info.
			unregisterNode(node);
			parent->removeChild(id);
			return true;
		}
		else {
			DEBUG_CLASS_WARNING("Root node can not be removed.");
			return false;
		}
	}
	return false;
}

template<typename TagType, typename Type>
inline void IndexableTree<TagType, Type>::registerNode(Node::WeakReference node) {
	auto tagIDIterator = _tagIDs.find(node->tag());
	if (tagIDIterator != _tagIDs.end()) {
		tagIDIterator->second.insert(node->id());
	}
	else {
		_tagIDs[node->tag()] = { node->id() };
	}
	_idReferences[node->id()] = node;
}

template<typename TagType, typename Type>
inline void IndexableTree<TagType, Type>::unregisterNode(Node::WeakReference node) {
	const auto& tag = node->tag();
	auto tagIDIterator = _tagIDs.find(tag);
	if (tagIDIterator != _tagIDs.end()) {
		auto& nodes = tagIDIterator->second;
		nodes.erase(node->id());
		if (nodes.empty()) {
			_tagIDs.erase(tag);
		}
	}
	_idReferences.erase(node->id());
}

template<typename TagType, typename Type>
template<typename DataRawType>
inline DataRawType* IndexableTree<TagType, Type>::get(Node::ID id) {
	auto idRefIterator = _idReferences.find(id);
	if (idRefIterator == _idReferences.end()) {
		return nullptr;
	}
	return idRefIterator->second->cast<DataRawType*>();
}

template<typename TagType, typename Type>
template<typename FuncType, typename ...ParameterTypes>
inline void IndexableTree<TagType, Type>::forEach(FuncType& callback, ParameterTypes&& ...parameters) {
	for (auto& [id, node]  : _idReferences) {
		// Skip the root node
		if (id == root()) {
			continue;
		}
		if constexpr (std::is_same_v<std::invoke_result_t<FuncType, Type&, std::type_index, ParameterTypes...>, bool>) {
			bool shouldBreak = callback(**node, node->typeIndex(), std::forward<ParameterTypes>(parameters)...);
			if (shouldBreak) {
				break;
			}
		}
		else {
			callback(**node, node->typeIndex(), std::forward<ParameterTypes>(parameters)...);
		}
	}
}

template<typename TagType, typename Type>
template<typename FuncType, typename ...ParameterTypes>
inline void IndexableTree<TagType, Type>::forEachDFS(FuncType& callback, ParameterTypes && ...parameters) {
	forEachDFS(_root, callback, parameters...);
}

template<typename TagType, typename Type>
template<typename FuncType, typename ...ParameterTypes>
inline void IndexableTree<TagType, Type>::forEachDFS(Node& root, FuncType& callback, ParameterTypes&& ...parameters) {
	// Skip the tree root node
	if (&root != &_root) {
		if constexpr (std::is_same_v<std::invoke_result_t<FuncType, Type&, std::type_index, ParameterTypes...>, bool>) {
			if (callback(*root, root.typeIndex(), std::forward<ParameterTypes>(parameters)...)) {
				return;
			}
		}
		else {
			callback(*root, root.typeIndex(), std::forward<ParameterTypes>(parameters)...);
		}
	}
	root.forEachChild([&](Node& child) {
		forEachDFS(child, callback, std::forward<ParameterTypes>(parameters)...);
	});
}
