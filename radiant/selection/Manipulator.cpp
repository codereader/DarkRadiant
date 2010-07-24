#include "Manipulator.h"

const Colour4b& Manipulator::COLOUR_SPHERE()
{
    static Colour4b _colour(0, 0, 0, 255);
    return _colour;
}

const Colour4b& Manipulator::COLOUR_SCREEN()
{
    static Colour4b _colour(0, 255, 255, 255);
    return _colour;
}

const Colour4b& Manipulator::COLOUR_SELECTED()
{
    static Colour4b _colour(255, 255, 0, 255);
    return _colour;
}

const Colour4b& colourSelected(const Colour4b& colour, bool selected)
{
  return (selected) ? Manipulator::COLOUR_SELECTED() : colour;
}

