#include "Accelerator.h"

#include <cctype>
#include <wx/defs.h>
#include <boost/algorithm/string/case_conv.hpp>

// Construct an accelerator out of the key/modifier plus a command
Accelerator::Accelerator(const unsigned int key,
						 const unsigned int modifiers,
						 const IEventPtr& event) :
	_key(key),
	_modifiers(modifiers),
	_event(event)
{}

Accelerator::Accelerator(const Accelerator& other) :
	IAccelerator(),
	_key(other._key),
	_modifiers(other._modifiers),
	_event(other._event)
{}

// Returns true if the key/modifier combination matches this accelerator
bool Accelerator::match(const unsigned int key, const unsigned int modifiers) const {
	return (_key == key && _modifiers == modifiers);
}

bool Accelerator::match(const IEventPtr& event) const {
	// Only return true if the internal event is not NULL, otherwise false positives may be returned
	return (!_event->empty() && _event == event);
}

unsigned int Accelerator::getKey() const {
	return _key;
}

unsigned int Accelerator::getModifiers() const {
	return _modifiers;
}

void Accelerator::setKey(const unsigned int key) {
	_key = key;
}

// Make the accelerator use the specified accelerators
void Accelerator::setModifiers(const unsigned int modifiers) {
	_modifiers = modifiers;
}

// Connect this modifier to the specified command
void Accelerator::connectEvent(const IEventPtr& event) {
	_event = event;
}

IEventPtr Accelerator::getEvent() {
	return _event;
}

void Accelerator::keyUp() {
	_event->keyUp();
}

void Accelerator::keyDown() {
	_event->keyDown();
}

unsigned int Accelerator::getKeyCodeFromName(const std::string& name)
{
	if (name.length() == 0)
	{
		return WXK_NONE; // empty strings
	}

	// Single-char representations
	if (name.length() == 1)
	{
		return toupper(name[0]);
	}
	else // words like TAB, ESCAPE, etc.
	{
		std::string upper = boost::algorithm::to_upper_copy(name);

		// These are compatible with the ones defined in gdkkeysyms.h
		// to be able to load legacy input.xml files
		if (upper == "SPACE") return WXK_SPACE;
		if (upper == "BACKSPACE") return WXK_BACK;
		if (upper == "ESCAPE") return WXK_ESCAPE;
		if (upper == "TAB") return WXK_TAB;
		if (upper == "ISO_LEFT_TAB") return WXK_TAB;
		if (upper == "RETURN") return WXK_RETURN;

		if (upper == "KP_SUBTRACT") return WXK_NUMPAD_SUBTRACT;
		if (upper == "KP_ADD") return WXK_NUMPAD_ADD;
		if (upper == "KP_MULTIPLY") return WXK_NUMPAD_MULTIPLY;
		if (upper == "KP_DIVIDE") return WXK_NUMPAD_DIVIDE;
		if (upper == "KP_DELETE") return WXK_NUMPAD_DELETE;
		if (upper == "KP_INSERT") return WXK_NUMPAD_INSERT;
		if (upper == "KP_HOME") return WXK_NUMPAD_HOME;
		if (upper == "KP_END") return WXK_NUMPAD_END;
		if (upper == "KP_LEFT") return WXK_NUMPAD_LEFT;
		if (upper == "KP_RIGHT") return WXK_NUMPAD_RIGHT;
		if (upper == "KP_UP") return WXK_NUMPAD_UP;
		if (upper == "KP_DOWN") return WXK_NUMPAD_DOWN;
		if (upper == "KP_PAGE_UP") return WXK_NUMPAD_PAGEUP;
		if (upper == "KP_PAGE_DOWN") return WXK_NUMPAD_PAGEDOWN;

		if (upper == "NEXT") return WXK_PAGEDOWN;
		if (upper == "PRIOR") return WXK_PAGEUP;
		if (upper == "UP") return WXK_UP;
		if (upper == "DOWN") return WXK_DOWN;
		if (upper == "LEFT") return WXK_LEFT;
		if (upper == "RIGHT") return WXK_RIGHT;
		if (upper == "DELETE") return WXK_DELETE;
		if (upper == "INSERT") return WXK_INSERT;

		if (upper == "F1") return WXK_F1;
		if (upper == "F2") return WXK_F2;
		if (upper == "F3") return WXK_F3;
		if (upper == "F4") return WXK_F4;
		if (upper == "F5") return WXK_F5;
		if (upper == "F6") return WXK_F6;
		if (upper == "F7") return WXK_F7;
		if (upper == "F8") return WXK_F8;
		if (upper == "F9") return WXK_F9;
		if (upper == "F10") return WXK_F10;
		if (upper == "F11") return WXK_F11;
		if (upper == "F12") return WXK_F12;

		if (upper == "PERIOD") return '.';
		if (upper == "COMMA") return ',';
		if (upper == "MINUS") return '-';
		if (upper == "PLUS") return '+';
	}

	return WXK_NONE;
}

