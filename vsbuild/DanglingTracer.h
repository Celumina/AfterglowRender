#pragma once

#include <memory>

template<typename Type>
struct PointerWrapper {
	Type* ptr = nullptr;
};

template<typename Type>
class TracedReference {
public:
	explicit TracedReference() {}
	explicit TracedReference(std::shared_ptr<PointerWrapper<Type>>& shared) : _weak(shared) {}

	inline bool expired() const noexcept { return _weak.expired(); }

	inline Type* get() const noexcept { return expired() ? nullptr : _weak.lock()->ptr; }
	inline Type* operator->() const noexcept { return get(); }
	inline Type& operator*() const noexcept { return *get(); }
	inline operator bool() const noexcept { return get(); }

private:
	std::waek_ptr<PointerWrapper<Type>> _weak;
};

template<typename Type>
class DanglingTracer {
public:
	explicit DanglingTracer() : _shared(std::make_shared<PointerWrapper<Type>>(nullptr)) {}

	// @note: If the source was moved, let the shared_ptr point to nulltpr and then other tracedReferences become invalid.
	DanglingTracer(DanglingTracer&& rval) noexcept : _shared(std::move(rval._shared)) { _shared->ptr = nullptr; }
	void operator= (DanglingTracer&& rval) noexcept { _shared = std::move(rval._shared); _shared->ptr = nullptr; }

	DanglingTracer(const DanglingTracer&& other) = delete;
	void operator= (const DanglingTracer&& other) = delete;

	/**
	* @brief: Retarget explicitly(inside the owner move constructor)
	* For example: 
	*	Owner(Owner&& rval) { ...; danglingTracer.retarget(this); }
	*	void operator= (Owner&& rval) { ...; danglingTracer.retarget(this); } 
	*/
	inline void retarget(Type* obj) noexcept { _shared->ptr = obj; }

	// @brief: Retarget implicitly and make a traced reference.
	TracedReference<Type> makeReference(Type* obj) noexcept { retarget(obj); return TracedReference{ _shared }; }

private:
	std::shared_ptr<PointerWrapper<Type>> _shared;

};