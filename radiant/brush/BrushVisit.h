#pragma once

#include "iselection.h"
#include "FaceInstance.h"
#include "BrushNode.h"

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
	GlobalSceneGraph().root()->traverseChildren(visitor);
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

	GlobalSceneGraph().root()->traverseChildren(visitor);
}

} // namespace
