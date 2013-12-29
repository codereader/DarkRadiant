#pragma once

#include "iselectionset.h"
#include "iselection.h"
#include "inode.h"

namespace selection
{

class SelectionSet :
	public ISelectionSet
{
private:
	typedef std::set<scene::INodeWeakPtr> NodeSet;
	NodeSet _nodes;

	std::string _name;

public:
	SelectionSet(const std::string& name);

	const std::string& getName();

	// Checks whether this set is empty
	bool empty();

	// Clear members
	void clear();

	// Selects all member nodes of this set
	void select();

	// De-selects all member nodes of this set
	void deselect();

	void addNode(const scene::INodePtr& node);

	// Clears this set and loads the currently selected nodes in the
	// scene as new members into this set.
	void assignFromCurrentScene();

	std::set<scene::INodePtr> getNodes();
};
typedef boost::shared_ptr<SelectionSet> SelectionSetPtr;

} // namespace
