#ifndef SELECTION_ALGORITHM_GENERAL_H_
#define SELECTION_ALGORITHM_GENERAL_H_

#include <list>
#include <string>
#include "icommandsystem.h"
#include "iscenegraph.h"
#include "ientity.h"
#include "math/Vector3.h"
#include "math/AABB.h"

namespace selection {
	namespace algorithm {

	typedef std::list<std::string> ClassnameList;

	/**
	 * greebo: This selects each visible entity in the subgraph whose classname matches
	 *         the given list.
	 */
	class EntitySelectByClassnameWalker :
		public scene::NodeVisitor
	{
		const ClassnameList& _classnames;
	public:
		EntitySelectByClassnameWalker(const ClassnameList& classnames);

		bool pre(const scene::INodePtr& node);

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
	void selectAllOfType(const cmd::ArgumentList& args);

	/**
	 * greebo: Hides all selected nodes in the scene.
	 */
	void hideSelected(const cmd::ArgumentList& args);

	/**
	 * greebo: Hides everything that is not selected.
	 */
	void hideDeselected(const cmd::ArgumentList& args);

	/**
	 * greebo: Clears the hidden flag of all nodes in the scene.
	 */
	void showAllHidden(const cmd::ArgumentList& args);

	/**
	 * greebo: Each selected item will be deselected and vice versa.
	 */
	void invertSelection(const cmd::ArgumentList& args);

	/**
	 * greebo: Removes all selected nodes. If entities end up with
	 *         no child nodes, the empty container is removed as well.
	 *
	 * Note: this is the command target, it triggers an UndoableCommand.
	 *       Use deleteSelection() to just remove the selection without
	 *       disrupting the undo stack.
	 */
	void deleteSelectionCmd(const cmd::ArgumentList& args);

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
	void selectInside(const cmd::ArgumentList& args);
	void selectTouching(const cmd::ArgumentList& args);
	void selectCompleteTall(const cmd::ArgumentList& args);

	// Returns the center point of the current selection (or <0,0,0> if nothing selected).
	Vector3 getCurrentSelectionCenter();

	// Returns the AABB of the current selection (invalid bounds if nothing is selected).
	AABB getCurrentSelectionBounds();

	// Calculates the axis-aligned bounding box of the selection components.
	AABB getCurrentComponentSelectionBounds();

	// Finds the index of the selected entity and primitive and 
	// writes the result back into the given numbers. Does nothing if no selection is present.
	void getSelectionIndex(std::size_t& ent, std::size_t& brush);

	/**
	 * Snaps the current (component) selection to the grid.
	 */
	void snapSelectionToGrid(const cmd::ArgumentList& args);

	/**
	 * Connect the various events to the functions in this namespace
	 */
	void registerCommands();

	} // namespace algorithm
} // namespace selection

#endif /* SELECTION_ALGORITHM_GENERAL_H_ */
