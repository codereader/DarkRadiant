#include "GroupCycle.h"

#include "selectionlib.h"

namespace selection {

namespace algorithm {
			
	class ChildInstanceFinder : 
		public scene::Graph::Walker
	{
		InstanceVector& _targetList;
		scene::Path& _parentPath;
		unsigned int _minPathDepth;
	public:
		/** greebo: This populates the given targetList with
		 * all the child selectable Instances found under the parentPath.
		 * 
		 * Specify minPathDepth > 1 to choose the children only,
		 * set minPathDepth to 1 to allow choosing the parent as well.
		 */
		ChildInstanceFinder(InstanceVector& targetList, 
							scene::Path& parentPath,
							unsigned int minPathDepth = 1) : 
			_targetList(targetList),
			_parentPath(parentPath),
			_minPathDepth(minPathDepth)
		{}
		
		bool pre(const scene::Path& path, scene::Instance& instance) const {
			
			Selectable* selectable = Instance_getSelectable(instance);
			
			// If a selectable was found and it's not the parent itself, add it to the list
			if (selectable != NULL && 
				!(path == _parentPath) && 
				path.size() >= _minPathDepth) 
			{
				_targetList.push_back(&instance);
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

void GroupCycle::selectionChanged(scene::Instance& instance) {
	rescanSelection();		
}

void GroupCycle::rescanSelection() {
	if (_updateActive) {
		return;
	}
	
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	_list.clear();
	_index = 0;
	
	if (info.totalCount == 1 && info.entityCount == 1) {
		scene::Instance& instance = GlobalSelectionSystem().ultimateSelected();
		
		scene::Path startingPath = instance.path();
		algorithm::ChildInstanceFinder walker(_list, startingPath);
		
		GlobalSceneGraph().traverse_subgraph(walker, startingPath);
	}
}

void GroupCycle::updateSelection() {
	_updateActive = true;
	
	// Do some sanity checking before we run into crashes
	if (_index >= 0 && _index < static_cast<int>(_list.size())) {
		for (unsigned int i = 0; i < _list.size(); i++) {
			Selectable* selectable = Instance_getSelectable(*_list[i]);
			selectable->setSelected(false);
		}
		
		Selectable* selectable = Instance_getSelectable(*_list[_index]);
		selectable->setSelected(true);
	}
	
	SceneChangeNotify();
	
	_updateActive = false;
}

void GroupCycle::doCycleForward() {
	if (_list.size() > 0) {
		// Increase the index and wrap around at the list end
		_index = (_index+1) % _list.size();
		
		// Select the new candidate
		updateSelection();
	}
}

void GroupCycle::doCycleBackward() {
	if (_list.size() > 0) {
		// Decrease the index and wrap around, if necessary 
		_index--;
	
		if (_index < 0) {
			_index += _list.size();
		}
	
		// Select the new candidate
		updateSelection();
	}
}

void GroupCycle::cycleBackward() {
	Instance().doCycleBackward();
}

void GroupCycle::cycleForward() {
	Instance().doCycleForward();
}

GroupCycle& GroupCycle::Instance() {
	static GroupCycle _instance;
	
	return _instance;
}
	
} // namespace selection
