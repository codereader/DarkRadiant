#ifndef GROUPCYCLE_H_
#define GROUPCYCLE_H_

#include <vector>
#include "scenelib.h"

namespace selection {

typedef std::vector<scene::INodePtr> NodeVector;

/** greebo: This class allows cycling through the child primitives of the
 * currently selected entity (like func_static).
 */
class GroupCycle :
	public SelectionSystem::Observer
{
	// The list of possible Selectable candidates
	NodeVector _list;
	
	// The current index in the vector
	int _index;

	// This is to avoid callback loops
	bool _updateActive;

public:
	GroupCycle();
	
	/** greebo: The callback that gets invoked upon selectionChange
	 * by the RadiantSelectionSystem
	 */
	void selectionChanged(const scene::INodePtr& node, bool isComponent);
	
	/** greebo: Rescans the current selection and populates the Vector of candidates
	 */
	void rescanSelection();
	
	/** greebo: Selects the currently active instance (where _index is pointing to) 
	 */
	void updateSelection();
	
	/** greebo: This actually performs the cycling
	 */
	void doCycleForward();
	void doCycleBackward();
	
	// The targets for the according Events (have to be static to allow
	// them being used in FreeCaller<> templates instead of MemberCallers.
	static void cycleBackward();
	static void cycleForward();
	
	/** greebo: This contains the static instance of this class
	 */
	static GroupCycle& Instance();
};

} // namespace selection

#endif /*GROUPCYCLE_H_*/
