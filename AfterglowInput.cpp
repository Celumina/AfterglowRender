#include "AfterglowInput.h"

#include <algorithm>
#include <GLFW/glfw3.h>

#include "DebugUtilities.h"

struct AfterglowInput::Context {
	UnstableStates unstableKeyIndices;
	UnstableStates unstableMouseButtonIndices;
	KeyStateArray<Key> keyStates;
	KeyStateArray<MouseButton> mouseButtonStates;
	Position cursorPos;
	Position wheelOffset;
	bool cursorEntered;
};

AfterglowInput::AfterglowInput() : 
	_context(std::make_unique<Context>()) {
}

AfterglowInput::~AfterglowInput() {
}

void AfterglowInput::updateKeyFromGLFW(int keyCode, int actionCode) {
	Key key = keyFromGLFW(keyCode);
	State targetState = stateFromGLFW(actionCode);
	updateFromGLFWImpl(
		key, 
		targetState, 
		_context->keyStates, 
		_context->unstableKeyIndices
	);
}

void AfterglowInput::updateMouseButtonFromGLFW(int buttonCode, int actionCode) {
	MouseButton button = mouseButtonFromGLFW(buttonCode);
	State targetState = stateFromGLFW(actionCode);
	updateFromGLFWImpl(
		button,
		targetState,
		_context->mouseButtonStates,
		_context->unstableMouseButtonIndices
	);
}

void AfterglowInput::updateCursorPositionFromGLFW(double posX, double posY) {
	_context->cursorPos.x = posX;
	_context->cursorPos.y = posY;
}

void AfterglowInput::updateScrollFromGLFW(double offsetX, double offsetY) {
	_context->wheelOffset.x = offsetX;
	_context->wheelOffset.y = offsetY;
}

void AfterglowInput::updateCursorEnteredFromGLFW(int entered) {
	_context->cursorEntered = static_cast<bool>(entered);
}

bool AfterglowInput::modifiedWith(MouseButton mouseButton, Modifier modifiers) const {
	return pressed(mouseButton) && modified(modifiers);
}

bool AfterglowInput::modifiedWith(Key key, Modifier modifiers) const {
	return pressed(key) && modified(modifiers);
}

bool AfterglowInput::pressDown(MouseButton mouseButton) const {
	return _context->mouseButtonStates[util::EnumValue(mouseButton)] == State::PressDown;
}

bool AfterglowInput::pressDown(Key key) const {
	return _context->keyStates[util::EnumValue(key)] == State::PressDown;
}

bool AfterglowInput::pressed(MouseButton mouseButton) const {
	return pressed(_context->mouseButtonStates[util::EnumValue(mouseButton)]);
}

bool AfterglowInput::pressed(Key key) const {
	return pressed(_context->keyStates[util::EnumValue(key)]);
}

bool AfterglowInput::releaseUp(MouseButton mouseButton) const {
	return _context->mouseButtonStates[util::EnumValue(mouseButton)] == State::ReleaseUp;
}

bool AfterglowInput::releaseUp(Key key) const {
	return _context->keyStates[util::EnumValue(key)] == State::ReleaseUp;
}

bool AfterglowInput::released(MouseButton mouseButton) const {
	return released(_context->mouseButtonStates[util::EnumValue(mouseButton)]);
}

bool AfterglowInput::released(Key key) const {
	return released(_context->keyStates[util::EnumValue(key)]);
}

bool AfterglowInput::repeated(MouseButton mouseButton) const {
	return _context->mouseButtonStates[util::EnumValue(mouseButton)] == State::Repeat;
}

bool AfterglowInput::repeated(Key key) const {
	return _context->keyStates[util::EnumValue(key)] == State::Repeat;
}

AfterglowInput::Position AfterglowInput::cursorPosition() const {
	return _context->cursorPos;
}

AfterglowInput::Position AfterglowInput::wheelOffset() const {
	return _context->wheelOffset;
}

bool AfterglowInput::cursorEntered() const {
	return _context->cursorEntered;
}

inline bool AfterglowInput::pressed(State state) const {
	return state == State::Pressed || state == State::Repeat || state == State::PressDown;
}

inline bool AfterglowInput::released(State state) const {
	return state == State::Released || state == State::ReleaseUp;
}

