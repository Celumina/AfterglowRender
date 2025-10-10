#pragma once

#include <memory>
#include <vector>
#include <string>
#include <stdexcept>
#include <type_traits>

#include <vulkan/vulkan.h>

#include "AfterglowObject.h"
#include "DebugUtilities.h"

// TODO: const functions support.

#define proxy_protected \
friend class ProxyBase; \
protected

// If a derived class could be derived by other class, use this marco instead of proxy_protected.
#define proxy_base_protected(Base) \
friend class Base::ProxyBase; \
protected

#define proxy_private \
friend class ProxyBase; \
private

// If a derived class could be derived by other class, use this marco instead of proxy_private.
#define proxy_base_private(Base) \
friend class Base::ProxyBase; \
private

// This Marco declare inside the class to indicate derived that class will never override initCreateInfo() and create() function.
#define AFTERGLOW_PROXY_STORAGE_ONLY \
proxy_protected: \
	void initCreateInfo() {}; \
	void create() {};

template<typename CreateInfoType>
struct AfterglowProxyTemplateContext {
	// CreateInfo will be destroy after data is created.
	std::unique_ptr<CreateInfoType> createInfo;
};

template<typename DerivedType, typename CreateInfoType>
class AfterglowProxyTemplateAsElement {
public:
	CreateInfoType* operator->();
	operator DerivedType& ();
	operator DerivedType* ();
	operator bool();
	DerivedType& operator*();
	DerivedType& get();

	std::unique_ptr<DerivedType>& raw();

	template<typename ...ArgTypes>
	void recreate(ArgTypes&&... args);

private:
	std::unique_ptr<DerivedType> _data = nullptr;
};

template <
	typename DerivedType, 
	typename CreateInfoType, 
	typename ContextType = AfterglowProxyTemplateContext<CreateInfoType>, 
	typename AsElementType = AfterglowProxyTemplateAsElement<DerivedType, CreateInfoType>
>
class AfterglowProxyTemplate : public AfterglowObject {
public:
	AfterglowProxyTemplate();
	AfterglowProxyTemplate(AfterglowProxyTemplate&& rval);

	using AsElement = AsElementType;
	using Array = std::vector<AsElement>;
	using Info = CreateInfoType;
	using Derived = DerivedType;

	// Only before vkObject is created, CreateInfo will be cleared after that. 
	CreateInfoType* operator->();

	// If you want to store a  proxy in a  container, use this function to create a element handle.
	template<typename ...ArgTypes>
	static AsElement makeElement(ArgTypes&& ...args);

protected:
	// Override this function to implement CreateInfo default settings.
	// CRTP Based functions.
	void initCreateInfo();
	void create();

	ContextType& context();
	const ContextType& context() const;

	CreateInfoType& info();

	bool isInfoExists() const;

	void initializationCore();

	// Repace raw runtime error for contains class info.
	std::runtime_error runtimeError(const char* text) const;

private:
	// To avoid client forget to call parent function.
	void initCreateInfoShell();

	std::unique_ptr<ContextType> _context;
};


#define __PROXY_TEMPLATE_HEADER \
template<typename DerivedType, typename CreateInfoType, typename ContextType, typename AsElementType>

#define __PROXY_TYPE \
AfterglowProxyTemplate<DerivedType, CreateInfoType, ContextType, AsElementType>

#define __PROXY_CONSTRUCTOR \
__PROXY_TEMPLATE_HEADER \
inline __PROXY_TYPE 

#define __PROXY_MEMBER_FUNCTION(ReturnType) \
__PROXY_TEMPLATE_HEADER \
inline ReturnType __PROXY_TYPE 

#define __ASELEMENT_TEMPLATE_HEADER \
template<typename DerivedType, typename CreateInfoType>

#define __ASLELEMENT_TYPE \
AfterglowProxyTemplateAsElement<DerivedType, CreateInfoType>

#define __ASELEMENT_IMPLICIT_CONVERSION \
 __ASELEMENT_TEMPLATE_HEADER \
inline __ASLELEMENT_TYPE

#define __ASELEMENT_MEMBER_FUNCTION(ReturnType) \
__ASELEMENT_TEMPLATE_HEADER \
inline ReturnType __ASLELEMENT_TYPE


__PROXY_CONSTRUCTOR::AfterglowProxyTemplate() :
	_context(std::make_unique<ContextType>()) {
}

