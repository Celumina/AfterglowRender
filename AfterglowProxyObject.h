#pragma once

#include "AfterglowProxyTemplate.h"

template<typename Type, typename CreateInfoType>
struct AfterglowProxyObjectContext : public AfterglowProxyTemplateContext<CreateInfoType> {
	Type data = nullptr;
};


template<typename DerivedType, typename Type, typename CreateInfoType>
class AfterglowProxyObjectAsElement : public AfterglowProxyTemplateAsElement<DerivedType, CreateInfoType> {
public:
	using AsElementBase = AfterglowProxyTemplateAsElement<DerivedType, CreateInfoType>;
	operator Type& ();
	operator Type* ();
};


#define __ASELEMENT_TYPE \
AfterglowProxyObjectAsElement<DerivedType, Type, CreateInfoType>

#define __PROXY_BASE \
AfterglowProxyTemplate<DerivedType, CreateInfoType, AfterglowProxyObjectContext<Type, CreateInfoType>, __ASELEMENT_TYPE>


// Type should be a ptr.
template <typename DerivedType, typename Type, typename CreateInfoType = int>
class AfterglowProxyObject : public __PROXY_BASE {

public:
	using Raw = Type;
	using ProxyBase = __PROXY_BASE;
	// Remind that if you want to use AsElement and Array in a sequence of DerivedClasses, you should do some preparation.
	// See AfterglowBuffer.h

	~AfterglowProxyObject();

	operator Type&();
	operator Type*();

protected:
	// Only if data ptr is created from extern. 
	// If you want to use linkData(), Template "Type" should be a pointer.
	void linkData(Type data);

	// Reset data only.
	void destroy();

	// Destory data from this function.
	// Parameters: (nativeDestroyFunction, nativeDestroyArguments)
	template<typename FuncType, typename ...ArgTypes>
	void destroy(FuncType&& func, ArgTypes&& ...args);

	// Never use data() outside the create(), beacuse data could be not created, use *this instead.
	Type& data();
	bool isDataExists();

	// (Optional) If you want to create vkObject early before implicit cast call, call this function on Constructor.
	void initialize();
};


#define __PROXY_TEMPLATE_HEADER \
template<typename DerivedType, typename Type, typename CreateInfoType>

#define __PROXY_TYPE \
AfterglowProxyObject<DerivedType, Type, CreateInfoType>

#define __PROXY_CONSTRUCTOR \
__PROXY_TEMPLATE_HEADER \
inline __PROXY_TYPE 

#define __PROXY_DESTRUCTOR __PROXY_CONSTRUCTOR

#define __PROXY_IMPLICIT_CONVERSION __PROXY_CONSTRUCTOR

#define __PROXY_MEMBER_FUNCTION(ReturnType) \
__PROXY_TEMPLATE_HEADER \
inline ReturnType __PROXY_TYPE 

#define __ASELEMENT_IMPLICIT_CONVERSION \
template<typename DerivedType, typename Type, typename CreateInfoType> \
inline __ASELEMENT_TYPE


__PROXY_DESTRUCTOR::~AfterglowProxyObject() {
	if constexpr (std::is_array<Type>::value) {
		if (ProxyBase::context().data[0] != nullptr) {
			DEBUG_TYPE_WARNING(DerivedType, "Data array was not destroyed. Make sure destroy() was called before destruction done.");
			return;
		}
	}
	else {
		if (ProxyBase::context().data != nullptr) {
			DEBUG_TYPE_WARNING(DerivedType, "Dataptr was not destroyed. Make sure destroy() was called before destruction done.");
			return;
		}
	}
	DEBUG_TYPE_INFO(DerivedType, "Class was destroyed.");
}

__PROXY_IMPLICIT_CONVERSION::operator Type&() {
	initialize();
	return ProxyBase::context().data;
}

__PROXY_IMPLICIT_CONVERSION::operator Type*() {
	return &(this->operator Type & ());
}

__PROXY_MEMBER_FUNCTION(void)::linkData(Type data) {
	if constexpr (std::is_pointer<Type>::value) {
		ProxyBase::context().data = data;
	}
}

__PROXY_MEMBER_FUNCTION(void)::destroy() {
	// Clear array create flag (First Element).
	if constexpr (std::is_array<Type>::value) {
		ProxyBase::context().data[0] = nullptr;
	}
	else {
		ProxyBase::context().data = nullptr;
	}
}

__PROXY_MEMBER_FUNCTION(Type&)::data() {
	return ProxyBase::context().data;
}

__PROXY_MEMBER_FUNCTION(bool)::isDataExists() {
	if constexpr (std::is_array<Type>::value) {
		return ProxyBase::context().data[0];
	}
	else {
		return ProxyBase::context().data;
	}
}

__PROXY_MEMBER_FUNCTION(void)::initialize() {
	// Array support.
	if constexpr (std::is_array<Type>::value) {
		if (!ProxyBase::context().data[0]) {
			ProxyBase::initializationCore();
		}
	}
	else {
		if (!ProxyBase::context().data) {
			ProxyBase::initializationCore();
		}
	}
}

__PROXY_TEMPLATE_HEADER
template<typename FuncType, typename ...ArgTypes>
inline void __PROXY_TYPE::destroy(FuncType&& func, ArgTypes && ...args) {
	func(args...);
	destroy();
}

__ASELEMENT_IMPLICIT_CONVERSION::operator Type& () {
	return *AsElementBase::raw().get();
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
#undef __ASELEMENT_IMPLICIT_CONVERSION