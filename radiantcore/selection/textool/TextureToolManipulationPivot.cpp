#include "TextureToolManipulationPivot.h"

#include "itexturetoolmodel.h"

namespace textool
{

void TextureToolManipulationPivot::updateFromSelection()
{
    _needsRecalculation = false;
    _userLocked = false;

    // Check the centerpoint of all selected items
    Vector2 sum(0, 0);
    std::size_t count = 0;

    GlobalTextureToolSelectionSystem().foreachSelectedNode([&](const INode::Ptr& node)
    {
        auto bounds = node->localAABB();
        sum += Vector2(bounds.origin.x(), bounds.origin.y());
        count++;

        return true;
    });

    if (count > 0)
    {
        sum /= count;
        setFromMatrix(Matrix4::getTranslation(Vector3(sum.x(), sum.y(), 0)));
    }
    else
    {
        setFromMatrix(Matrix4::getIdentity());
    }
}

}
