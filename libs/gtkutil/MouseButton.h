#pragma once

#include <wx/event.h>

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
};

} // namespace
