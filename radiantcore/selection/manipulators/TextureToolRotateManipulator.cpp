#include "TextureToolRotateManipulator.h"

namespace selection
{

std::size_t TextureToolRotateManipulator::getId() const
{
    return _id;
}

void TextureToolRotateManipulator::setId(std::size_t id)
{
    _id = id;
}

IManipulator::Type TextureToolRotateManipulator::getType() const
{
    return IManipulator::Rotate;
}

IManipulator::Component* TextureToolRotateManipulator::getActiveComponent()
{
    return nullptr;
}

void TextureToolRotateManipulator::setSelected(bool select)
{
    _isSelected = select;
}

bool TextureToolRotateManipulator::isSelected() const
{
    return _isSelected;
}

void TextureToolRotateManipulator::testSelect(SelectionTest& test, const Matrix4& pivot2world)
{
    // TODO
}

}
