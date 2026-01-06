#pragma once

#include <unordered_map>
#include <memory>
#include <typeindex>

template<typename TagType, typename Type>
class IndexableNode {
public:
	using ID = uint64_t;
	using Tag = TagType;
	// Polymorphic support.
	using Data = std::unique_ptr<Type>;
	using Child = std::unique_ptr<IndexableNode>;
	using Children = std::unordered_map<ID, Child>;
	// Weak means data is not constrast in vaild, maybe nullptr or dangling.
	using WeakReference = IndexableNode*;

	IndexableNode(const TagType& tag = TagType(), Data data = nullptr, WeakReference parent = nullptr, std::type_index typeIndex = std::type_index(typeid(Type)));

	template<typename DataRawType>
	static IndexableNode create(const TagType& tag = TagType(), std::unique_ptr<DataRawType> data = nullptr, WeakReference parent = nullptr);

	static constexpr ID invalidID() noexcept;

	// @return new child node reference.
	template<typename DataRawType>
	WeakReference createChild(const TagType& tag = TagType(), std::unique_ptr<DataRawType> data = nullptr);

	// @return new child node reference.
	WeakReference appendChild(Child&& node);
	bool removeChild(ID id);
	bool moveChild(ID childID, WeakReference dstNode);

	template<typename DataRawType>
	void reset(std::unique_ptr<DataRawType>&& data);

	ID id() const noexcept;
	TagType tag() const noexcept;

	WeakReference parent() noexcept;
	const WeakReference parent() const noexcept;

	uint64_t numChildren() const noexcept;

	// @desc: Complexity O(n)
	WeakReference child(ID id);
	const WeakReference child(ID id) const;

	std::type_index typeIndex() const noexcept;

	template<typename DataRawType>
	bool isType() noexcept;

	Type& operator*() noexcept;
	const Type& operator*() const noexcept;
	operator bool() const noexcept;

	operator Type& () noexcept;
	operator const Type& () const noexcept;

	operator Type* () noexcept;
	operator const Type* () const noexcept;

	template<typename DerivedPointerType>
	DerivedPointerType cast() noexcept;

	/**
	@param callback:
		void(IndexableNode&, ParameterTypes ...parameters);
		or
		bool(IndexableNode&, ParameterTypes ...parameters); return true to break out the loop.
	*/ 
	template<typename FuncType, typename ...ParameterTypes>
	void forEachChild(FuncType callback, ParameterTypes&& ...parameters);

private:
	// TODO: Multi-threads support.
	// C++ 17 support.
	static inline ID _allocatedID = 0;

	ID _id;
	TagType _tag;
	std::type_index _typeIndex;
	Data _data;
	Children _children;
	WeakReference _parent;
};

template<typename TagType, typename Type>
inline IndexableNode<TagType, Type>::IndexableNode(const TagType& tag, Data data, WeakReference parent, std::type_index typeIndex) :
	_tag(tag), _data(std::move(data)), _id(++_allocatedID), _parent(parent), _typeIndex(typeIndex) {
}

template<typename TagType, typename Type>
template<typename DataRawType>
inline IndexableNode<TagType, Type>::WeakReference IndexableNode<TagType, Type>::createChild(const TagType& tag, std::unique_ptr<DataRawType> data) {
	_children[IndexableNode::_allocatedID] = std::make_unique<IndexableNode>(tag, std::move(data), this, std::type_index(typeid(DataRawType)));
	return _children[IndexableNode::_allocatedID].get();
}

template<typename TagType, typename Type>
inline constexpr IndexableNode<TagType, Type>::ID IndexableNode<TagType, Type>::invalidID() noexcept {
	return 0;
}

template<typename TagType, typename Type>
inline IndexableNode<TagType, Type>::WeakReference IndexableNode<TagType, Type>::appendChild(Child&& node) {
	_children[node._id] = node;
	_children[node._id]._parent = this;
	return _children[node._id];
}

template<typename TagType, typename Type>
inline bool IndexableNode<TagType, Type>::removeChild(ID id) {
	return _children.erase(id);
}

template<typename TagType, typename Type>
inline bool IndexableNode<TagType, Type>::moveChild(ID childID, WeakReference dstNode) {
	auto iterator = _children.find(childID);
	if (iterator != _children.end() && dstNode) {
		dstNode->appendChild(std::unique_ptr<IndexableNode>(std::move((*iterator))));
		_children.erase(childID);
		return true;
	}
	return false;
}

