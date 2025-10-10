#pragma once

#include "AfterglowProxyTemplate.h"

/* Different between these proxy array: 
ProxyObject::Array
	a.k.a std::vector<ProxyObject::AsElement>
	This Type use for every objects have theirs own independent create() procedual.
	Dynamic Array Size:    Yes
	Memory Continuous:     No
	CRTP:                  Yes

ProxyObject<DerivedType, Type[Size], CreateInfoType>
	This class use for vulkan array reference if this array has fixed size. 
	Its array created use only one create() procedual for all element.
	If you want to change array size, you should re-compile program.
	Dynamic Array Size:    No
	Memory Continuous:     Yes
	CRTP:                  Yes

ProxyArray
	This proxy class make sure vkptrs have continuous memory.
	You can also set array size when class is constructed.
	Dynamic Array Size:    Yes
	Memory Continuous:     Yes
	CRTP:                  Yes
*/


template<typename Type, typename CreateInfoType>
struct AfterglowProxyArrayContext : public AfterglowProxyTemplateContext<CreateInfoType> {
	std::vector<Type> array;
};


template<typename DerivedType, typename Type, typename CreateInfoType>
class AfterglowProxyArrayAsElement : public AfterglowProxyTemplateAsElement<DerivedType, CreateInfoType> {
public:
	using AsElementBase = AfterglowProxyTemplateAsElement<DerivedType, CreateInfoType>;
	Type& operator[] (uint32_t index);
	operator Type* ();
};


#define __ASELEMENT_TYPE \
AfterglowProxyArrayAsElement<DerivedType, Type, CreateInfoType>

#define __PROXY_BASE \
AfterglowProxyTemplate<DerivedType, CreateInfoType, AfterglowProxyArrayContext<Type, CreateInfoType>, __ASELEMENT_TYPE>


// This proxy class make sure vkptrs have continuous memory.
// Type should be a ptr.
template <typename DerivedType, typename Type, typename CreateInfoType>
class AfterglowProxyArray : public __PROXY_BASE {

public:
	using Raw = Type;
	using ProxyBase = __PROXY_BASE;
	using ConstProxyBase = const ProxyBase;

	AfterglowProxyArray(uint32_t size);
	~AfterglowProxyArray();

	Type& operator[] (uint32_t index);

	// Returns the first element address of array.
	operator const Type* ();

	uint32_t size() const;

protected:
	// Reset data only.
	void destroy();

	// Destory data from this function.
	// Parameters: (nativeDestroyFunction, nativeDestroyArguments)
	template<typename FuncType, typename ...ArgTypes>
	void destroy(FuncType&& func, ArgTypes&& ...args);

	// Returns the first data address.
	Type* data();
	const Type* data() const;
	bool isDataExists();

	// (Optional) If you want to create vkObject early before implicit cast call, call this function on Constructor.
	void initialize();
};


#define __PROXY_TEMPLATE_HEADER \
template<typename DerivedType, typename Type, typename CreateInfoType>

#define __PROXY_TYPE \
AfterglowProxyArray<DerivedType, Type, CreateInfoType>

#define __PROXY_CONSTRUCTOR \
__PROXY_TEMPLATE_HEADER \
inline __PROXY_TYPE

#define __PROXY_DESTRUCTOR __PROXY_CONSTRUCTOR

#define __PROXY_IMPLICIT_CONVERSION __PROXY_CONSTRUCTOR

#define __PROXY_MEMBER_FUNCTION(ReturnType) \
__PROXY_TEMPLATE_HEADER \
inline ReturnType __PROXY_TYPE

#define __ASELEMENT_TEMPLATE_HEADER \
template<typename DerivedType, typename Type, typename CreateInfoType>

#define __ASELEMENT_IMPLICIT_CONVERSION \
__ASELEMENT_TEMPLATE_HEADER \
inline __ASELEMENT_TYPE

#define __ASELEMENT_MEMBER_FUNCTION(ReturnType) \
__ASELEMENT_TEMPLATE_HEADER \
inline ReturnType __ASELEMENT_TYPE


__PROXY_CONSTRUCTOR::AfterglowProxyArray(uint32_t size) {
	ProxyBase::context().array.resize(size);
}

__PROXY_DESTRUCTOR::~AfterglowProxyArray() {
	// AfterglowProxyArray use first element as flag.
	if (ProxyBase::context().array[0] != nullptr) {
		DEBUG_TYPE_WARNING(DerivedType, "Class may not be destroyed. Make sure call destroy() explicitly.");
		return;
	}
	DEBUG_TYPE_INFO(DerivedType, "Class was destroyed.");
}

__PROXY_MEMBER_FUNCTION(Type&)::operator[](uint32_t index) {
	initialize();
	return ProxyBase::context().array[index];
}

__PROXY_IMPLICIT_CONVERSION::operator const Type* () {
	initialize();
	return ProxyBase::context().array.data();
}

__PROXY_MEMBER_FUNCTION(uint32_t)::size()  const {
	return static_cast<uint32_t>(ConstProxyBase::context().array.size());
}

__PROXY_MEMBER_FUNCTION(void)::destroy() {
	ProxyBase::context().array[0] = nullptr;
}

__PROXY_TEMPLATE_HEADER
template<typename FuncType, typename ...ArgTypes>
inline void __PROXY_TYPE::destroy(FuncType&& func, ArgTypes && ...args) {
	func(args...);
	destroy();
}

__PROXY_MEMBER_FUNCTION(Type*)::data() {
	return ProxyBase::context().array.data();
}

__PROXY_MEMBER_FUNCTION(const Type*)::data() const {
	return ProxyBase::context().array.data();
}

__PROXY_MEMBER_FUNCTION(bool)::isDataExists() {
	return ProxyBase::context().array[0];
}

__PROXY_MEMBER_FUNCTION(void)::initialize() {
	if (!ProxyBase::context().array[0]) {
		ProxyBase::initializationCore();
		if (ProxyBase::context().array[0] == nullptr) {
			// Because many of proxy array methods use array[0] for validation check.
			DEBUG_TYPE_ERROR(DerivedType, "The first element of array have not contain any data, it would cause memory leak.");
			throw  std::runtime_error(
				std::string("[") + typeid(DerivedType).name() + "] The first element of array have not contain any data, it would cause memory leak."
			);
		}
	}
}

__ASELEMENT_MEMBER_FUNCTION(Type&)::operator[](uint32_t index) {
	return (*AsElementBase::raw().get())[index];
}

__ASELEMENT_IMPLICIT_CONVERSION::operator Type* () {
	return *AsElementBase::raw().get();
}


#undef __ASELEMENT_TYPE
#undef __PROXY_BASE

#undef __PROXY_TEMPLATE_HEADER
#undef __PROXY_TYPE
#undef __PROXY_CONSTRUCTOR
#undef __PROXY_DESTRUCTOR
#undef __PROXY_IMPLICIT_CONVERSION
#undef __PROXY_MEMBER_FUNCTION
#undef __ASELEMENT_TEMPLATE_HEADER
#undef __ASELEMENT_IMPLICIT_CONVERSION
#undef __ASELEMENT_MEMBER_FUNCTION
