#include "SelectableNode.h"

#include "iselection.h"
#include "BasicUndoMemento.h"
#include "imap.h"
#include <stdexcept>
#include <algorithm>
#include <iterator>

namespace scene
{

SelectableNode::SelectableNode() :
	_selected(false),
	_undoStateSaver(nullptr)
{}

SelectableNode::SelectableNode(const SelectableNode& other) :
	scene::Node(other),
	_selected(false),
	_undoStateSaver(nullptr)
{}

SelectableNode::~SelectableNode()
{
	setSelected(false);
}

void SelectableNode::onInsertIntoScene(IMapRootNode& root)
{
	connectUndoSystem(root.getUndoChangeTracker());

	Node::onInsertIntoScene(root);

	// If the group ID set is not empty, this node was likely removed while
	// it was still member of one or more groups.
	// Try to add ourselves to any groups we were assigned to, if the group
	// is not there anymore, it will be created.
	for (std::size_t id : _groups)
	{
		auto group = root.getSelectionGroupManager().findOrCreateSelectionGroup(id);

		if (group)
		{
			group->addNode(getSelf());
		}
	}
}

void SelectableNode::onRemoveFromScene(IMapRootNode& root)
{
	setSelected(false);

	disconnectUndoSystem(root.getUndoChangeTracker());

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

			auto group = root.getSelectionGroupManager().getSelectionGroup(id);

			if (group)
			{
				group->removeNode(getSelf());
			}
			else
			{
				_groups.erase(_groups.begin());
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
		undoSave();

		_groups.push_back(groupId);
	}
}

void SelectableNode::removeFromGroup(std::size_t groupId)
{
	std::vector<std::size_t>::iterator i = std::find(_groups.begin(), _groups.end(), groupId);

	if (i != _groups.end())
	{
		undoSave();

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

IUndoMementoPtr SelectableNode::exportState() const
{
	return IUndoMementoPtr(new undo::BasicUndoMemento<GroupIds>(_groups));
}

void SelectableNode::importState(const IUndoMementoPtr& state)
{
	undoSave();

	// Process the incoming set difference
	GroupIds newGroups = std::static_pointer_cast< undo::BasicUndoMemento<GroupIds> >(state)->data();

	// The set_difference algorithm requires the sets to be sorted
	std::sort(_groups.begin(), _groups.end());
	std::sort(newGroups.begin(), newGroups.end());

	GroupIds removedGroups;
	std::set_difference(_groups.begin(), _groups.end(), newGroups.begin(), newGroups.end(),
		std::inserter(removedGroups, removedGroups.begin()));

	GroupIds addedGroups;
	std::set_difference(newGroups.begin(), newGroups.end(), _groups.begin(), _groups.end(),
		std::inserter(addedGroups, addedGroups.begin()));

	// Remove ourselves from each removed group ID
	for (GroupIds::value_type id : removedGroups)
	{
		selection::ISelectionGroupPtr group = GlobalSelectionGroupManager().getSelectionGroup(id);

		if (group)
		{
			group->removeNode(getSelf());
		}
	}

	// Add ourselves to each missing group ID
	for (GroupIds::value_type id : addedGroups)
	{
		selection::ISelectionGroupPtr group = GlobalSelectionGroupManager().findOrCreateSelectionGroup(id);

		assert(group);
		group->addNode(getSelf());
	}

	// The _groups array should now contain the same elements as the imported ones
#if _DEBUG
	std::sort(_groups.begin(), _groups.end());
	assert(std::equal(_groups.begin(), _groups.end(), newGroups.begin()));
#endif

	// After undoing group assignments, highlight this node for better user feedback
	setSelected(true, false);
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

void SelectableNode::connectUndoSystem(IMapFileChangeTracker& changeTracker)
{
	_undoStateSaver = GlobalUndoSystem().getStateSaver(*this, changeTracker);

	Node::connectUndoSystem(changeTracker);
}

void SelectableNode::disconnectUndoSystem(IMapFileChangeTracker& changeTracker)
{
	_undoStateSaver = nullptr;
	GlobalUndoSystem().releaseStateSaver(*this);

	Node::disconnectUndoSystem(changeTracker);
}

void SelectableNode::undoSave()
{
	if (_undoStateSaver != nullptr)
	{
		_undoStateSaver->save(*this);
	}
}

} // namespace
