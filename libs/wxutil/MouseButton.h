#pragma once

#include <wx/event.h>
#include <vector>
#include "xmlutil/Node.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace wxutil
{

class MouseButton
{
public:
	enum ButtonFlags
	{
		NONE	= 0,
		LEFT	= 1 << 1,
		RIGHT	= 1 << 2,
		MIDDLE	= 1 << 3,
		AUX1	= 1 << 4,
		AUX2	= 1 << 5,
		SHIFT	= 1 << 6,
		CONTROL	= 1 << 7,
		ALT		= 1 << 8
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
		if (ev.ShiftDown())
		{
			newState |= SHIFT;
		}
		else
		{
			newState &= ~SHIFT;
		}

		if (ev.ControlDown())
		{
			newState |= CONTROL;
		}
		else
		{
			newState &= ~CONTROL;
		}

		if (ev.AltDown())
		{
			newState |= ALT;
		}
		else
		{
			newState &= ~ALT;
		}

		return newState;
	}

    // Parses the node's attributes to the corresponding flag
    static unsigned int LoadFromNode(const xml::Node& node)
    {
        unsigned int state = NONE;

        std::string buttonStr = node.getAttributeValue("button");

        if (buttonStr == "LMB") state |= LEFT;
        if (buttonStr == "RMB") state |= RIGHT;
        if (buttonStr == "MMB") state |= MIDDLE;
        if (buttonStr == "AUX1") state |= AUX1;
        if (buttonStr == "AUX2") state |= AUX2;

        std::string modifierStr = node.getAttributeValue("modifiers");

        std::vector<std::string> parts;
        boost::algorithm::split(parts, modifierStr, boost::algorithm::is_any_of("+"));

        for (const std::string& mod : parts)
        {
            if (mod == "SHIFT") { state |= SHIFT; continue; }
            if (mod == "ALT") { state |= ALT; continue; }
            if (mod == "CONTROL") { state |= CONTROL; continue; }
        }

        return state;
    }

    // Saves the button flags to the given node
    static void SaveToNode(unsigned int state, xml::Node& node)
    {
        if ((state & LEFT) != 0) node.setAttributeValue("button", "LMB");
        if ((state & RIGHT) != 0) node.setAttributeValue("button", "RMB");
        if ((state & MIDDLE) != 0) node.setAttributeValue("button", "MMB");
        if ((state & AUX1) != 0) node.setAttributeValue("button", "AUX1");
        if ((state & AUX2) != 0) node.setAttributeValue("button", "AUX2");

        std::string mod = "";

        if ((state & ALT) != 0) mod += mod.empty() ? "ALT" : "+ALT";
        if ((state & CONTROL) != 0) mod += mod.empty() ? "CONTROL" : "+CONTROL";
        if ((state & SHIFT) != 0) mod += mod.empty() ? "SHIFT" : "+SHIFT";

        node.setAttributeValue("modfiier", mod);
    }
};

} // namespace