inline bool AfterglowInput::modified(Modifier modifiers) const {
	bool modified = true;
	if (util::EnumValue(modifiers) & util::EnumValue(Modifier::Shift)) {
		modified &= (pressed(Key::LeftShift) || pressed(Key::RightShift));
	}
	if (util::EnumValue(modifiers) & util::EnumValue(Modifier::Alt)) {
		modified &= (pressed(Key::LeftAlt) || pressed(Key::RightAlt));
	}
	if (util::EnumValue(modifiers) & util::EnumValue(Modifier::Control)) {
		modified &= (pressed(Key::LeftControl) || pressed(Key::RightControl));
	}
	if (util::EnumValue(modifiers) & util::EnumValue(Modifier::Super)) {
		modified &= (pressed(Key::LeftSuper) || pressed(Key::RightSuper));
	}
	return modified;
}

void AfterglowInput::update() {
	LockGuard lockGuard(_mutex);
	updateUnstableIndices<Key>(
		_context->keyStates, _context->unstableKeyIndices
	);
	updateUnstableIndices<MouseButton>(
		_context->mouseButtonStates, _context->unstableMouseButtonIndices
	);
}

AfterglowInput::Key AfterglowInput::keyFromGLFW(int glfwKeyCode) {
	switch (glfwKeyCode) {
	case GLFW_KEY_ESCAPE: return Key::Escape;
	case GLFW_KEY_ENTER: return Key::Enter;
	case GLFW_KEY_TAB: return Key::Tab;
	case GLFW_KEY_BACKSPACE: return Key::Backspace;
	case GLFW_KEY_INSERT: return Key::Insert;
	case GLFW_KEY_DELETE: return Key::Delete;
	case GLFW_KEY_UP: return Key::Up;
	case GLFW_KEY_DOWN: return Key::Down;
	case GLFW_KEY_LEFT: return Key::Left;
	case GLFW_KEY_RIGHT: return Key::Right;
	case GLFW_KEY_PAGE_UP: return Key::PageUp;
	case GLFW_KEY_PAGE_DOWN: return Key::PageDown;
	case GLFW_KEY_HOME: return Key::Home;
	case GLFW_KEY_END: return Key::End;
	case GLFW_KEY_CAPS_LOCK: return Key::CapsLock;
	case GLFW_KEY_SCROLL_LOCK: return Key::ScrollLock;
	case GLFW_KEY_NUM_LOCK: return Key::NumLock;
	case GLFW_KEY_PRINT_SCREEN: return Key::PrintScreen;
	case GLFW_KEY_PAUSE: return Key::Pause;
	case GLFW_KEY_MENU: return Key::Menu;
	case GLFW_KEY_F1: return Key::F1;
	case GLFW_KEY_F2: return Key::F2;
	case GLFW_KEY_F3: return Key::F3;
	case GLFW_KEY_F4: return Key::F4;
	case GLFW_KEY_F5: return Key::F5;
	case GLFW_KEY_F6: return Key::F6;
	case GLFW_KEY_F7: return Key::F7;
	case GLFW_KEY_F8: return Key::F8;
	case GLFW_KEY_F9: return Key::F9;
	case GLFW_KEY_F10: return Key::F10;
	case GLFW_KEY_F11: return Key::F11;
	case GLFW_KEY_F12: return Key::F12;
	case GLFW_KEY_SPACE: return Key::Space;
	case GLFW_KEY_APOSTROPHE: return Key::Apostrophe;
	case GLFW_KEY_COMMA: return Key::Comma;
	case GLFW_KEY_MINUS: return Key::Minus;
	case GLFW_KEY_EQUAL: return Key::Equal;
	case GLFW_KEY_PERIOD: return Key::Period;
	case GLFW_KEY_SLASH: return Key::Slash;
	case GLFW_KEY_BACKSLASH: return Key::Backslash;
	case GLFW_KEY_SEMICOLON: return Key::Semicolon;
	case GLFW_KEY_LEFT_BRACKET: return Key::LeftBracket;
	case GLFW_KEY_RIGHT_BRACKET: return Key::RightBracket;
	case GLFW_KEY_GRAVE_ACCENT: return Key::GraveAccent;
	case GLFW_KEY_0: return Key::Num0;
	case GLFW_KEY_1: return Key::Num1;
	case GLFW_KEY_2: return Key::Num2;
	case GLFW_KEY_3: return Key::Num3;
	case GLFW_KEY_4: return Key::Num4;
	case GLFW_KEY_5: return Key::Num5;
	case GLFW_KEY_6: return Key::Num6;
	case GLFW_KEY_7: return Key::Num7;
	case GLFW_KEY_8: return Key::Num8;
	case GLFW_KEY_9: return Key::Num9;
	case GLFW_KEY_A: return Key::A;
	case GLFW_KEY_B: return Key::B;
	case GLFW_KEY_C: return Key::C;
	case GLFW_KEY_D: return Key::D;
	case GLFW_KEY_E: return Key::E;
	case GLFW_KEY_F: return Key::F;
	case GLFW_KEY_G: return Key::G;
	case GLFW_KEY_H: return Key::H;
	case GLFW_KEY_I: return Key::I;
	case GLFW_KEY_J: return Key::J;
	case GLFW_KEY_K: return Key::K;
	case GLFW_KEY_L: return Key::L;
	case GLFW_KEY_M: return Key::M;
	case GLFW_KEY_N: return Key::N;
	case GLFW_KEY_O: return Key::O;
	case GLFW_KEY_P: return Key::P;
	case GLFW_KEY_Q: return Key::Q;
	case GLFW_KEY_R: return Key::R;
	case GLFW_KEY_S: return Key::S;
	case GLFW_KEY_T: return Key::T;
	case GLFW_KEY_U: return Key::U;
	case GLFW_KEY_V: return Key::V;
	case GLFW_KEY_W: return Key::W;
	case GLFW_KEY_X: return Key::X;
	case GLFW_KEY_Y: return Key::Y;
	case GLFW_KEY_Z: return Key::Z;	
	case GLFW_KEY_KP_0: return Key::KeypadNum0;
	case GLFW_KEY_KP_1: return Key::KeypadNum1;
	case GLFW_KEY_KP_2: return Key::KeypadNum2;
	case GLFW_KEY_KP_3: return Key::KeypadNum3;
	case GLFW_KEY_KP_4: return Key::KeypadNum4;
	case GLFW_KEY_KP_5: return Key::KeypadNum5;
	case GLFW_KEY_KP_6: return Key::KeypadNum6;
	case GLFW_KEY_KP_7: return Key::KeypadNum7;
	case GLFW_KEY_KP_8: return Key::KeypadNum8;
	case GLFW_KEY_KP_9: return Key::KeypadNum9;
	case GLFW_KEY_KP_DECIMAL: return Key::KeypadDecimal;
	case GLFW_KEY_KP_DIVIDE: return Key::KeypadDivide;
	case GLFW_KEY_KP_MULTIPLY: return Key::KeypadMultiply;
	case GLFW_KEY_KP_SUBTRACT: return Key::KeypadSubtract;
	case GLFW_KEY_KP_ADD: return Key::KeypadAdd;
	case GLFW_KEY_KP_ENTER: return Key::KeypadEnter;
	case GLFW_KEY_KP_EQUAL: return Key::KeypadEqual;
	case GLFW_KEY_LEFT_SHIFT: return Key::LeftShift;
	case GLFW_KEY_LEFT_ALT: return Key::LeftAlt;
	case GLFW_KEY_LEFT_CONTROL: return Key::LeftControl;
	case GLFW_KEY_LEFT_SUPER: return Key::LeftSuper;
	case GLFW_KEY_RIGHT_SHIFT: return Key::RightShift;
	case GLFW_KEY_RIGHT_ALT: return Key::RightAlt;
	case GLFW_KEY_RIGHT_CONTROL: return Key::RightControl;
	case GLFW_KEY_RIGHT_SUPER: return Key::RightSuper;
	default: return Key::Undefined;
	}   
}

AfterglowInput::MouseButton AfterglowInput::mouseButtonFromGLFW(int glfwMouseButtonCode) {
	switch (glfwMouseButtonCode) {
	case GLFW_MOUSE_BUTTON_LEFT: return MouseButton::Left;
	case GLFW_MOUSE_BUTTON_RIGHT: return MouseButton::Right;
	case GLFW_MOUSE_BUTTON_MIDDLE: return MouseButton::Middle;
	default: return MouseButton::Undefined;
	}
}

AfterglowInput::State AfterglowInput::stateFromGLFW(int glfwActionCode) {
	switch (glfwActionCode) {
	case GLFW_RELEASE: return State::Released;
	case GLFW_PRESS: return State::Pressed;
	case GLFW_REPEAT: return State::Repeat;
	default: return State::Undefined;
	}
}
