#pragma once

#include <memory>
#include "inode.h"

/**
 * greebo: A Selectable is everything that can be highlighted
 * by the user in the scene (e.g. by interaction with the mouse).
 */
class ISelectable
{
public:
	virtual ~ISelectable() {}

	// Set the selection status of this object
	virtual void setSelected(bool select) = 0;

	// Check the selection status of this object (TRUE == selected)
	virtual bool isSelected() const = 0;
};
typedef std::shared_ptr<ISelectable> ISelectablePtr;

namespace scene
{
	class INode;
	typedef std::shared_ptr<INode> INodePtr;
}

inline void Node_setSelected(const scene::INodePtr& node, bool selected)
{
	auto selectable = scene::node_cast<ISelectable>(node);

    if (selectable)
	{
        selectable->setSelected(selected);
    }
}

inline bool Node_isSelected(const scene::INodePtr& node)
{
	auto selectable = scene::node_cast<ISelectable>(node);

    if (selectable)
	{
        return selectable->isSelected();
    }

    return false;
}
