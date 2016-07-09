#pragma once

#include "scene/Node.h"
#include "igroupselectable.h"
#include "iselection.h"

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
	public IGroupSelectable
{
private:
	// Current selection state
	bool _selected;

	// The groups this node is a member of. The last value in the list 
	// represents the group this node has been added to most recently
	std::vector<std::size_t> _groups;

public:
	SelectableNode()
	{}

	// The copy-constructor doesn't copy the signal, re-connect to this instance instead
	SelectableNode(const SelectableNode& other) :
		scene::Node(other)
	{}

    virtual ~SelectableNode()
	{
		setSelected(false);
	}

    // override scene::Inode::onRemoveFromScene to de-select self
    virtual void onRemoveFromScene(IMapRootNode& root) override
	{
		setSelected(false);

		Node::onRemoveFromScene(root);
	}

	/**
	* \brief
	* Set the selection state.
	*/
	virtual void setSelected(bool select) override
	{
		// Change state and invoke callback only if the new state is different
		// from the current state
		if (select ^ _selected)
		{
			_selected = select;

			onSelectionStatusChange();
		}
	}

	virtual void addToGroup(std::size_t groupId) override
	{
		assert(std::find(_groups.begin(), _groups.end(), groupId) == _groups.end());
		_groups.push_back(groupId);
	}

	virtual void removeFromGroup(std::size_t groupId) override
	{
		std::vector<std::size_t>::iterator i = std::find(_groups.begin(), _groups.end(), groupId);
		
		assert(i != _groups.end());

		if (i != _groups.end())
		{
			_groups.erase(i);
		}
	}

	virtual void setSelected(bool select, bool notifyGroup) override
	{
		// For now, just redirect the call
		setSelected(select);
	}

	virtual bool isSelected() const override
	{
		return _selected;
	}

	virtual void invertSelected() override
	{
		setSelected(!isSelected());
	}

protected:
	/**
     * \brief
     * Invoked when the selection status changes.
     */
	virtual void onSelectionStatusChange()
    {
		// Check if this node is member of a group
		if (!_groups.empty())
		{
			std::size_t mostRecentGroupId = _groups.back();


		}
		
		GlobalSelectionSystem().onSelectedChanged(Node::getSelf(), *this);
	}
};

} // namespace
