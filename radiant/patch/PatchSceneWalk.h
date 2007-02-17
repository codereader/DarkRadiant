#ifndef PATCHSCENEWALKERS_H_
#define PATCHSCENEWALKERS_H_

#include "iscenegraph.h"
#include "iselection.h"
#include "PatchInstance.h"

// Casts a node onto a patch
inline Patch* Node_getPatch(scene::Node& node) {
	return NodeTypeCast<Patch>::cast(node);
}

// Cast an instance onto a patch
inline PatchInstance* Instance_getPatch(scene::Instance& instance) {
  return InstanceTypeCast<PatchInstance>::cast(instance);
}

// ------------------------------------------------------------------------------------

template<typename Functor>
class PatchForEachWalker : public scene::Graph::Walker {
	const Functor& m_functor;
public:
	PatchForEachWalker(const Functor& functor) : m_functor(functor)
	{
	}
	
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		if(path.top().get().visible()) {
			Patch* patch = Node_getPatch(path.top());
			if(patch != 0) {
				m_functor(*patch);
			}
		}
		
		return true;
	}
};

template<typename Functor>
class PatchForEachSelectedWalker : public scene::Graph::Walker {
	const Functor& m_functor;
public:
	PatchForEachSelectedWalker(const Functor& functor) : m_functor(functor) 
	{
	}
	
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		if(path.top().get().visible()) {
			Patch* patch = Node_getPatch(path.top());
			if(patch != 0 && Instance_getSelectable(instance)->isSelected()) {
				m_functor(*patch);
			}
		}
		
		return true;
	}
};

template<typename Functor>
class PatchForEachInstanceWalker : public scene::Graph::Walker {
	const Functor& m_functor;
public:
	PatchForEachInstanceWalker(const Functor& functor) : m_functor(functor)
	{
	}

	bool pre(const scene::Path& path, scene::Instance& instance) const {
		if(path.top().get().visible()) {
			PatchInstance* patch = Instance_getPatch(instance);
			if(patch != 0) {
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

	void visit(scene::Instance& instance) const {
		PatchInstance* patch = Instance_getPatch(instance);
		if(patch != 0) {
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

	void visit(scene::Instance& instance) const {
		PatchInstance* patch = Instance_getPatch(instance);
		if(patch != 0 && instance.path().top().get().visible()) {
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

template<typename Functor>
inline void Scene_forEachVisiblePatchInstance(const Functor& functor) {
  GlobalSceneGraph().traverse(PatchForEachInstanceWalker<Functor>(functor));
}

// Selection traversors using the above visitor classes, selected patch and selected patch instance
template<typename Functor>
inline void Scene_forEachSelectedPatch(const Functor& functor) {
  GlobalSelectionSystem().foreachSelected(PatchSelectedVisitor<Functor>(functor));
}

template<typename Functor>
inline void Scene_forEachVisibleSelectedPatchInstance(const Functor& functor) {
  GlobalSelectionSystem().foreachSelected(PatchVisibleSelectedVisitor<Functor>(functor));
}

#endif /*PATCHSCENEWALKERS_H_*/
