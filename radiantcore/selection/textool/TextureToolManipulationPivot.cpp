#include "TextureToolManipulationPivot.h"

#include "math/AABB.h"
#include "itexturetoolmodel.h"

namespace textool
{

void TextureToolManipulationPivot::updateFromSelection()
{
    _needsRecalculation = false;
    _userLocked = false;

    // Check the centerpoint of all selected items
    AABB bounds;

    if (GlobalTextureToolSelectionSystem().getSelectionMode() == SelectionMode::Surface)
    {
        GlobalTextureToolSelectionSystem().foreachSelectedNode([&](const INode::Ptr& node)
        {
            bounds.includeAABB(node->localAABB());
            return true;
        });
    }
    else
    {
        GlobalTextureToolSelectionSystem().foreachSelectedComponentNode([&](const INode::Ptr& node)
        {
            auto componentSelectable = std::dynamic_pointer_cast<IComponentSelectable>(node);

            if (componentSelectable)
            {
                bounds.includeAABB(componentSelectable->getSelectedComponentBounds());
            }

            return true;
        });
    }

    if (bounds.isValid())
    {
        setFromMatrix(Matrix4::getTranslation(Vector3(bounds.origin.x(), bounds.origin.y(), 0)));
    }
    else
    {
        setFromMatrix(Matrix4::getIdentity());
    }
}

}
