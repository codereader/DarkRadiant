#pragma once

#include "iselection.h"
#include "scenelib.h"

namespace scene
{

// Checks the current selection to see whether it consists of
// group nodes only.
class GroupNodeChecker :
	public SelectionSystem::Visitor
{
private:
	mutable bool _onlyGroups;
	mutable std::size_t _numGroups;
	mutable scene::INodePtr _firstGroupNode;

public:
	GroupNodeChecker() :
		_onlyGroups(true),
		_numGroups(0)
	{}

	void visit(const scene::INodePtr& node) const
	{
		if (!scene::hasChildPrimitives(node))
		{
			_onlyGroups = false;
		}
		else
		{
			_numGroups++;

			if (!_firstGroupNode)
			{
				_firstGroupNode = node;
			}
		}
	}

	// Returns true if the current selection consists of group nodes only
	// Returns false if any selected node is a non-group or if nothing is selected.
	bool onlyGroupsAreSelected() const
	{
		return _numGroups > 0 && _onlyGroups;
	}

	// Returns the number of group nodes in the current selection
	std::size_t selectedGroupCount() const
	{
		return _numGroups;
	}

	// Returns the first group node of the selection or NULL if nothing selected
	scene::INodePtr getFirstSelectedGroupNode() const
	{
		return _firstGroupNode;
	}
};

}
