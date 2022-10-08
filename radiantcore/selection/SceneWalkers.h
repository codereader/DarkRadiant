#pragma once

#include "ientity.h"
#include "ieclass.h"
#include "itransformnode.h"
#include "itextstream.h"
#include "igroupnode.h"
#include "scenelib.h"
#include "iselectiontest.h"
#include "editable.h"
#include "brush/BrushNode.h"

// ----------- The Walker Classes ------------------------------------------------

// Selects the visited component instances in the graph, according to the current component mode
class SelectAllComponentWalker :
	public scene::NodeVisitor
{
	bool _select;
    selection::ComponentSelectionMode _mode;

public:
	SelectAllComponentWalker(bool select, selection::ComponentSelectionMode mode) :
		_select(select),
		_mode(mode)
	{}

	bool pre(const scene::INodePtr& node)
	{
		ComponentSelectionTestablePtr componentSelectionTestable = Node_getComponentSelectionTestable(node);

		if (componentSelectionTestable != NULL)
		{
			componentSelectionTestable->setSelectedComponents(_select, _mode);
		}

		return true;
	}
};

// Traverses through the scenegraph and removes degenerated brushes from the selected.
// greebo: The actual erasure is performed in the destructor to keep the scenegraph intact during traversal.
class RemoveDegenerateBrushWalker :
    public selection::SelectionSystem::Visitor
{
	mutable std::list<scene::INodePtr> _eraseList;
public:
	// Destructor removes marked paths
	~RemoveDegenerateBrushWalker() override
    {
        for (const auto& node : _eraseList)
        {
            // Check if the parent has any children left at all
            auto parent = node->getParent();

            // Remove the node from the scene
            removeNodeFromParent(node);

            if (parent && !parent->hasChildNodes())
            {
                rError() << "Warning: removing empty parent entity." << std::endl;
                removeNodeFromParent(parent);
            }
        }
	}

	void visit(const scene::INodePtr& node) const override
	{
		if (auto brush = Node_getBrush(node); brush)
		{
            brush->evaluateBRep();

            if (!brush->hasContributingFaces())
            {
			    // greebo: Mark this path for removal
			    _eraseList.push_back(node);

			    rError() << "Warning: removed degenerate brush!\n";
            }
		}
	}
};

namespace scene
{

inline bool freezeTransformableNode(const scene::INodePtr& node)
{
	ITransformablePtr transform = scene::node_cast<ITransformable>(node);

	if (transform)
	{
		transform->freezeTransform();
	}

	return true;
}

} // namespace

/**
 * greebo: Traverses the selection and invokes the functor on
 * each encountered primitive.
 *
 * The SelectionWalker traverses the currently selected instances and
 * passes Brushes and Patches right to the PrimitiveVisitor. When
 * GroupNodes are encountered, the GroupNode itself is traversed
 * and all child primitives are passed to the PrimitiveVisitor as well.
 */
class SelectionWalker :
	public scene::NodeVisitor
{
public:
	void visit(const scene::INodePtr& node)
	{
		// Check if we have an entity
		scene::GroupNodePtr groupNode = Node_getGroupNode(node);

		if (groupNode != NULL)
		{
			// We have a selected groupnode, traverse it using self as walker
			node->traverseChildren(*this);
			return;
		}

		handleNode(node);
	}

	// NodeVisitor implemenatation
	bool pre(const scene::INodePtr& node)
	{
		visit(node);
		return true; // traverse further
	}

protected:
	// Type-specific handler method
	virtual void handleNode(const scene::INodePtr& node) = 0;
};

// Walker specialisation for Brushes
class BrushSelectionWalker :
	public SelectionWalker
{
	typedef std::function<void(Brush&)> VisitFunc;
	VisitFunc _functor;
public:
	BrushSelectionWalker(const VisitFunc& functor) :
		_functor(functor)
	{}

protected:
	void handleNode(const scene::INodePtr& node)
	{
		Brush* brush = Node_getBrush(node);

		if (brush != NULL)
		{
			_functor(*brush);
		}
	}
};

// Walker specialisation for Patches
class PatchSelectionWalker :
	public SelectionWalker
{
	typedef std::function<void(IPatch&)> VisitFunc;
	VisitFunc _functor;
public:
	PatchSelectionWalker(const VisitFunc& functor) :
		_functor(functor)
	{}

protected:
	void handleNode(const scene::INodePtr& node)
	{
		if (Node_isPatch(node))
		{
			_functor(*Node_getIPatch(node));
		}
	}
};

// Walker specialisation for Faces
class FaceSelectionWalker :
	public SelectionWalker
{
	typedef std::function<void(Face&)> VisitFunc;
	VisitFunc _functor;
public:
	FaceSelectionWalker(const VisitFunc& functor) :
		_functor(functor)
	{}

protected:
	void handleNode(const scene::INodePtr& node)
	{
		Brush* brush = Node_getBrush(node);

		if (brush != nullptr)
		{
            brush->forEachVisibleFace(_functor);
		}
	}
};
