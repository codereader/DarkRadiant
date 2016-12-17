#pragma once

#include "iselection.h"
#include "render/Colour4b.h"

namespace selection
{

/**
 * A Manipulator is a renderable object which contains one or more
 * ManipulatorComponents, each of which can be manipulated by the user. For
 * example, the rotation Manipulator draws several circles which cause rotations
 * around specific axes.
 */
class ManipulatorBase :
	public Manipulator
{
public:
	virtual ~ManipulatorBase() {}

	// By default, manipulators can operate on components too (Clipper can't)
	virtual bool supportsComponentManipulation() const override
	{
		return true;
	}

public:
    /* Static colours */
    static const Colour4b& COLOUR_SCREEN();
    static const Colour4b& COLOUR_SPHERE();
    static const Colour4b& COLOUR_SELECTED();
};

}

const Colour4b& colourSelected(const Colour4b& colour, bool selected);
