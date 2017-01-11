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
private:
	std::size_t _id;
public:
	ManipulatorBase() :
		_id(0)
	{}

	virtual ~ManipulatorBase() {}

	// By default, manipulators can operate on components too (Clipper can't)
	virtual bool supportsComponentManipulation() const override
	{
		return true;
	}

	// No visual representation by default
	virtual void render(RenderableCollector& collector, const VolumeTest& volume) override
	{}

public:
	std::size_t getId() const override
	{
		return _id;
	}

	void setId(std::size_t id) override
	{
		_id = id;
	}

	/* Static colours */
	static const Colour4b& COLOUR_X();
	static const Colour4b& COLOUR_Y();
	static const Colour4b& COLOUR_Z();

    static const Colour4b& COLOUR_SCREEN();
    static const Colour4b& COLOUR_SPHERE();
    static const Colour4b& COLOUR_SELECTED();
};

}

const Colour4b& colourSelected(const Colour4b& colour, bool selected);
