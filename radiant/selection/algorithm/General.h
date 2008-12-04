#ifndef SELECTION_ALGORITHM_GENERAL_H_
#define SELECTION_ALGORITHM_GENERAL_H_

#include <list>
#include <string>
#include "iscenegraph.h"
#include "ientity.h"
#include "math/Vector3.h"
#include "math/aabb.h"

namespace selection {
	namespace algorithm {

	typedef std::list<std::string> ClassnameList; 

	/**
	 * greebo: This selects each entity in the scene whose classname matches
	 *         the given list.
	 */
	class EntitySelectByClassnameWalker : 
		public scene::Graph::Walker
	{
		const ClassnameList& _classnames;
	public:
		EntitySelectByClassnameWalker(const ClassnameList& classnames);

		bool pre(const scene::Path& path, const scene::INodePtr& node) const;

	private:
		bool entityMatches(Entity* entity) const;
	};

	/** 
	 * greebo: "Select All of Type" expands the selection to all items 
	 *         of similar type. The exact action depends on the current selection.
	 *
	 * For entities: all entities of the same classname as the selection are selected.
	 *
	 * For primitives: all items having the current shader (as selected in the texture
	 *                 browser) are selected.
	 *
	 * For faces: all faces carrying the current shader (as selected in the texture
	 *            browser) are selected.
	 */
	void selectAllOfType();

	/**
	 * greebo: Hides all selected nodes in the scene.
	 */
	void hideSelected();

	/**
	 * greebo: Hides everything that is not selected.
	 */
	void hideDeselected();

	/**
	 * greebo: Clears the hidden flag of all nodes in the scene.
	 */
	void showAllHidden();

	/**
	 * greebo: Each selected item will be deselected and vice versa.
	 */
	void invertSelection();

	/**
	 * greebo: Removes all selected nodes. If entities end up with
	 *         no child nodes, the empty container is removed as well.
	 *
	 * Note: this is the command target, it triggers an UndoableCommand.
	 *       Use deleteSelection() to just remove the selection without
	 *       disrupting the undo stack.
	 */
	void deleteSelectionCmd();

	/**
	 * greebo: Deletes all selected nodes including empty entities. 
	 * 
	 * Note: this command does not create an UndoableCommand, it only
	 *       contains the deletion algorithm.
	 */
	void deleteSelection();

	/**
	 * greebo: As the name says, these are the various selection routines.
	 */
	void selectInside();
	void selectTouching();
	void selectCompleteTall();

	// Returns the center point of the current selection (or <0,0,0> if nothing selected).
	Vector3 getCurrentSelectionCenter();

	// Returns the AABB of the current selection (invalid bounds if nothing is selected).
	AABB getCurrentSelectionBounds();

	} // namespace algorithm
} // namespace selection

#endif /* SELECTION_ALGORITHM_GENERAL_H_ */
