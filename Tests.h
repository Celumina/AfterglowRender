#pragma once
#include <iostream>
#include "Inreflect.h"

namespace reflectionTest {

	class TestBaseBase {
		INR_ENABLE_PRIVATE_REFLECTION(TestBaseBase);
		static std::string basedBasedFunc(float input) { return "It's MyGO!!!!!"; };
	public:
		int id = 99;
	};

	struct OtherTestBa {
		static std::string staticBranch(const std::string& str) { return std::string("---[[[[[") + str + "]]]]]---"; };
		float whatFunc(float a) { return a * fade; };
		// Unit test: align of double, float
		float fade = 1.23;
		double calmM = 789.0123;
	};

	class TestBase : public TestBaseBase, public OtherTestBa {
		INR_ENABLE_PRIVATE_REFLECTION(TestBase);
	public:
		static inline float basefloat = 456.311;
		std::string basestr = "This is a base sstring...";
	};

	struct OtherTest {
		std::string foundMe() { return std::string("You found me! ") + std::to_string(calm); };
		int calm = 17921;
	};

	class TestClass : public TestBase, public OtherTest {
	public:
		int testFunc(int a) const { return a * some; };
		static int otherStaticFunc(float a) { return a * a; };

		float test = 6.06;
		int some = 45;
		std::string astr;
		static constexpr auto t = std::make_tuple((TestClass*)nullptr, 1, "What");
		using Type = std::remove_pointer_t<std::tuple_element_t<0, decltype(t)>>;
		static inline std::string someText{ "Hello Reflection." };
	};

	enum class TestEnum {
		TestEnumNameA,
		ThisIsEnumNameB,
		CCC,
		DDD,
		AllOfThese
	};

	INR_CLASS(TestEnum) {
		INR_ATTRS(
			INR_ENUM(TestEnumNameA),
			INR_ENUM(ThisIsEnumNameB),
			INR_ENUM(CCC),
			INR_ENUM(DDD),
			INR_ENUM(AllOfThese)
		);
	};

	// Usage (Marco)
	INR_CLASS(TestBaseBase) {
		INR_ATTRS(
			INR_ATTR(id)
		);
		INR_FUNCS(
			INR_STATIC_FUNC(basedBasedFunc)
		);
	};

	INR_CLASS(OtherTestBa) {
		INR_ATTRS(
			INR_ATTR(calmM)
		);
		INR_FUNCS(
			INR_STATIC_FUNC(staticBranch),
			INR_FUNC(whatFunc)
		);
	};

	INR_CLASS(TestBase) {
		INR_BASE_CLASSES<TestBaseBase, OtherTestBa>;
		INR_ATTRS(
			INR_STATIC_ATTR(basefloat),
			INR_ATTR(basestr)
		);
	};

	INR_CLASS(OtherTest) {
		INR_ATTRS(
			INR_ATTR(calm)
		);
		INR_FUNCS(
			INR_FUNC(foundMe)
		);
	};

	INR_CLASS(TestClass) {
		INR_BASE_CLASSES<TestBase, OtherTest>;
		INR_FUNCS(
			INR_FUNC(testFunc),
			INR_STATIC_FUNC(otherStaticFunc)
		);
		INR_ATTRS(
			INR_ATTR(test),
			INR_ATTR(some),
			INR_STATIC_ATTR(someText)
		);
	};

	void test() {
		TestClass tc;
		tc.some = 12445;

		auto valname = std::string("so") + "me";
		auto& val = *Inreflect<TestClass>::attribute<int>(tc, valname);
		std::cout << "Get a int of some: " << val << "\n";

		auto& sval = *Inreflect<TestClass>::attribute<float, TEMPLATE_STR("basefloat")>();
		std::cout << "static attribute: " << sval << "\n";

		auto* tryval = Inreflect<TestClass>::attribute<float>(tc, "some");
		if (!tryval) {
			std::cout << "Try val not found" << "\n";
		}

		std::cout << "Get a static: " << *Inreflect<TestClass>::attribute<std::string>("someText") << "\n";

		std::cout << "Output function directly: " << (tc.*Inreflect<TestClass>::function<float(OtherTestBa::*)(float), TEMPLATE_STR("whatFunc")>())(123.0) << "\n";

		std::cout << typeid(Inreflect<TestClass>::BaseClasses).name() << " \n";
		Inreflect<TestClass>::forEachAttribute([&tc](auto typeInfo) {
			std::cout << typeInfo.name << ": ";

			if constexpr (typeInfo.isType<int>()) {
				std::cout << " This value is int: ";
			}

			if constexpr (typeInfo.isStatic) {
				std::cout << typeInfo.value << "\n";
			}
			else {
				std::cout << typeInfo.value(tc) << "\n";
			}
		});

		Inreflect<TestClass>::forEachFunction([&tc](auto typeInfo) {
			if constexpr (typeInfo.name == "whatFunc") {
				std::cout << " CallFunc: " << typeInfo.name << ", result: " << typeInfo.call(tc, 1000.0f) << "\n";
			}
			if constexpr (typeInfo.name == "testFunc") {
				std::cout << " CallFunc: " << typeInfo.name << ", result: " << typeInfo.call(tc, 10) << "\n";
			}
			if constexpr (typeInfo.name == "staticBranch") {
				std::cout << " CallFunc: " << typeInfo.name << ", result: " << typeInfo.call("Hello Function") << "\n";
			}

			if constexpr (typeInfo.sameParamTypes<float>() && typeInfo.isStatic) {
				std::cout << " BatchCall: " << typeInfo.name << ", result: " << typeInfo.call(123.0f) << "\n";
			}
			if constexpr (typeInfo.sameParamTypes<>() && typeInfo.sameReturnType<std::string>() && !typeInfo.isStatic) {
				std::cout << " FilterCall: " << typeInfo.name << ", result: " << typeInfo.call(tc) << "\n";
			}
		});

		Inreflect<TestEnum>::forEachAttribute([](auto enumInfo) {
			std::cout << "Enum: " << enumInfo.name << ", Value: " << enumInfo.value << '\n';
		});
	}
};


#include "AfterglowStructLayout.h"
namespace structLayoutTest {
	void test() {
		AfterglowStructLayout layout;
		layout.addAttribute(AfterglowStructLayout::AttributeType::Float, "member0");
		layout.addAttribute(AfterglowStructLayout::AttributeType::Float4, "member1");
		layout.addAttribute(AfterglowStructLayout::AttributeType::Float, "member2");
		layout.addAttribute(AfterglowStructLayout::AttributeType::Float, "member3");

		std::cout << "ByteOffset: " << layout.offset("member3") << '\n';
	};
}

namespace partialSpecificationTest {
	template<typename D>
	struct AA {
		using Tag = D;
	};

	template<typename D>
	struct A : AA<D> {
		using Tag = D;
	};

	struct B : public A<B> {};

	template<typename T>
	struct S {};

	template<typename T>
	concept DeriviedFromA = std::is_same_v<typename A<T>::Tag, T>;
	template<DeriviedFromA T>
	struct S<A<T>> {};

	template<typename T>
	concept DeriviedFromAA = std::is_same_v<typename AA<T>::Tag, T>;
	template<DeriviedFromAA T>
	struct S<AA<T>> {};

	//
	//template<>
	//struct S<B> {};

	using TestAA = S<AA<B>>;
	// using TestA = S<A<B>>;
	// using TestB = S<A1<B>>;
	// using TestC = S<B>;

	void Test() {
		TestAA{};
		// TestA{};
		// TestB{};
		// TestC{};

	}
}
