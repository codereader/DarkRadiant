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

	SelectableNode(const SelectableNode& other) :
		scene::Node(other),
		_selected(false)
	{}

    virtual ~SelectableNode()
	{
		setSelected(false);
	}

	virtual void onInsertIntoScene(IMapRootNode& root) override
	{
		Node::onInsertIntoScene(root);

		// If the group ID set is not empty, this node was likely removed while
		// it was still member of one or more groups.
		// Try to add ourselves to any groups we were assigned to, if the group
		// is not there anymore, we don't do anything.
		for (std::size_t id : _groups)
		{
			selection::ISelectionGroupPtr group = GlobalSelectionGroupManager().getSelectionGroup(id);
			
			if (group)
			{
				group->addNode(getSelf());
			}
		}
	}

    // override scene::Inode::onRemoveFromScene to de-select self
    virtual void onRemoveFromScene(IMapRootNode& root) override
	{
		setSelected(false);

		// When a node is removed from the scene with a non-empty group assignment
		// we do notify the SelectionGroup to remove ourselves, but we keep the ID list
		// That way we can re-add ourselves when being inserted into the scene again

		if (!_groups.empty())
		{
			// Copy the group IDs, as calling removeNode() will alter the group ID list
			GroupIds copy(_groups);

			// Remove ourselves from all groups
			while (!_groups.empty())
			{
				std::size_t id = _groups.front();

				selection::ISelectionGroupPtr group = GlobalSelectionGroupManager().getSelectionGroup(id);

				if (group)
				{
					group->removeNode(getSelf());
				}
			}

			// Now copy the values back in for later use
			_groups.swap(copy);
		}

		Node::onRemoveFromScene(root);
	}

	/**
	* \brief
	* Set the selection state.
	*/
	virtual void setSelected(bool select) override
	{
		// Set selection status and notify group members if applicable
		setSelected(select, false);
	}

	virtual void addToGroup(std::size_t groupId) override
	{
		if (std::find(_groups.begin(), _groups.end(), groupId) == _groups.end())
		{
			_groups.push_back(groupId);
		}
	}

	virtual void removeFromGroup(std::size_t groupId) override
	{
		std::vector<std::size_t>::iterator i = std::find(_groups.begin(), _groups.end(), groupId);
		
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
		bool selected = isSelected();

		// Update the flag to render selected nodes regardless of their hidden status 
		setForcedVisibility(selected, true);

		GlobalSelectionSystem().onSelectedChanged(Node::getSelf(), *this);

		// Check if this node is member of a group
		if (changeGroupStatus && !_groups.empty())
		{
			std::size_t mostRecentGroupId = _groups.back();

			// Propagate the selection status of this node to all members of the topmost group
			GlobalSelectionGroupManager().setGroupSelected(mostRecentGroupId, selected);
		}
	}
};

} // namespace
