#include "Traverse.h"

#include "scenelib.h"
#include "ibrush.h"
#include "ipatch.h"

namespace scene
{

class IncludeSelectedWalker :
	public scene::NodeVisitor
{
	scene::NodeVisitor& m_walker;
	mutable std::size_t m_selected;
	mutable bool m_skip;

	bool selectedParent() const {
		return m_selected != 0;
	}
public:
	IncludeSelectedWalker(scene::NodeVisitor& walker) :
		m_walker(walker),
		m_selected(0),
		m_skip(false)
	{}

	virtual bool pre(const scene::INodePtr& node) {
		// include node if:
		// node is not a 'root' AND ( node is selected OR any child of node is selected OR any parent of node is selected )
		if (!node->isRoot() && (Node_isSelected(node) || Node_hasSelectedChildNodes(node) || selectedParent()))
		{
			if (Node_isSelected(node))
			{
				++m_selected;
			}
			m_walker.pre(node);
			return true;
		}
		else {
			m_skip = true;
			return false;
		}
	}

	virtual void post(const scene::INodePtr& node) {
		if (m_skip) {
			m_skip = false;
		}
		else {
			if (Node_isSelected(node)) {
				--m_selected;
			}
			m_walker.post(node);
		}
	}
};

void traverseSelected(const scene::INodePtr& root, scene::NodeVisitor& nodeExporter)
{
	// Create a wrapper which calls the nodeExporter only for selected and related items
	IncludeSelectedWalker visitor(nodeExporter);
	root->traverseChildren(visitor);
}

void traverse(const scene::INodePtr& root, scene::NodeVisitor& nodeExporter)
{
	// Just traverse the root using the given nodeExporter, no special rules
	root->traverseChildren(nodeExporter);
}

void foreachVisibleFace(const std::function<void(IFace&)>& functor)
{
	GlobalSceneGraph().root()->foreachNode([&](const scene::INodePtr& node)->bool
	{
		if (Node_isBrush(node) && node->visible())
		{
			auto* brush = Node_getIBrush(node);
			for (std::size_t i = 0; i < brush->getNumFaces(); ++i)
			{
				auto& face = brush->getFace(i);

				if (face.isVisible())
				{
					functor(face);
				}
			}
		}

		return true;
	});
}

void foreachVisiblePatch(const std::function<void(IPatch&)>& functor)
{
	GlobalSceneGraph().root()->foreachNode([&](const scene::INodePtr& node)->bool
	{
		if (Node_isPatch(node) && node->visible())
		{
			functor(*Node_getIPatch(node));
		}

		return true;
	});
}

} // namespace
