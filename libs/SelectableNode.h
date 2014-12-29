#pragma once

#include "scene/Node.h"
#include "ObservedSelectable.h"
#include <functional>

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
	public selection::ObservedSelectable
{
public:
	SelectableNode() :
		ObservedSelectable(std::bind(&SelectableNode::selectedChanged, this, std::placeholders::_1))
	{}

	// The copy-constructor doesn't copy the signal, re-connect to this instance instead
	SelectableNode(const SelectableNode& other) :
		scene::Node(other),
        ObservedSelectable(std::bind(&SelectableNode::selectedChanged, this, std::placeholders::_1))
	{}

    // override scene::Inode::onRemoveFromScene to de-select self
	virtual void onRemoveFromScene()
	{
		setSelected(false);

		Node::onRemoveFromScene();
	}

private:
	/**
     * \brief
     * Callback invoked by the ObservedSelectable when the selection changes.
     */
	void selectedChanged(const Selectable& selectable)
    {
		GlobalSelectionSystem().onSelectedChanged(Node::getSelf(), selectable);
	}
};

} // namespace