std::string Accelerator::getNameFromKeyCode(unsigned int keyCode)
{
	if (keyCode == WXK_NONE)
	{
		return "";
	}

	// These are compatible with the ones defined in gdkkeysyms.h
	if (keyCode == WXK_SPACE) return "SPACE";
	if (keyCode == WXK_BACK) return "BACKSPACE";
	if (keyCode == WXK_ESCAPE) return "ESCAPE";
	if (keyCode == WXK_TAB) return "TAB";
	if (keyCode == WXK_TAB) return "ISO_LEFT_TAB";
	if (keyCode == WXK_RETURN) return "RETURN";

	if (keyCode == WXK_NUMPAD_SUBTRACT) return "KP_SUBTRACT";
	if (keyCode == WXK_NUMPAD_ADD) return "KP_ADD";
	if (keyCode == WXK_NUMPAD_MULTIPLY) return "KP_MULTIPLY";
	if (keyCode == WXK_NUMPAD_DIVIDE) return "KP_DIVIDE";
	if (keyCode == WXK_NUMPAD_DELETE) return "KP_DELETE";
	if (keyCode == WXK_NUMPAD_INSERT) return "KP_INSERT";
	if (keyCode == WXK_NUMPAD_HOME) return "KP_HOME";
	if (keyCode == WXK_NUMPAD_END) return "KP_END";
	if (keyCode == WXK_NUMPAD_LEFT) return "KP_LEFT";
	if (keyCode == WXK_NUMPAD_RIGHT) return "KP_RIGHT";
	if (keyCode == WXK_NUMPAD_UP) return "KP_UP";
	if (keyCode == WXK_NUMPAD_DOWN) return "KP_DOWN";
	if (keyCode == WXK_NUMPAD_PAGEUP) return "KP_PAGE_UP";
	if (keyCode == WXK_NUMPAD_PAGEDOWN) return "KP_PAGE_DOWN";

	if (keyCode == WXK_PAGEDOWN) return "NEXT";
	if (keyCode == WXK_PAGEUP) return "PRIOR";
	if (keyCode == WXK_UP) return "UP";
	if (keyCode == WXK_DOWN) return "DOWN";
	if (keyCode == WXK_LEFT) return "LEFT";
	if (keyCode == WXK_RIGHT) return "RIGHT";
	if (keyCode == WXK_DELETE) return "DELETE";
	if (keyCode == WXK_INSERT) return "INSERT";

	if (keyCode == WXK_F1) return "F1";
	if (keyCode == WXK_F2) return "F2";
	if (keyCode == WXK_F3) return "F3";
	if (keyCode == WXK_F4) return "F4";
	if (keyCode == WXK_F5) return "F5";
	if (keyCode == WXK_F6) return "F6";
	if (keyCode == WXK_F7) return "F7";
	if (keyCode == WXK_F8) return "F8";
	if (keyCode == WXK_F9) return "F9";
	if (keyCode == WXK_F10) return "F10";
	if (keyCode == WXK_F11) return "F11";
	if (keyCode == WXK_F12) return "F12";

	if (keyCode == '.') return "PERIOD";
	if (keyCode == ',') return "COMMA";
	if (keyCode == '-') return "MINUS";
	if (keyCode == '+') return "PLUS";

	// For ASCII characters just return string with the single character
	if (keyCode < std::numeric_limits<char>::max())
	{
		char upper = toupper(static_cast<char>(keyCode));
		return std::string(1, upper);
	}

	return "";
}