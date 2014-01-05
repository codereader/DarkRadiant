#pragma once

#include "ipatch.h"
#include "iscenegraph.h"
#include "iselection.h"

namespace scene
{

typedef std::function<void(Patch&)> PatchVisitFunc;

/**
 * Scene traversor methods for patches: visible and selected instance
 */
class PatchVisitor :
    public scene::NodeVisitor
{
	const PatchVisitFunc& _functor;
public:
	PatchVisitor(const PatchVisitFunc& functor) :
		_functor(functor)
	{}

    bool pre(const INodePtr& node)
	{
		if (!node->visible())
		{
			return false;
		}

		Patch* patch = Node_getPatch(node);

		if (patch != NULL)
		{
			_functor(*patch);
			return false;
		}

        return true;
    }
};

inline void foreachVisiblePatch(const PatchVisitFunc& functor)
{
	PatchVisitor visitor(functor);
	GlobalSceneGraph().root()->traverseChildren(visitor);
}

} // namespace
