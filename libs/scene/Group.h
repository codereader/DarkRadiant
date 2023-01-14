#pragma once

#include "i18n.h"
#include "imap.h"
#include "iundo.h"
#include "iselection.h"
#include "iselectiongroup.h"
#include "iscenegraph.h"

#include "selectionlib.h"
#include "SelectableNode.h"
#include "command/ExecutionNotPossible.h"

namespace selection
{

namespace detail
{
	inline ISelectionGroupManager& getMapSelectionGroupManager()
	{
		assert(GlobalMapModule().getRoot());

		return GlobalMapModule().getRoot()->getSelectionGroupManager();
	}
}

/**
 * Returns if the groupSelected command is able to execute
 * at this point, otherwise throws cmd::ExecutionNotPossible
 */
inline void checkGroupSelectedAvailable()
{
	if (!GlobalMapModule().getRoot())
	{
		throw cmd::ExecutionNotPossible(_("No map loaded"));
	}

	if (GlobalSelectionSystem().getSelectionMode() != SelectionMode::Primitive &&
		GlobalSelectionSystem().getSelectionMode() != SelectionMode::GroupPart)
	{
		throw cmd::ExecutionNotPossible(_("Groups can be formed in Primitive and Group Part selection mode only"));
	}

	if (GlobalSelectionSystem().getSelectionInfo().totalCount == 0)
	{
		throw cmd::ExecutionNotPossible(_("Nothing selected, cannot group anything"));
	}

	if (GlobalSelectionSystem().getSelectionInfo().totalCount == 1)
	{
		throw cmd::ExecutionNotPossible(_("Select more than one element to form a group"));
		return;
	}

	// Check if the current selection already is member of the same group
	std::set<std::size_t> groupIds;
	bool hasUngroupedNode = false;

	GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
	{
		std::shared_ptr<IGroupSelectable> selectable = std::dynamic_pointer_cast<IGroupSelectable>(node);

		if (!selectable) return;

		if (!selectable->getGroupIds().empty())
		{
			groupIds.insert(selectable->getMostRecentGroupId());
		}
		else
		{
			hasUngroupedNode = true;
		}
	});

	if (!hasUngroupedNode && groupIds.size() == 1)
	{
		throw cmd::ExecutionNotPossible(_("The selected elements already form a group"));
	}
}

/**
 * Groups the currently selected elements.
 * Will throw cmd::ExecutionNotPossible if it cannot execute.
 */
inline void groupSelected()
{
	// This will throw exceptions
	checkGroupSelectedAvailable();

	UndoableCommand cmd("GroupSelected");

	ISelectionGroupPtr group = detail::getMapSelectionGroupManager().createSelectionGroup();

	GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
	{
		group->addNode(node);
	});

	SceneChangeNotify();
}

/**
 * Returns if the ungroupSelected command is able to execute
 * at this point, otherwise throws cmd::ExecutionNotPossible.
 */
inline void checkUngroupSelectedAvailable()
{
	if (!GlobalMapModule().getRoot())
	{
		throw cmd::ExecutionNotPossible(_("No map loaded"));
	}

	if (GlobalSelectionSystem().getSelectionMode() != SelectionMode::Primitive &&
		GlobalSelectionSystem().getSelectionMode() != SelectionMode::GroupPart)
	{
		throw cmd::ExecutionNotPossible(_("Groups can be dissolved in Primitive and Group Part selection mode only"));
	}

	if (GlobalSelectionSystem().getSelectionInfo().totalCount == 0)
	{
		throw cmd::ExecutionNotPossible(_("Nothing selected, cannot un-group anything"));
	}

	// Check if the current selection already is member of the same group
	bool hasOnlyUngroupedNodes = true;

	GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
	{
		std::shared_ptr<IGroupSelectable> selectable = std::dynamic_pointer_cast<IGroupSelectable>(node);

		if (!selectable) return;

		if (!selectable->getGroupIds().empty())
		{
			hasOnlyUngroupedNodes = false;
		}
	});

	if (hasOnlyUngroupedNodes)
	{
		throw cmd::ExecutionNotPossible(_("The selected elements aren't part of any group"));
	}
}

/**
 * Resolve the currently selected group.
 * Will throw cmd::ExecutionNotPossible if it cannot execute.
 */
inline void ungroupSelected()
{
	// Will throw exceptions if not available
	checkUngroupSelectedAvailable();

	UndoableCommand cmd("UngroupSelected");

	// Collect all the latest group Ids from all selected nodes
	std::set<std::size_t> ids;

	GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
	{
		std::shared_ptr<scene::SelectableNode> selectable = std::dynamic_pointer_cast<scene::SelectableNode>(node);

		if (!selectable) return;

		if (selectable->isGroupMember())
		{
			ids.insert(selectable->getMostRecentGroupId());
		}
	});

	auto& selGroupMgr = detail::getMapSelectionGroupManager();

	// Now remove the found group by ID (maybe convert them to a selection set before removal?)
	std::for_each(ids.begin(), ids.end(), [&](std::size_t id)
	{
		selGroupMgr.deleteSelectionGroup(id);
	});

	SceneChangeNotify();
}

}
