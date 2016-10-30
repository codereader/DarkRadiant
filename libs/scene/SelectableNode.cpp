#include "SelectableNode.h"

#include "iselection.h"
#include <stdexcept>

namespace scene
{

SelectableNode::SelectableNode() :
	_selected(false)
{}

SelectableNode::SelectableNode(const SelectableNode& other) :
	scene::Node(other),
	_selected(false)
{}

SelectableNode::~SelectableNode()
{
	setSelected(false);
}

void SelectableNode::onInsertIntoScene(IMapRootNode& root)
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

void SelectableNode::onRemoveFromScene(IMapRootNode& root)
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

void SelectableNode::setSelected(bool select)
{
	// Set selection status and notify group members if applicable
	setSelected(select, false);
}

void SelectableNode::addToGroup(std::size_t groupId)
{
	if (std::find(_groups.begin(), _groups.end(), groupId) == _groups.end())
	{
		_groups.push_back(groupId);
	}
}

void SelectableNode::removeFromGroup(std::size_t groupId)
{
	std::vector<std::size_t>::iterator i = std::find(_groups.begin(), _groups.end(), groupId);

	if (i != _groups.end())
	{
		_groups.erase(i);
	}
}

bool SelectableNode::isGroupMember()
{
	return !_groups.empty();
}

std::size_t SelectableNode::getMostRecentGroupId()
{
	if (_groups.empty()) throw std::runtime_error("This node is not a member of any group.");

	return _groups.back();
}

const SelectableNode::GroupIds& SelectableNode::getGroupIds()
{
	return _groups;
}

void SelectableNode::setSelected(bool select, bool changeGroupStatus)
{
	// Change state and invoke callback only if the new state is different
	// from the current state
	if (select ^ _selected)
	{
		_selected = select;

		onSelectionStatusChange(changeGroupStatus);
	}
}

bool SelectableNode::isSelected() const
{
	return _selected;
}

void SelectableNode::onSelectionStatusChange(bool changeGroupStatus)
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

} // namespace
