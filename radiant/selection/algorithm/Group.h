#ifndef SELECTIONGROUP_H_
#define SELECTIONGROUP_H_

namespace selection {
	namespace algorithm {
	
	/** greebo: This reparents the child primitives of an entity container like func_static
	 * back to worldspawn and deletes the entity thereafter.  
	 */
	void revertGroupToWorldSpawn();

	/** greebo: This re-parents the selected primitives to an entity. The entity has to 
	 * 			be selected last. Emits an error message if the selection doesn't meet
	 * 			the requirements
	 */
	void parentSelection();
	
	/** greebo: Selects the children of the currently selected groupnodes.
	 * 			This deselects the groupnodes entities, so that ONLY the children 
	 * 			are highlighted.
	 */
	void selectChildren();

	/** 
	 * greebo: This selects all the children of an entity, given the case 
	 *         that a child of this entity is already selected. For instance, 
	 *         if a child brush of a func_static is selected, this command 
	 *         expands the selection to all other children (but not the 
	 *         func_static entity itself). Select a single primitive of 
	 *         the worldspawn entity and this command will select every primitive 
	 *         that is child of worldspawn. 
	 */
	void expandSelectionToEntities();

	} // namespace algorithm
} // namespace selection

#endif /*SELECTIONGROUP_H_*/
