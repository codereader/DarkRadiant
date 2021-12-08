#include "RenderableEntityBox.h"

#include "EntityNode.h"

namespace entity
{

RenderableEntityBox::RenderableEntityBox(EntityNode& node) :
    _needsUpdate(true),
    _filledBox(true)
{}

void RenderableEntityBox::queueUpdate()
{
    _needsUpdate = true;
}

void RenderableEntityBox::setFillMode(bool fill)
{
    _filledBox = fill;
}

void RenderableEntityBox::updateGeometry()
{
    if (!_needsUpdate) return;

    _needsUpdate = false;


}

}