__PROXY_CONSTRUCTOR::AfterglowProxyTemplate(AfterglowProxyTemplate&& rval) :
	_context(std::move(rval._context)) {
}

__PROXY_MEMBER_FUNCTION(CreateInfoType*)::operator->() {
	initCreateInfoShell();
	return _context->createInfo.get();
}

__PROXY_TEMPLATE_HEADER
template<typename ...ArgTypes>
inline __PROXY_TYPE::AsElement __PROXY_TYPE::makeElement(ArgTypes&& ...args) {
	AsElement element;
	element.raw() = std::make_unique<DerivedType>(std::forward<ArgTypes>(args)...);
	return element;
}

__PROXY_MEMBER_FUNCTION(void)::initCreateInfo() {
	static_assert(
		!std::is_same_v<decltype(&__PROXY_TYPE::initCreateInfo), decltype(&DerivedType::initCreateInfo)>,
		"[AfterglowProxyTemplate::initCreateInfo] The CRTP interface function has not been declared in DerivedType."
	);
	reinterpret_cast<DerivedType*>(this)->initCreateInfo();
}

__PROXY_MEMBER_FUNCTION(void)::create() {
	static_assert(
		!std::is_same_v<decltype(&__PROXY_TYPE::create), decltype(&DerivedType::create)>,
		"[AfterglowProxyTemplate::create] The CRTP interface function has not been declared in DerivedType."
	);
	reinterpret_cast<DerivedType*>(this)->create();
}

__PROXY_MEMBER_FUNCTION(ContextType&)::context() {
	return *_context;
}

__PROXY_MEMBER_FUNCTION(const ContextType&)::context() const {
	return *_context;
}

__PROXY_MEMBER_FUNCTION(CreateInfoType&)::info() {
	return *operator->();
}

__PROXY_MEMBER_FUNCTION(bool)::isInfoExists() const {
	return _context->createInfo.get();
}

__PROXY_MEMBER_FUNCTION(void)::initCreateInfoShell() {
	if (!_context->createInfo) {
		_context->createInfo = std::make_unique<CreateInfoType>();
		initCreateInfo();
		DEBUG_TYPE_INFO(DerivedType, "CreateInfo was initialized.");
	}
}

__PROXY_MEMBER_FUNCTION(void)::initializationCore() {
	initCreateInfoShell();
	create();
	context().createInfo.reset();
	DEBUG_TYPE_INFO(DerivedType, "Class was created.");
}

__PROXY_MEMBER_FUNCTION(std::runtime_error)::runtimeError(const char* text) const {
	DEBUG_TYPE_FATAL(DerivedType, text);
	std::string errorText = std::string("[throw runtimeError] ") + typeid(DerivedType).name() + " " + text;
	return std::runtime_error(errorText);
}

__ASELEMENT_MEMBER_FUNCTION(CreateInfoType*)::operator->() {
	return _data.get()->operator->();
}

__ASELEMENT_IMPLICIT_CONVERSION::operator DerivedType& () {
	return *_data.get();
}

__ASELEMENT_IMPLICIT_CONVERSION::operator DerivedType* () {
	return _data.get();
}

__ASELEMENT_IMPLICIT_CONVERSION::operator bool() {
	return _data.get();
}

__ASELEMENT_MEMBER_FUNCTION(DerivedType&)::operator*() {
	return *_data.get();
}

__ASELEMENT_MEMBER_FUNCTION(DerivedType&)::get() {
	return *_data.get();
}

__ASELEMENT_MEMBER_FUNCTION(std::unique_ptr<DerivedType>&)::raw() {
	return _data;
}

__ASELEMENT_TEMPLATE_HEADER
template<typename ...ArgTypes>
inline void __ASLELEMENT_TYPE::recreate(ArgTypes&& ...args) {
	_data.reset();
	_data = std::make_unique<DerivedType>(std::forward<ArgTypes>(args)...);
};


#undef __PROXY_TEMPLATE_HEADER
#undef __PROXY_TYPE
#undef __PROXY_CONSTRUCTOR
#undef __PROXY_MEMBER_FUNCTION
#undef __ASELEMENT_TEMPLATE_HEADER
#undef __ASELEMENT_TYPE
#undef __ASELEMENT_MEMBER_FUNCTION
#undef __ASELEMENT_IMPLICIT_CONVERSION