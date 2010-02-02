#ifndef SELECTIONGROUP_H_
#define SELECTIONGROUP_H_

#include "icommandsystem.h"
#include "iselection.h"
#include <list>

namespace selection {
	namespace algorithm {
	
	class ParentSelectedPrimitivesToEntityWalker : 
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
		ParentSelectedPrimitivesToEntityWalker(const scene::INodePtr& parent) :
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

	/** greebo: This reparents the child primitives of an entity container like func_static
	 * back to worldspawn and deletes the entity thereafter.  
	 */
	void revertGroupToWorldSpawn(const cmd::ArgumentList& args);

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

	} // namespace algorithm
} // namespace selection

#endif /*SELECTIONGROUP_H_*/
