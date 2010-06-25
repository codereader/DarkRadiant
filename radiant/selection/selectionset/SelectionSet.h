#ifndef _SELECTION_SET_H_
#define _SELECTION_SET_H_

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

public:
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
};
typedef boost::shared_ptr<SelectionSet> SelectionSetPtr;

} // namespace

#endif /* _SELECTION_SET_H_ */
