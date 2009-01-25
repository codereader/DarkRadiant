#ifndef PATCHSCENEWALKERS_H_
#define PATCHSCENEWALKERS_H_

#include "ipatch.h"
#include "iscenegraph.h"
#include "iselection.h"

// Visits each visible patch with the given functor
template<typename Functor>
class PatchForEachWalker : 
	public scene::NodeVisitor
{
	const Functor& _functor;
public:
	PatchForEachWalker(const Functor& functor) : 
		_functor(functor)
	{}
	
	bool pre(const scene::INodePtr& node) {
		// Skip hidden paths
		if (!node->visible()) return false;

		Patch* patch = Node_getPatch(node);

		if (patch != NULL) {
			_functor(*patch);
			return false;
		}
		
		return true;
	}
};

// ------------------------------------------------------------------------------------

template<typename Functor>
class PatchSelectedVisitor : 
	public SelectionSystem::Visitor
{
	const Functor& _functor;
public:
	PatchSelectedVisitor(const Functor& functor) : 
		_functor(functor)
	{}

	void visit(const scene::INodePtr& node) const {
		// Skip hidden nodes
		if (!node->visible()) return;

		Patch* patch = Node_getPatch(node);
		if (patch != 0) {
			_functor(*patch);
		}
	}
};

/** 
 * Scene traversor methods for patches: visible, visible selected and visible selected instance
 */
template<typename Functor>
inline void Scene_forEachVisiblePatch(const Functor& functor) {
	PatchForEachWalker<Functor> walker(functor);
	Node_traverseSubgraph(GlobalSceneGraph().root(), walker);
}

// Selection traversors using the above visitor classes, selected patch and selected patch instance
template<typename Functor>
inline void Scene_forEachSelectedPatch(const Functor& functor) {
	PatchSelectedVisitor<Functor> walker(functor);
	GlobalSelectionSystem().foreachSelected(walker);
}

#endif /*PATCHSCENEWALKERS_H_*/
