#include "ManipulatorBase.h"

namespace selection
{

const Colour4b& ManipulatorBase::COLOUR_X()
{
	static Colour4b _colour(255, 0, 0, 255);
	return _colour;
}

const Colour4b& ManipulatorBase::COLOUR_Y()
{
	static Colour4b _colour(0, 255, 0, 255);
	return _colour;
}

const Colour4b& ManipulatorBase::COLOUR_Z()
{
	static Colour4b _colour(0, 0, 255, 255);
	return _colour;
}

const Colour4b& ManipulatorBase::COLOUR_SPHERE()
{
    static Colour4b _colour(0, 0, 0, 255);
    return _colour;
}

const Colour4b& ManipulatorBase::COLOUR_SCREEN()
{
    static Colour4b _colour(0, 255, 255, 255);
    return _colour;
}

const Colour4b& ManipulatorBase::COLOUR_SELECTED()
{
    static Colour4b _colour(255, 255, 0, 255);
    return _colour;
}

} // namespace

const Colour4b& colourSelected(const Colour4b& colour, bool selected)
{
  return (selected) ? selection::ManipulatorBase::COLOUR_SELECTED() : colour;
}

