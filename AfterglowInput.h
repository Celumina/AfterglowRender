#pragma once
#include <array>
#include <memory>
#include <mutex>
#include <glm/glm.hpp>
#include <unordered_map>

#include "AfterglowObject.h"
#include "AfterglowUtilities.h"


class AfterglowInput : public AfterglowObject {
public:
	using Position = glm::dvec2;

	enum class Key {
		Undefined, 

		// Function Keys
		Escape, 
		Enter, 
		Tab, 
		Backspace, 
		Insert, 
		Delete, 
		Up,
		Down,
		Left,
		Right, 
		PageUp, 
		PageDown, 
		Home, 
		End, 
		CapsLock, 
		ScrollLock, 
		NumLock, 
		PrintScreen, 
		Pause, 
		Menu, 

		// Char Keys
		F1, 
		F2, 
		F3, 
		F4, 
		F5, 
		F6, 
		F7, 
		F8, 
		F9, 
		F10, 
		F11, 
		F12, 
		Space, //  
		Apostrophe, // '
		Comma, // ,
		Minus, // -
		Equal, // =
		Period, // .
		Slash, // /
		Backslash, // \ <-oops, it can't not put in end of the line.
		Semicolon, // ;
		LeftBracket, // [
		RightBracket, // ]
		GraveAccent, // `
		Num0, 
		Num1, 
		Num2, 
		Num3, 
		Num4, 
		Num5, 
		Num6, 
		Num7, 
		Num8, 
		Num9, 
		A, 
		B, 
		C, 
		D, 
		E, 
		F, 
		G, 
		H, 
		I, 
		J, 
		K, 
		L, 
		M, 
		N, 
		O, 
		P, 
		Q, 
		R, 
		S, 
		T, 
		U, 
		V, 
		W, 
		X, 
		Y, 
		Z, 

		// Keypad Keys
		KeypadNum0, 
		KeypadNum1, 
		KeypadNum2, 
		KeypadNum3, 
		KeypadNum4, 
		KeypadNum5, 
		KeypadNum6, 
		KeypadNum7, 
		KeypadNum8, 
		KeypadNum9, 
		KeypadDecimal, 
		KeypadDivide, 
		KeypadMultiply, 
		KeypadSubtract, 
		KeypadAdd, 
		KeypadEnter, 
		KeypadEqual, 

		// Modifier Keys
		LeftShift,
		LeftAlt,
		LeftControl,
		LeftSuper,
		RightShift,
		RightAlt,
		RightControl,
		RightSuper,

		EnumCount
	};

	enum class MouseButton {
		Undefined,
		Left, 
		Right, 
		Middle, 

		EnumCount
	};

	enum class Modifier {
		Undefined = 0 << 0,
		Shift = 0 << 1, 
		Alt = 0 << 2, 
		Control = 0 << 3, 
		Super = 0 << 4, 

		EnumCount
	};

	enum class State {
		Undefined, 
		PressDown, 
		Pressed, 
		Repeat, 
		ReleaseUp, 
		Released, 

		EnumCount
	};

	using Mutex = std::mutex;
	using LockGuard = std::lock_guard<Mutex>;
	using UnstableStates = std::unordered_map<uint32_t, State>;

	AfterglowInput();
	~AfterglowInput();

	// TODO: No blocking by GUI methods.
	bool modifiedWith(MouseButton mouseButton, Modifier modifiers) const;
	bool modifiedWith(Key key, Modifier modifiers) const;

	bool pressDown(MouseButton mouseButton) const;
	bool pressDown(Key key) const;

	bool pressed(MouseButton mouseButton) const;
	bool pressed(Key key) const;

	bool releaseUp(MouseButton mouseButton) const;
	bool releaseUp(Key key) const;

	bool released(MouseButton mouseButton) const;
	bool released(Key key) const;

	// Hold press for a certain amount of time.
	bool repeated(MouseButton mouseButton) const;
	bool repeated(Key key) const;

	Position cursorPosition() const;
	Position wheelOffset() const;
	bool cursorEntered() const;

	void updateKeyFromGLFW(int keyCode, int actionCode);
	void updateMouseButtonFromGLFW(int buttonCode, int actionCode);
	void updateCursorPositionFromGLFW(double posX, double posY);
	void updateScrollFromGLFW(double offsetX, double offsetY);
	void updateCursorEnteredFromGLFW(int entered);

	// @brief: Updated in one thread only, Which thread want to use input, then which thread should update it.
	void update();

	static Key keyFromGLFW(int glfwKeyCode);
	static MouseButton mouseButtonFromGLFW(int glfwMouseButtonCode);
	static State stateFromGLFW(int glfwActionCode);

private:
	template <typename EnumType>
	using KeyStateArray = std::array<State, util::EnumValue(EnumType::EnumCount)>;

	inline bool pressed(State state) const;
	inline bool released(State state) const;
	inline bool modified(Modifier modifiers) const;

	template<typename EnumType>
	inline void updateUnstableIndices(
		KeyStateArray<EnumType>& states,
		UnstableStates& unstableIndices
	);

	template<typename EnumType>
	inline void updateFromGLFWImpl(
		EnumType targetEnum, 
		State targetState, 
		KeyStateArray<EnumType>& states, 
		UnstableStates& unstables
	);

	struct Impl;
	std::unique_ptr<Impl> _impl;
	Mutex _mutex;
};

template<typename EnumType>
inline void AfterglowInput::updateUnstableIndices(KeyStateArray<EnumType>& states, UnstableStates& unstables) {
	for (auto& [index, state] : unstables) {
		states[index] = state;
	}

	std::erase_if(unstables, [](const auto& item){ 
		return item.second == State::Pressed || item.second == State::Released; 
	});

	for (auto& [index, state] : unstables) {
		if (state == State::PressDown) {
			state = State::Pressed;
		}
		else if (state == State::ReleaseUp) {
			state = State::Released;
		}
	}
}

template<typename EnumType>
inline void AfterglowInput::updateFromGLFWImpl(
	EnumType targetEnum,
	State targetState,
	KeyStateArray<EnumType>& states,
	UnstableStates& unstables) {

	LockGuard lockGuard(_mutex);

	uint32_t index = util::EnumValue(targetEnum);
	State state = states[index];
	// Don't update states directly, due to a potential thread interference.
	if (targetState == State::Pressed) {
		unstables[index] = State::PressDown;
	}
	else if (targetState == State::Released) {
		unstables[index] = State::ReleaseUp;
	}
	else {
		unstables[index] = targetState;
	}
}
