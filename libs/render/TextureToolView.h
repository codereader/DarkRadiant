#pragma once

#include "View.h"
#include "math/AABB.h"
#include "math/Matrix4.h"

namespace render
{

class TextureToolView :
    public View
{
public:
    /**
     * Construct the orthographic projection matrix such that the given
     * texture space bounds are visible.
     * 
     * Only the XY coordinates of the bounds are considered, Z is ignored.
     */
    void constructFromTextureSpaceBounds(const AABB& texSpaceAABB, std::size_t deviceWidth, std::size_t deviceHeight)
    {
        // Set up the orthographic projection matrix to [b,l..t,r] => [-1,-1..+1,+1]
        // with b,l,t,r set to values centered at origin
        double left = -texSpaceAABB.extents.x();
        double right = texSpaceAABB.extents.x();

        double top = texSpaceAABB.extents.y();
        double bottom = -texSpaceAABB.extents.y();

        auto rMinusL = right - left;
        auto rPlusL = right + left;

        auto tMinusB = top - bottom;
        auto tPlusB = top + bottom;

        auto projection = Matrix4::getIdentity();

        projection[0] = 2.0f / rMinusL;
        projection[5] = 2.0f / tMinusB;
        projection[10] = -1;

        projection[12] = rPlusL / rMinusL;
        projection[13] = tPlusB / tMinusB;
        projection[14] = 0;

        auto modelView = Matrix4::getIdentity();

        // We have to invert the Y axis to have the negative tex coords in the upper quadrants
        modelView.xx() = 1;
        modelView.yy() = -1;

        // Shift the visible UV space such that it is centered around origin before projecting it
        modelView.tx() = -texSpaceAABB.origin.x();
        modelView.ty() = texSpaceAABB.origin.y();

        construct(projection, modelView, deviceWidth, deviceHeight);
    }
};

}
