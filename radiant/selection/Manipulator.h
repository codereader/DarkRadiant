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
class Manipulator
{
public:
	// Manipulator type enum, user-defined manipulators should return "Custom"
	enum Type
	{
		Drag,
		Translate,
		Rotate,
		Scale,
		Clip,
		Custom
	};


    virtual ~Manipulator() {}

	virtual Type getType() const = 0;

    /**
     * Get the currently-active ManipulatorComponent. This is determined by the
     * most recent selection test.
     */
    virtual ManipulatorComponent* getActiveComponent() = 0;

    virtual void testSelect(const render::View& view, const Matrix4& pivot2world) {}

    // This function is responsible for bringing the visual representation
    // of this manipulator onto the screen
    virtual void render(RenderableCollector& collector,
                        const VolumeTest& volume,
                        const Matrix4& pivot2world) {}

    virtual void setSelected(bool select) = 0;
    virtual bool isSelected() const = 0;

public:

    /* Static colours */
    static const Colour4b& COLOUR_SCREEN();
    static const Colour4b& COLOUR_SPHERE();
    static const Colour4b& COLOUR_SELECTED();
};
typedef std::shared_ptr<Manipulator> ManipulatorPtr;

}

// ------------ Helper functions ---------------------------

const Colour4b& colourSelected(const Colour4b& colour, bool selected);



