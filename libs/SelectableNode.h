#pragma once

#include "scene/Node.h"
#include "iselectiongroup.h"
#include "iselection.h"
#include <stdexcept>

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
	GroupIds _groups;

public:
	SelectableNode() :
		_selected(false)
	{}

	// The copy-constructor doesn't copy the signal, re-connect to this instance instead
	SelectableNode(const SelectableNode& other) :
		scene::Node(other),
		_selected(false)
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
		// Set selection status and notify group members if applicable
		setSelected(select, true);
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

	virtual bool isGroupMember() override
	{
		return !_groups.empty();
	}

	virtual std::size_t getMostRecentGroupId() override
	{
		if (_groups.empty()) throw std::runtime_error("This node is not a member of any group.");

		return _groups.back();
	}

	virtual const GroupIds& getGroupIds() override
	{
		return _groups;
	}

	virtual void setSelected(bool select, bool changeGroupStatus) override
	{
		// Change state and invoke callback only if the new state is different
		// from the current state
		if (select ^ _selected)
		{
			_selected = select;

			onSelectionStatusChange(changeGroupStatus);
		}
	}

	virtual bool isSelected() const override
	{
		return _selected;
	}

protected:
	/**
     * \brief
     * Invoked when the selection status changes.
     */
	virtual void onSelectionStatusChange(bool changeGroupStatus)
    {
		GlobalSelectionSystem().onSelectedChanged(Node::getSelf(), *this);

		// Check if this node is member of a group
		if (changeGroupStatus && !_groups.empty())
		{
			std::size_t mostRecentGroupId = _groups.back();

			// Propagate the selection status of this node to all members of the topmost group
			GlobalSelectionGroupManager().setGroupSelected(mostRecentGroupId, isSelected());
		}
	}
};

} // namespace
