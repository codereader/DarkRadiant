#pragma once

#include "iselection.h"
#include "FaceInstance.h"
#include "BrushNode.h"

template<typename Functor>
class BrushSelectedVisitor :
	public SelectionSystem::Visitor
{
	const Functor& m_functor;
public:
	BrushSelectedVisitor(const Functor& functor) : m_functor(functor) {}
	virtual ~BrushSelectedVisitor() {}

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
    virtual ~FaceInstanceVisitFace() {}
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
    virtual ~FaceVisitAll() {}
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

namespace scene
{

typedef std::function<void(Brush&)> BrushVisitFunc;
typedef std::function<void(FaceInstance&)> FaceInstanceVisitFunc;

class BrushVisitor :
    public scene::NodeVisitor
{
	const BrushVisitFunc _functor;
public:
	BrushVisitor(const BrushVisitFunc& functor) :
		_functor(functor)
	{}

    bool pre(const INodePtr& node)
	{
		if (!node->visible())
		{
			return false;
		}

		Brush* brush = Node_getBrush(node);

		if (brush != NULL)
		{
			_functor(*brush);
			return false;
		}

        return true;
    }
};

inline void foreachVisibleBrush(const BrushVisitFunc& functor)
{
	BrushVisitor visitor(functor);
	GlobalSceneGraph().root()->traverse(visitor);
}

inline void foreachVisibleFaceInstance(const FaceInstanceVisitFunc& functor)
{
	// Pass a special lambda as BrushVisitFunc to the existing walker
	BrushVisitor visitor([&] (Brush& brush)
	{
		brush.getBrushNode().forEachFaceInstance([&] (FaceInstance& instance)
		{
			if (instance.getFace().faceIsVisible())
			{
				functor(instance);
			}
		});
	});

	GlobalSceneGraph().root()->traverse(visitor);
}

} // namespace

template<typename Functor>
inline const Functor& Scene_forEachBrush(scene::Graph& graph, const Functor& functor) {
	NodeWalker< InstanceApply<BrushNode, Functor> > walker(functor);
	Node_traverseSubgraph(graph.root(), walker);
	return functor;
}

class BrushVisitEachFace :
	public scene::NodeVisitor
{
	const BrushInstanceVisitor& _visitor;
public:
	BrushVisitEachFace(const BrushInstanceVisitor& visitor) :
		_visitor(visitor)
	{}
	virtual ~BrushVisitEachFace() {}

	bool pre(const scene::INodePtr& node) {
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
	Node_traverseSubgraph(GlobalSceneGraph().root(), walker);
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
inline const Functor& Scene_ForEachSelectedBrushFace(scene::Graph& graph, const Functor& functor)
{
	selection::algorithm::forEachSelectedFaceComponent([&] (Face& face)
	{
		functor(face);
	});

	return functor;
}
