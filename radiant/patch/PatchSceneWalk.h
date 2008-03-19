#ifndef PATCHSCENEWALKERS_H_
#define PATCHSCENEWALKERS_H_

#include "ipatch.h"
#include "iscenegraph.h"
#include "iselection.h"

template<typename Functor>
class PatchForEachWalker : public scene::Graph::Walker {
	const Functor& m_functor;
public:
	PatchForEachWalker(const Functor& functor) : m_functor(functor)
	{
	}
	
	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		if (node->visible()) {
			Patch* patch = Node_getPatch(node);
			if (patch != NULL) {
				m_functor(*patch);
			}
		}
		
		return true;
	}
};

template<typename Functor>
class PatchForEachSelectedWalker : 
	public scene::Graph::Walker
{
	const Functor& m_functor;
public:
	PatchForEachSelectedWalker(const Functor& functor) : m_functor(functor) 
	{}
	
	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		if (node->visible()) {
			Patch* patch = Node_getPatch(node);
			if (patch != 0 && Node_isSelected(node)) {
				m_functor(*patch);
			}
		}
		
		return true;
	}
};

// ------------------------------------------------------------------------------------

template<typename Functor>
class PatchSelectedVisitor : public SelectionSystem::Visitor {
	const Functor& m_functor;
public:
	PatchSelectedVisitor(const Functor& functor) : m_functor(functor)
	{
	}

	void visit(const scene::INodePtr& node) const {
		Patch* patch = Node_getPatch(node);
		if (patch != 0) {
			m_functor(*patch);
		}
	}
};

template<typename Functor>
class PatchVisibleSelectedVisitor : public SelectionSystem::Visitor {
	const Functor& m_functor;
public:
	PatchVisibleSelectedVisitor(const Functor& functor) : m_functor(functor) 
	{
	}

	void visit(const scene::INodePtr& node) const {
		Patch* patch = Node_getPatch(node);

		if (patch != NULL && node->visible()) {
			m_functor(*patch);
		}
	}
};

/* Scene traversor methods for patches: visible, visible selected and visible selected instance
 */
template<typename Functor>
inline void Scene_forEachVisiblePatch(const Functor& functor) {
  GlobalSceneGraph().traverse(PatchForEachWalker<Functor>(functor));
}

template<typename Functor>
inline void Scene_forEachVisibleSelectedPatch(const Functor& functor) {
  GlobalSceneGraph().traverse(PatchForEachSelectedWalker<Functor>(functor));
}

// Selection traversors using the above visitor classes, selected patch and selected patch instance
template<typename Functor>
inline void Scene_forEachSelectedPatch(const Functor& functor) {
  GlobalSelectionSystem().foreachSelected(PatchSelectedVisitor<Functor>(functor));
}

#endif /*PATCHSCENEWALKERS_H_*/
