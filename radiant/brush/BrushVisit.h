#ifndef BRUSHVISIT_H_
#define BRUSHVISIT_H_

#include "iselection.h"
#include "FaceInstance.h"
#include "BrushNode.h"

extern FaceInstanceSet g_SelectedFaceInstances;

template<typename Functor>
class BrushSelectedVisitor : 
	public SelectionSystem::Visitor
{
	const Functor& m_functor;
public:
	BrushSelectedVisitor(const Functor& functor) : m_functor(functor) {}

	void visit(const scene::INodePtr& node) const {
		BrushNodePtr brush = boost::dynamic_pointer_cast<BrushNode>(node);
		if (brush != 0) {
			m_functor(brush);
		}
	}
};

template<typename Functor>
inline const Functor& Scene_forEachSelectedBrush(const Functor& functor) {
	GlobalSelectionSystem().foreachSelected(BrushSelectedVisitor<Functor>(functor));
	return functor;
}

template<typename Functor>
class BrushVisibleSelectedVisitor : 
	public SelectionSystem::Visitor
{
	const Functor& m_functor;
public:
	BrushVisibleSelectedVisitor(const Functor& functor) : 
		m_functor(functor)
	{}

	void visit(const scene::INodePtr& node) const {
		Brush* brush = Node_getBrush(node);

		if (brush != NULL && node->visible()) {
			m_functor(*brush);
		}
	}
};

template<typename Functor>
inline const Functor& Scene_forEachVisibleSelectedBrush(const Functor& functor) {
	GlobalSelectionSystem().foreachSelected(BrushVisibleSelectedVisitor<Functor>(functor));
	return functor;
}

class BrushForEachFace {
	const BrushInstanceVisitor& m_visitor;
public:
	BrushForEachFace(const BrushInstanceVisitor& visitor) : 
		m_visitor(visitor)
	{}

	void operator()(const BrushNodePtr& brush) const {
		brush->forEachFaceInstance(m_visitor);
	}
};

template<class Functor>
class FaceInstanceVisitFace : public BrushInstanceVisitor {
	const Functor& functor;
public:
	FaceInstanceVisitFace(const Functor& functor)
		: functor(functor) {}
	void visit(FaceInstance& face) const {
		functor(face.getFace());
	}
};

template<class Functor>
class FaceVisitAll : public BrushVisitor {
	const Functor& functor;
public:
	FaceVisitAll(const Functor& functor)
		: functor(functor) {}
	void visit(Face& face) const {
		functor(face);
	}
};

template<typename Functor>
inline const Functor& Brush_forEachFace(const Brush& brush, const Functor& functor) {
	brush.forEachFace(FaceVisitAll<Functor>(functor));
	return functor;
}

template<typename Functor>
inline const Functor& Brush_forEachFace(Brush& brush, const Functor& functor) {
	brush.forEachFace(FaceVisitAll<Functor>(functor));
	return functor;
}

template<class Functor>
class FaceInstanceVisitAll : public BrushInstanceVisitor {
	const Functor& functor;
public:
	FaceInstanceVisitAll(const Functor& functor)
		: functor(functor) {}
	void visit(FaceInstance& face) const {
		functor(face);
	}
};

/*template<typename Functor>
inline const Functor& Brush_ForEachFaceInstance(BrushInstance& brush, const Functor& functor) {
	brush.forEachFaceInstance(FaceInstanceVisitAll<Functor>(functor));
	return functor;
}*/

template<typename Functor>
inline const Functor& Scene_forEachBrush(scene::Graph& graph, const Functor& functor) {
	NodeWalker< InstanceApply<BrushNode, Functor> > walker(functor);
	graph.traverse(walker);
	return functor;
}

class BrushVisitEachFace :
	public scene::Graph::Walker
{
	const BrushInstanceVisitor& _visitor;
public:
	BrushVisitEachFace(const BrushInstanceVisitor& visitor) :
		_visitor(visitor)
	{}

	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		BrushNodePtr brush = boost::dynamic_pointer_cast<BrushNode>(node);
		if (brush != NULL) {
			brush->forEachFaceInstance(_visitor);
			return false;
		}
		return true;
	}
};

// Visit each face of each brush in the scene
inline void Scene_ForEachBrush_ForEachFace(const BrushInstanceVisitor& visitor) {
	BrushVisitEachFace walker(visitor);
	GlobalSceneGraph().traverse(walker);
}

template<typename Functor>
inline const Functor& Scene_ForEachBrush_ForEachFace(scene::Graph& graph, const Functor& functor) {
	Scene_forEachBrush(graph, BrushForEachFace(FaceInstanceVisitFace<Functor>(functor)));
	return functor;
}

template<typename Functor>
inline const Functor& Scene_ForEachSelectedBrush_ForEachFace(scene::Graph& graph, const Functor& functor) {
	Scene_forEachSelectedBrush(BrushForEachFace(FaceInstanceVisitFace<Functor>(functor)));
	return functor;
}

template<typename Functor>
inline const Functor& Scene_ForEachSelectedBrush_ForEachFaceInstance(scene::Graph& graph, const Functor& functor) {
	Scene_forEachSelectedBrush(BrushForEachFace(FaceInstanceVisitAll<Functor>(functor)));
	return functor;
}

template<typename Functor>
class FaceVisitorWrapper {
	const Functor& functor;
public:
	FaceVisitorWrapper(const Functor& functor) : functor(functor) {}

	void operator()(FaceInstance& faceInstance) const {
		functor(faceInstance.getFace());
	}
};

template<typename Functor>
inline const Functor& Scene_ForEachSelectedBrushFace(scene::Graph& graph, const Functor& functor) {
	g_SelectedFaceInstances.foreach(FaceVisitorWrapper<Functor>(functor));
	return functor;
}

#endif /*BRUSHVISIT_H_*/
