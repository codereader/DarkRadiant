#pragma once

#include "ManipulatorComponent.h"

#include "irenderable.h"

#include "render/View.h"
#include "math/Matrix4.h"
#include "render.h"
#include <memory>

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

public:
    /* Static colours */
    static const Colour4b& COLOUR_SCREEN();
    static const Colour4b& COLOUR_SPHERE();
    static const Colour4b& COLOUR_SELECTED();
};

}

// ------------ Helper functions ---------------------------

const Colour4b& colourSelected(const Colour4b& colour, bool selected);



