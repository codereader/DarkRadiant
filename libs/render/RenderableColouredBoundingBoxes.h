#pragma once

#include "RenderableBoundingBoxes.h"

namespace render
{

// Specialised bounding box renderer supporting invididual box colours
class RenderableColouredBoundingBoxes :
    public RenderableBoundingBoxes
{
private:
    const std::vector<Vector4>& _colours;

public:
    RenderableColouredBoundingBoxes(const std::vector<AABB>& aabbs, const std::vector<Vector4>& colours) :
        RenderableBoundingBoxes(aabbs, { 1,1,1,1 }),
        _colours(colours)
    {}

protected:
    virtual Vector4 getBoxColour(std::size_t boxIndex) override
    {
        return _colours.size() > boxIndex ? _colours[boxIndex] : Vector4(1, 1, 1, 1);
    }
};

}
