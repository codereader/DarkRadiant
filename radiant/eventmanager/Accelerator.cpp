#include "Accelerator.h"

#include "itextstream.h"
#include <cctype>
#include <wx/defs.h>
#include "wxutil/Modifier.h"
#include "string/case_conv.h"

namespace ui
{

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

bool Accelerator::match(const IEventPtr& event) const
{
    // Only return true if the internal event is not NULL, otherwise false positives may be returned
    return _event == event && !_event->empty();
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

const IEventPtr& Accelerator::getEvent()
{
    return _event;
}

void Accelerator::setEvent(const IEventPtr& ev)
{
    _event = ev;
}

void Accelerator::keyUp() {
    _event->keyUp();
}

void Accelerator::keyDown() {
    _event->keyDown();
}

std::string Accelerator::getAcceleratorString(bool forMenu)
{
    const std::string keyStr = _key != 0 ? Accelerator::getNameFromKeyCode(_key) : "";

    if (!keyStr.empty())
    {
        // Return a modifier string for a menu
        const std::string modifierStr = forMenu ?
            wxutil::Modifier::GetModifierStringForMenu(_modifiers) :
            wxutil::Modifier::GetModifierString(_modifiers);

        const std::string connector = (forMenu) ? "~" : "+";

        std::string returnValue = modifierStr;
        returnValue += !modifierStr.empty() ? connector : "";
        returnValue += keyStr;

        return returnValue;
    }

    return "";
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
        std::string upper = string::to_upper_copy(name);

        // These are compatible with the ones defined in gdkkeysyms.h
        // to be able to load legacy input.xml files
        if (upper == "SPACE") return WXK_SPACE;
        if (upper == "BACKSPACE") return WXK_BACK;
        if (upper == "ESCAPE") return WXK_ESCAPE;
        if (upper == "TAB") return WXK_TAB;
        if (upper == "ISO_LEFT_TAB") return WXK_TAB;
        if (upper == "RETURN") return WXK_RETURN;
        if (upper == "PAUSE") return WXK_PAUSE;
        if (upper == "CAPITAL") return WXK_CAPITAL;
        if (upper == "SELECT") return WXK_SELECT;
        if (upper == "PRINT") return WXK_PRINT;
        if (upper == "EXECUTE") return WXK_EXECUTE;
        if (upper == "SNAPSHOT") return WXK_SNAPSHOT;
        if (upper == "HELP") return WXK_HELP;

        if (upper == "NUMPAD0") return WXK_NUMPAD0;
        if (upper == "NUMPAD1") return WXK_NUMPAD1;
        if (upper == "NUMPAD2") return WXK_NUMPAD2;
        if (upper == "NUMPAD3") return WXK_NUMPAD3;
        if (upper == "NUMPAD4") return WXK_NUMPAD4;
        if (upper == "NUMPAD5") return WXK_NUMPAD5;
        if (upper == "NUMPAD6") return WXK_NUMPAD6;
        if (upper == "NUMPAD7") return WXK_NUMPAD7;
        if (upper == "NUMPAD8") return WXK_NUMPAD8;
        if (upper == "NUMPAD9") return WXK_NUMPAD9;

        if (upper == "CLEAR") return WXK_CLEAR;
        if (upper == "KP_SUBTRACT" || upper == "NUMPAD_SUBTRACT") return WXK_NUMPAD_SUBTRACT;
        if (upper == "KP_ADD" || upper == "NUMPAD_ADD") return WXK_NUMPAD_ADD;
        if (upper == "KP_MULTIPLY" || upper == "NUMPAD_MULTIPLY") return WXK_NUMPAD_MULTIPLY;
        if (upper == "KP_DIVIDE" || upper == "NUMPAD_DIVIDE") return WXK_NUMPAD_DIVIDE;
        if (upper == "KP_DELETE" || upper == "NUMPAD_DELETE") return WXK_NUMPAD_DELETE;
        if (upper == "KP_INSERT" || upper == "NUMPAD_INSERT") return WXK_NUMPAD_INSERT;
        if (upper == "KP_HOME" || upper == "NUMPAD_HOME") return WXK_NUMPAD_HOME;
        if (upper == "KP_END" || upper == "NUMPAD_END") return WXK_NUMPAD_END;
        if (upper == "KP_LEFT" || upper == "NUMPAD_LEFT") return WXK_NUMPAD_LEFT;
        if (upper == "KP_RIGHT" || upper == "NUMPAD_RIGHT") return WXK_NUMPAD_RIGHT;
        if (upper == "KP_UP" || upper == "NUMPAD_UP") return WXK_NUMPAD_UP;
        if (upper == "KP_DOWN" || upper == "NUMPAD_DOWN") return WXK_NUMPAD_DOWN;
        if (upper == "KP_PAGE_UP" || upper == "NUMPAD_PAGEUP") return WXK_NUMPAD_PAGEUP;
        if (upper == "KP_PAGE_DOWN" || upper == "NUMPAD_PAGEDOWN") return WXK_NUMPAD_PAGEDOWN;
        if (upper == "NUMPAD_SPACE") return WXK_NUMPAD_SPACE;
        if (upper == "NUMPAD_TAB") return WXK_NUMPAD_TAB;
        if (upper == "NUMPAD_ENTER") return WXK_NUMPAD_ENTER;
        if (upper == "NUMPAD_F1") return WXK_NUMPAD_F1;
        if (upper == "NUMPAD_F2") return WXK_NUMPAD_F2;
        if (upper == "NUMPAD_F3") return WXK_NUMPAD_F3;
        if (upper == "NUMPAD_F4") return WXK_NUMPAD_F4;
        if (upper == "NUMPAD_BEGIN") return WXK_NUMPAD_BEGIN;
        if (upper == "NUMPAD_EQUAL") return WXK_NUMPAD_EQUAL;
        if (upper == "NUMPAD_SEPARATOR") return WXK_NUMPAD_SEPARATOR;
        if (upper == "NUMPAD_DECIMAL") return WXK_NUMPAD_DECIMAL;

        if (upper == "MULTIPLY") return WXK_MULTIPLY;
        if (upper == "ADD") return WXK_ADD;
        if (upper == "SEPARATOR") return WXK_SEPARATOR;
        if (upper == "SUBTRACT") return WXK_SUBTRACT;
        if (upper == "DECIMAL") return WXK_DECIMAL;
        if (upper == "DIVIDE") return WXK_DIVIDE;

        if (upper == "NEXT" || upper == "PAGE_DOWN" || upper == "PAGEDOWN") return WXK_PAGEDOWN;
        if (upper == "PRIOR" || upper == "PAGE_UP" || upper == "PAGEUP") return WXK_PAGEUP;
        if (upper == "UP") return WXK_UP;
        if (upper == "DOWN") return WXK_DOWN;
        if (upper == "LEFT") return WXK_LEFT;
        if (upper == "RIGHT") return WXK_RIGHT;
        if (upper == "DELETE") return WXK_DELETE;
        if (upper == "INSERT") return WXK_INSERT;
        if (upper == "END") return WXK_END;
        if (upper == "HOME") return WXK_HOME;

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
        if (upper == "F13") return WXK_F13;
        if (upper == "F14") return WXK_F14;
        if (upper == "F15") return WXK_F15;
        if (upper == "F16") return WXK_F16;
        if (upper == "F17") return WXK_F17;
        if (upper == "F18") return WXK_F18;
        if (upper == "F19") return WXK_F19;
        if (upper == "F20") return WXK_F20;
        if (upper == "F21") return WXK_F21;
        if (upper == "F22") return WXK_F22;
        if (upper == "F23") return WXK_F23;
        if (upper == "F24") return WXK_F24;

        if (upper == "NUMLOCK") return WXK_NUMLOCK;
        if (upper == "SCROLL") return WXK_SCROLL;

        if (upper == "PERIOD") return '.';
        if (upper == "COMMA") return ',';
        if (upper == "MINUS") return '-';
        if (upper == "PLUS") return '+';
        if (upper == "BACKSLASH") return '\\';
    }

    rWarning() << "[Accelerator] Could not resolve keycode from name " << name << std::endl;

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
    if (keyCode == WXK_PAUSE) return "PAUSE";
    if (keyCode == WXK_CAPITAL) return "CAPITAL";
    if (keyCode == WXK_SELECT) return "SELECT";
    if (keyCode == WXK_PRINT) return "PRINT";
    if (keyCode == WXK_EXECUTE) return "EXECUTE";
    if (keyCode == WXK_SNAPSHOT) return "SNAPSHOT";
    if (keyCode == WXK_HELP) return "HELP";

    if (keyCode == WXK_NUMPAD0) return "NUMPAD0";
    if (keyCode == WXK_NUMPAD1) return "NUMPAD1";
    if (keyCode == WXK_NUMPAD2) return "NUMPAD2";
    if (keyCode == WXK_NUMPAD3) return "NUMPAD3";
    if (keyCode == WXK_NUMPAD4) return "NUMPAD4";
    if (keyCode == WXK_NUMPAD5) return "NUMPAD5";
    if (keyCode == WXK_NUMPAD6) return "NUMPAD6";
    if (keyCode == WXK_NUMPAD7) return "NUMPAD7";
    if (keyCode == WXK_NUMPAD8) return "NUMPAD8";
    if (keyCode == WXK_NUMPAD9) return "NUMPAD9";

    if (keyCode == WXK_CLEAR) return "CLEAR";
    if (keyCode == WXK_NUMPAD_SUBTRACT) return "NUMPAD_SUBTRACT";
    if (keyCode == WXK_NUMPAD_ADD) return "NUMPAD_ADD";
    if (keyCode == WXK_NUMPAD_MULTIPLY) return "NUMPAD_MULTIPLY";
    if (keyCode == WXK_NUMPAD_DIVIDE) return "NUMPAD_DIVIDE";
    if (keyCode == WXK_NUMPAD_DELETE) return "NUMPAD_DELETE";
    if (keyCode == WXK_NUMPAD_INSERT) return "NUMPAD_INSERT";
    if (keyCode == WXK_NUMPAD_HOME) return "NUMPAD_HOME";
    if (keyCode == WXK_NUMPAD_END) return "NUMPAD_END";
    if (keyCode == WXK_NUMPAD_LEFT) return "NUMPAD_LEFT";
    if (keyCode == WXK_NUMPAD_RIGHT) return "NUMPAD_RIGHT";
    if (keyCode == WXK_NUMPAD_UP) return "NUMPAD_UP";
    if (keyCode == WXK_NUMPAD_DOWN) return "NUMPAD_DOWN";
    if (keyCode == WXK_NUMPAD_PAGEUP) return "NUMPAD_PAGEUP";
    if (keyCode == WXK_NUMPAD_PAGEDOWN) return "NUMPAD_PAGEDOWN";
    if (keyCode == WXK_NUMPAD_SPACE) return "NUMPAD_SPACE";
    if (keyCode == WXK_NUMPAD_TAB) return "NUMPAD_TAB";
    if (keyCode == WXK_NUMPAD_ENTER) return "NUMPAD_ENTER";
    if (keyCode == WXK_NUMPAD_F1) return "NUMPAD_F1";
    if (keyCode == WXK_NUMPAD_F2) return "NUMPAD_F2";
    if (keyCode == WXK_NUMPAD_F3) return "NUMPAD_F3";
    if (keyCode == WXK_NUMPAD_F4) return "NUMPAD_F4";
    if (keyCode == WXK_NUMPAD_BEGIN) return "NUMPAD_BEGIN";
    if (keyCode == WXK_NUMPAD_EQUAL) return "NUMPAD_EQUAL";
    if (keyCode == WXK_NUMPAD_SEPARATOR) return "NUMPAD_SEPARATOR";
    if (keyCode == WXK_NUMPAD_DECIMAL) return "NUMPAD_DECIMAL";

    if (keyCode == WXK_MULTIPLY) return "MULTIPLY";
    if (keyCode == WXK_ADD) return "ADD";
    if (keyCode == WXK_SEPARATOR) return "SEPARATOR";
    if (keyCode == WXK_SUBTRACT) return "SUBTRACT";
    if (keyCode == WXK_DECIMAL) return "DECIMAL";
    if (keyCode == WXK_DIVIDE) return "DIVIDE";

    if (keyCode == WXK_PAGEDOWN) return "PAGEDOWN";
    if (keyCode == WXK_PAGEUP) return "PAGEUP";
    if (keyCode == WXK_UP) return "UP";
    if (keyCode == WXK_DOWN) return "DOWN";
    if (keyCode == WXK_LEFT) return "LEFT";
    if (keyCode == WXK_RIGHT) return "RIGHT";
    if (keyCode == WXK_DELETE) return "DELETE";
    if (keyCode == WXK_INSERT) return "INSERT";
    if (keyCode == WXK_END) return "END";
    if (keyCode == WXK_HOME) return "HOME";

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
    if (keyCode == WXK_F13) return "F13";
    if (keyCode == WXK_F14) return "F14";
    if (keyCode == WXK_F15) return "F15";
    if (keyCode == WXK_F16) return "F16";
    if (keyCode == WXK_F17) return "F17";
    if (keyCode == WXK_F18) return "F18";
    if (keyCode == WXK_F19) return "F19";
    if (keyCode == WXK_F20) return "F20";
    if (keyCode == WXK_F21) return "F21";
    if (keyCode == WXK_F22) return "F22";
    if (keyCode == WXK_F23) return "F23";
    if (keyCode == WXK_F24) return "F24";

    if (keyCode == WXK_NUMLOCK) return "NUMLOCK";
    if (keyCode == WXK_SCROLL) return "SCROLL";

    if (keyCode == '.') return "PERIOD";
    if (keyCode == ',') return "COMMA";
    if (keyCode == '-') return "MINUS";
    if (keyCode == '+') return "PLUS";

    // For ASCII characters just return string with the single character
    if (static_cast<int>(keyCode) < std::numeric_limits<char>::max())
    {
        char upper = toupper(static_cast<char>(keyCode));
        return std::string(1, upper);
    }

    return "";
}

}
