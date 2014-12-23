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
        unsigned int state = NONE;

        std::string buttonStr = node.getAttributeValue(ATTR_BUTTON);

        if (buttonStr == BUTTONSTR_LMB) state |= LEFT;
        if (buttonStr == BUTTONSTR_RMB) state |= RIGHT;
        if (buttonStr == BUTTONSTR_MMB) state |= MIDDLE;
        if (buttonStr == BUTTONSTR_AUX1) state |= AUX1;
        if (buttonStr == BUTTONSTR_AUX2) state |= AUX2;

        return state;
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