template<typename TagType, typename Type>
template<typename DataRawType>
inline void IndexableNode<TagType, Type>::reset(std::unique_ptr<DataRawType>&& data) {
	_data.reset(data);
	_typeIndex = std::type_index(typeid(DataRawType));
}

template<typename TagType, typename Type>
inline IndexableNode<TagType, Type>::ID IndexableNode<TagType, Type>::id() const noexcept {
	return _id;
}

template<typename TagType, typename Type>
inline TagType IndexableNode<TagType, Type>::tag() const noexcept {
	return _tag;
}

template<typename TagType, typename Type>
inline IndexableNode<TagType, Type>::WeakReference IndexableNode<TagType, Type>::parent() noexcept {
	return _parent;
}

template<typename TagType, typename Type>
inline const IndexableNode<TagType, Type>::WeakReference IndexableNode<TagType, Type>::parent() const noexcept {
	return _parent;
}

template<typename TagType, typename Type>
inline uint64_t IndexableNode<TagType, Type>::numChildren() const noexcept {
	return _children.size();
}

template<typename TagType, typename Type>
inline IndexableNode<TagType, Type>::WeakReference IndexableNode<TagType, Type>::child(ID id) {
	auto iterator = _children.find(id);
	if (iterator != _children.end()) {
		return *iterator;
	}
	return nullptr;
}

template<typename TagType, typename Type>
inline const IndexableNode<TagType, Type>::WeakReference IndexableNode<TagType, Type>::child(ID id) const {
	return const_cast<decltype(this)>(this)->child(id);
}

template<typename TagType, typename Type>
inline std::type_index IndexableNode<TagType, Type>::typeIndex() const noexcept {
	return _typeIndex;
}

template<typename TagType, typename Type>
inline Type& IndexableNode<TagType, Type>::operator*() noexcept {
	return *_data;
}

template<typename TagType, typename Type>
inline const Type& IndexableNode<TagType, Type>::operator*() const noexcept {
	return *_data;
}

template<typename TagType, typename Type>
inline IndexableNode<TagType, Type>::operator bool() const noexcept {
	return _data;
}

template<typename TagType, typename Type>
inline IndexableNode<TagType, Type>::operator Type& () noexcept {
	return *_data;
}

template<typename TagType, typename Type>
inline IndexableNode<TagType, Type>::operator const Type& () const noexcept {
	return *_data;
}

template<typename TagType, typename Type>
inline IndexableNode<TagType, Type>::operator Type* () noexcept {
	return _data.get();
}

template<typename TagType, typename Type>
inline IndexableNode<TagType, Type>::operator const Type* () const noexcept {
	return _data.get();
}

template<typename TagType, typename Type>
template<typename DataRawType>
inline IndexableNode<TagType, Type> IndexableNode<TagType, Type>::create(const TagType& tag, std::unique_ptr<DataRawType> data, WeakReference parent) {
	return IndexableNode(tag, data, parent, std::type_index(typeid(DataRawType)));
}

template<typename TagType, typename Type>
template<typename DataRawType>
inline bool IndexableNode<TagType, Type>::isType() noexcept {
	return _typeIndex == std::type_index(typeid(DataRawType));
}

template<typename TagType, typename Type>
template<typename DerivedPointerType>
inline DerivedPointerType IndexableNode<TagType, Type>::cast() noexcept {
	static_assert(std::is_pointer_v<DerivedPointerType>, "[IndexableNode] Cast target must be a pointer.");
	if (_typeIndex == std::type_index(typeid(std::remove_pointer_t<DerivedPointerType>))) {
		return reinterpret_cast<DerivedPointerType>(_data.get());
	}
	return nullptr;
}

template<typename TagType, typename Type>
template<typename FuncType, typename ...ParameterTypes>
inline void IndexableNode<TagType, Type>::forEachChild(FuncType callback, ParameterTypes && ...parameters) {
	for (const auto& [id, child] : _children) {
		if constexpr (std::is_same_v<std::invoke_result_t<FuncType, IndexableNode&, ParameterTypes...>, bool>) {
			if (callback(*child, parameters...)) {
				break;
			}
		}
		else {
			callback(*child, parameters...);
		}
	}
}
