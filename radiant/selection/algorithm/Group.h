#ifndef SELECTIONGROUP_H_
#define SELECTIONGROUP_H_

#include "icommandsystem.h"
#include "iselection.h"
#include "inode.h"
#include <list>

namespace selection {
	namespace algorithm {

	class ParentPrimitivesToEntityWalker :
		public SelectionSystem::Visitor,
		public scene::NodeVisitor
	{
	private:
		// The target parent node
		const scene::INodePtr _parent;

		// The list of children to reparent
		mutable std::list<scene::INodePtr> _childrenToReparent;

		// Old parents will be checked for emptiness afterwards
		mutable std::set<scene::INodePtr> _oldParents;

	public:
		ParentPrimitivesToEntityWalker(const scene::INodePtr& parent) :
			_parent(parent)
		{}

		// Call this to perform the actual reparenting after traversal
		void reparent();

		// Selects all primitives which are to be reparented
		void selectReparentedPrimitives();

		// SelectionSystem::Visitor implementation
		void visit(const scene::INodePtr& node) const;

		// scene::NodeVisitor implementation
		bool pre(const scene::INodePtr& node);
	};

	// Collects all groupnodes
	class GroupNodeCollector :
		public SelectionSystem::Visitor
	{
	public:
		typedef std::list<scene::INodePtr> GroupNodeList;

	private:
		mutable GroupNodeList _groupNodes;

	public:
		void visit(const scene::INodePtr& node) const;

		const GroupNodeList& getList() const
		{
			return _groupNodes;
		}
	};

	// Checks the current selection to see whether it consists of
	// group nodes only.
	class GroupNodeChecker :
		public SelectionSystem::Visitor
	{
		mutable bool _onlyGroups;
		mutable std::size_t _numGroups;
		mutable scene::INodePtr _firstGroupNode;
	public:
		GroupNodeChecker();

		void visit(const scene::INodePtr& node) const;

		// Returns true if the current selection consists of group nodes only
		// Returns false if any selected node is a non-group or if nothing is selected.
		bool onlyGroupsAreSelected() const;

		// Returns the number of group nodes in the current selection
		std::size_t selectedGroupCount() const;

		// Returns the first group node of the selection or NULL if nothing selected
		scene::INodePtr getFirstSelectedGroupNode() const;
	};

	/**
	 * greebo: Takes the selected primitives and converts them to func_static.
	 */
	void convertSelectedToFuncStatic(const cmd::ArgumentList& args);

	/** greebo: This reparents the child primitives of an entity container like func_static
	 * back to worldspawn and deletes the entity thereafter.
	 */
	void revertGroupToWorldSpawn(const cmd::ArgumentList& args);

	/**
	 * Tests the current selection and returns true if the selection is suitable
	 * for reparenting the selected primitives to the (last) selected entity.
	 */
	bool curSelectionIsSuitableForReparent();

	/** greebo: This re-parents the selected primitives to an entity. The entity has to
	 * 			be selected last. Emits an error message if the selection doesn't meet
	 * 			the requirements
	 */
	void parentSelection(const cmd::ArgumentList& args);

	/**
	 * greebo: Like the above method, but specialises on parent operations
	 * to the worldspawn entity.
	 */
	void parentSelectionToWorldspawn(const cmd::ArgumentList& args);

	/** greebo: Selects the children of the currently selected groupnodes.
	 * 			This deselects the groupnodes entities, so that ONLY the children
	 * 			are highlighted.
	 */
	void selectChildren(const cmd::ArgumentList& args);

	/**
	 * greebo: This selects all the children of an entity, given the case
	 *         that a child of this entity is already selected. For instance,
	 *         if a child brush of a func_static is selected, this command
	 *         expands the selection to all other children (but not the
	 *         func_static entity itself). Select a single primitive of
	 *         the worldspawn entity and this command will select every primitive
	 *         that is child of worldspawn.
	 */
	void expandSelectionToEntities(const cmd::ArgumentList& args);

	/**
	 * greebo: Merges all selected group nodes (func_* entities).
	 * After this operation only the first group node is preserved
	 * and all child primitives are parented to it.
	 * The other group nodes are removed from the scene.
	 */
	void mergeSelectedEntities(const cmd::ArgumentList& args);

	} // namespace algorithm
} // namespace selection

#endif /*SELECTIONGROUP_H_*/
