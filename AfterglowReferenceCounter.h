#pragma once
#include <stdint.h>
#include <functional>

class AfterglowReferenceCount {
public:
	using Count = int64_t;

	using IncreaseCallback = void(Count);
	using DecreaseCallback = void(Count);

	AfterglowReferenceCount(
		std::function<IncreaseCallback> increaseCallback = nullptr, 
		std::function<DecreaseCallback> decreaseCallback = nullptr
	);

	void increase();
	void decrease();

	Count count() const;

	void setIncreaseCallback(std::function<IncreaseCallback> increaseCallback);
	void setDecreaseCallback(std::function<DecreaseCallback> decreaseCallback);

private:
	std::function<IncreaseCallback> _increaseCallback;
	std::function<DecreaseCallback> _decreaseCallback;

	Count _count = 0;
};

class AfterglowReferenceCounter {
public:
	AfterglowReferenceCounter(AfterglowReferenceCount& count);
	AfterglowReferenceCounter(const AfterglowReferenceCounter& other);
	~AfterglowReferenceCounter();

	AfterglowReferenceCount& count();
	const AfterglowReferenceCount& count() const;

private:
	AfterglowReferenceCount& _count;
};