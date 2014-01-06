#pragma once

#include "scene/Node.h"
#include "selectionlib.h"
#include <boost/bind.hpp>

namespace scene
{

/**
 * \brief
 * Subclass of scene::Node which implements the Selectable interface.
 *
 * The GlobalSelectionSystem will be notified of selection changes.
 */
class SelectableNode :
	public scene::Node,
	public ObservedSelectable
{
public:
	SelectableNode() :
		ObservedSelectable(boost::bind(&SelectableNode::selectedChanged, this, _1))
	{}

	SelectableNode(const SelectableNode& other) :
		scene::Node(other),
		ObservedSelectable(boost::bind(&SelectableNode::selectedChanged, this, _1))
	{}

    /**
     * \brief
     * Callback invoked by the ObservedSelectable when the selection changes.
     */
	void selectedChanged(const Selectable& selectable)
    {
		GlobalSelectionSystem().onSelectedChanged(
            shared_from_this(), selectable
        );
	}

	// override scene::Inode::onRemoveFromScene to de-select self
	virtual void onRemoveFromScene()
	{
		setSelected(false);

		Node::onRemoveFromScene();
	}
};

} // namespace
