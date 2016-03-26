#pragma once

#include <wx/event.h>
#include "Modifier.h"

namespace wxutil
{

class MouseButton
{

#define ATTR_BUTTON ("button")

#define BUTTONSTR_LMB "LMB"
#define BUTTONSTR_MMB "MMB"
#define BUTTONSTR_RMB "RMB"
#define BUTTONSTR_AUX1 "AUX1"
#define BUTTONSTR_AUX2 "AUX2"

public:
	enum ButtonFlags
	{
		NONE	= 0,
		LEFT	= 1 << 1,
		RIGHT	= 1 << 2,
		MIDDLE	= 1 << 3,
		AUX1	= 1 << 4,
		AUX2	= 1 << 5,
		/* Used by wxutil::Modifier
        SHIFT	= 1 << 6,
		CONTROL	= 1 << 7,
		ALT		= 1 << 8
        */
	};

    // Returns a bit mask corresponding to a single mouse button CHANGE event
    // Only one mouse button will be marked in this bit mask since only one
    // button can be changed at a given time. The keyboard modifier state
    // is added to the bit mask in the usual way, the according bit flag will
    // be set if the modifier key is currently held.
    static unsigned int GetButtonStateChangeForMouseEvent(wxMouseEvent& ev)
	{
		unsigned int newState = NONE;

		if (ev.LeftDown() || ev.LeftUp())
		{
			newState |= LEFT;
		}
		else if (ev.RightDown() || ev.RightUp())
		{
			newState |= RIGHT;
		}
		else if (ev.MiddleDown() || ev.MiddleUp())
		{
			newState |= MIDDLE;
		}
#if wxCHECK_VERSION(3,0,0)
		else if (ev.Aux1Down() || ev.Aux1Up())
		{
			newState |= AUX1;
		}
		else if (ev.Aux2Down() || ev.Aux2Up())
		{
			newState |= AUX2;
		}
#endif
        // Add the modifier key state as usual
        newState |= Modifier::GetStateForMouseEvent(ev);

		return newState;
	}
    
    // Returns a bit mask representing the current mouse and modifier state
    // Since it represents the current state it's possible to find multiple
    // mouse buttons being held at the same time.
	static unsigned int GetStateForMouseEvent(wxMouseEvent& ev)
	{
		unsigned int newState = NONE;

		if (ev.LeftIsDown())
		{
			newState |= LEFT;
		}
		else
		{
			newState &= ~LEFT;
		}
	
		if (ev.RightIsDown())
		{
			newState |= RIGHT;
		}
		else
		{
			newState &= ~RIGHT;
		}

		if (ev.MiddleIsDown())
		{
			newState |= MIDDLE;
		}
		else
		{
			newState &= ~MIDDLE;
		}

#if wxCHECK_VERSION(3,0,0)
		if (ev.Aux1IsDown())
		{
			newState |= AUX1;
		}
		else
		{
			newState &= ~AUX1;
		}

		if (ev.Aux2IsDown())
		{
			newState |= AUX2;
		}
		else
		{
			newState &= ~AUX2;
		}
#endif
        newState |= Modifier::GetStateForMouseEvent(ev);

		return newState;
	}

    // Parses the node's attributes to the corresponding flag
    static unsigned int LoadFromNode(const xml::Node& node)
    {
        return GetStateFromString(node.getAttributeValue(ATTR_BUTTON));
    }

    // Converts "LMB" to the corresponding flag
    static unsigned int GetStateFromString(const std::string& str)
    {
        if (str == BUTTONSTR_LMB) return LEFT;
        if (str == BUTTONSTR_RMB) return RIGHT;
        if (str == BUTTONSTR_MMB) return MIDDLE;
        if (str == BUTTONSTR_AUX1) return AUX1;
        if (str == BUTTONSTR_AUX2) return AUX2;

        return NONE;
    }

    static std::string GetButtonString(unsigned int state)
    {
        if ((state & LEFT) != 0) return BUTTONSTR_LMB;
        if ((state & RIGHT) != 0) return BUTTONSTR_RMB;
        if ((state & MIDDLE) != 0) return BUTTONSTR_MMB;
        if ((state & AUX1) != 0) return BUTTONSTR_AUX1;
        if ((state & AUX2) != 0) return BUTTONSTR_AUX2;

        return "";
    }

    // Saves the button flags to the given node
    static void SaveToNode(unsigned int state, xml::Node& node)
    {
        node.setAttributeValue(ATTR_BUTTON, GetButtonString(state));
    }

    static void ForeachButton(const std::function<void(unsigned int)>& func)
    {
        func(LEFT);
        func(MIDDLE);
        func(RIGHT);
        func(AUX1);
        func(AUX2);
    }
};

} // namespace
