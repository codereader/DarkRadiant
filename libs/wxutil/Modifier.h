#pragma once

#include "i18n.h"
#include <wx/event.h>
#include <vector>
#include <string>
#include "xmlutil/Node.h"
#include "string/split.h"

namespace wxutil
{

class Modifier
{

#define ATTR_MODIFIER ("modifiers")

#define MODIFIERSTR_SHIFT   "SHIFT"
#define MODIFIERSTR_ALT     "ALT"
#define MODIFIERSTR_CONTROL "CONTROL"

public:
    enum Flags
    {
        NONE = 0,
        /* Used by wxutil::MouseButton
        LEFT = 1 << 1,
        RIGHT = 1 << 2,
        MIDDLE = 1 << 3,
        AUX1 = 1 << 4,
        AUX2 = 1 << 5,
        */
        SHIFT = 1 << 6,
        CONTROL = 1 << 7,
        ALT = 1 << 8
    };

    // Translates the given wxMouseEvent to modifier flags.
    static unsigned int GetStateForMouseEvent(wxMouseEvent& ev)
    {
        unsigned int state = NONE;

        if (ev.ShiftDown())
        {
            state |= SHIFT;
        }
        else
        {
            state &= ~SHIFT;
        }

        if (ev.ControlDown())
        {
            state |= CONTROL;
        }
        else
        {
            state &= ~CONTROL;
        }

        if (ev.AltDown())
        {
            state |= ALT;
        }
        else
        {
            state &= ~ALT;
        }

        return state;
    }

    // Translates the given wxKeyEvent to modifier flags.
    static unsigned int GetStateForKeyEvent(wxKeyEvent& ev)
    {
        unsigned int state = NONE;

        if (ev.ShiftDown())
        {
            state |= SHIFT;
        }
        else
        {
            state &= ~SHIFT;
        }

        if (ev.ControlDown())
        {
            state |= CONTROL;
        }
        else
        {
            state &= ~CONTROL;
        }

        if (ev.AltDown())
        {
            state |= ALT;
        }
        else
        {
            state &= ~ALT;
        }

        return state;
    }

    // Converts e.g. "SHIFT+ALT" to flags
    static unsigned int GetStateFromModifierString(const std::string& modifierStr)
    {
        unsigned int state = NONE;

        std::vector<std::string> parts;
        string::split(parts, modifierStr, "+");

        for (const std::string& mod : parts)
        {
            if (mod == MODIFIERSTR_SHIFT) { state |= SHIFT; continue; }
            if (mod == MODIFIERSTR_ALT) { state |= ALT; continue; }
            if (mod == MODIFIERSTR_CONTROL) { state |= CONTROL; continue; }
        }

        return state;
    }

    static unsigned int LoadFromNode(const xml::Node& node)
    {
        return GetStateFromModifierString(node.getAttributeValue(ATTR_MODIFIER));
    }

    static std::string GetModifierString(unsigned int state)
    {
        std::string mod = "";

        if ((state & ALT) != 0) mod += mod.empty() ? MODIFIERSTR_ALT : "+" MODIFIERSTR_ALT;
        if ((state & CONTROL) != 0) mod += mod.empty() ? MODIFIERSTR_CONTROL : "+" MODIFIERSTR_CONTROL;
        if ((state & SHIFT) != 0) mod += mod.empty() ? MODIFIERSTR_SHIFT : "+" MODIFIERSTR_SHIFT;

        return mod;
    }

    static std::string GetModifierStringForMenu(unsigned int state, const std::string& separator = "+")
    {
        std::string mod = "";

        if ((state & ALT) != 0) mod += mod.empty() ? _("Alt") : separator + _("Alt");
        if ((state & CONTROL) != 0) mod += mod.empty() ? _("Ctrl") : separator + _("Ctrl");
        if ((state & SHIFT) != 0) mod += mod.empty() ? _("Shift") : separator + _("Shift");

        return mod;
    }

    static void SaveToNode(unsigned int state, xml::Node& node)
    {
        node.setAttributeValue(ATTR_MODIFIER, GetModifierString(state));
    }
};

} // namespace
