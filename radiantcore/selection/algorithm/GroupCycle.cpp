#include "GroupCycle.h"

#include "iselectable.h"
#include "iselectiongroup.h"
#include "imap.h"
#include "selectionlib.h"
#include "iscenegraph.h"

#include <algorithm>

namespace selection {

namespace algorithm {

	/** greebo: This helper visitor populates the given targetList with
	 *          all found child selectables under the given node.
	 */
	class ChildNodeFinder :
		public scene::NodeVisitor
	{
		NodeVector& _targetList;
	public:
		ChildNodeFinder(NodeVector& targetList) :
			_targetList(targetList)
		{}

		bool pre(const scene::INodePtr& node) {
			ISelectablePtr selectable = scene::node_cast<ISelectable>(node);

			// If a visible selectable was found and the path depth is appropriate, add it
			if (selectable != NULL && node->visible()) {
				_targetList.push_back(node);
			}

			return true;
		}
	};

} // namespace algorithm

GroupCycle::GroupCycle() :
	_index(0),
	_updateActive(false)
{
	GlobalSelectionSystem().addObserver(this);
	rescanSelection();
}

void GroupCycle::selectionChanged(const scene::INodePtr& node, bool isComponent) {
	// greebo: Only rescan the selection for non-component changes, otherwise the list
	// will get cleared as soon as the face dragresize manipulator gets active
	if (!isComponent) {
		rescanSelection();
	}
}

void GroupCycle::rescanSelection() {
	if (_updateActive) {
		return;
	}

	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	_list.clear();
	_index = 0;

	if (info.totalCount == 1 && info.entityCount == 1) {
		const scene::INodePtr& node = GlobalSelectionSystem().ultimateSelected();

		algorithm::ChildNodeFinder finder(_list);
		node->traverse(finder);
	}
	else if (info.totalCount > 1)
	{
		// Check if all selected nodes share a common selection group
		std::size_t commonGroupId = 0;
		bool hasCommonGroup = true;
		bool first = true;

		GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node) {
			if (!hasCommonGroup) return;

			auto groupSelectable = std::dynamic_pointer_cast<IGroupSelectable>(node);
			if (!groupSelectable || !groupSelectable->isGroupMember()) {
				hasCommonGroup = false;
				return;
			}

			if (first) {
				commonGroupId = groupSelectable->getMostRecentGroupId();
				first = false;
			} else {
				const auto& ids = groupSelectable->getGroupIds();
				if (std::find(ids.begin(), ids.end(), commonGroupId) == ids.end()) {
					hasCommonGroup = false;
				}
			}
		});

		if (hasCommonGroup && !first) {
			auto root = GlobalMapModule().getRoot();
			if (root) {
				auto group = root->getSelectionGroupManager().getSelectionGroup(commonGroupId);
				if (group) {
					group->foreachNode([&](const scene::INodePtr& node) {
						if (node->visible()) {
							_list.push_back(node);
						}
					});
				}
			}
		}
	}
}

void GroupCycle::updateSelection() {
	_updateActive = true;

	// Do some sanity checking before we run into crashes
	if (_index >= 0 && _index < static_cast<int>(_list.size())) {
		for (std::size_t i = 0; i < _list.size(); i++) {
			Node_setSelected(_list[i], false);
		}

		Node_setSelected(_list[_index], true);
	}

	SceneChangeNotify();

	_updateActive = false;
}

void GroupCycle::doCycleForward() {
	if (_list.size() > 1) {
		// Increase the index and wrap around at the list end
		_index = (_index+1) % static_cast<int>(_list.size());

		// Select the new candidate
		updateSelection();
	}
}

void GroupCycle::doCycleBackward() {
	if (_list.size() > 1) {
		// Decrease the index and wrap around, if necessary
		_index--;

		if (_index < 0) {
			_index += static_cast<int>(_list.size());
		}

		// Select the new candidate
		updateSelection();
	}
}

void GroupCycle::cycleBackward(const cmd::ArgumentList& args) {
	Instance().doCycleBackward();
}

void GroupCycle::cycleForward(const cmd::ArgumentList& args) {
	Instance().doCycleForward();
}

GroupCycle& GroupCycle::Instance() {
	static GroupCycle _instance;

	return _instance;
}

} // namespace selection
