#include "Traverse.h"

#include "iscenegraph.h"
#include "scenelib.h"
#include "ibrush.h"
#include "ipatch.h"

namespace scene
{

class IncludeSelectedWalker :
	public scene::NodeVisitor
{
	scene::NodeVisitor& m_walker;
	const std::set<scene::INode*> *m_subsetOverride;
	mutable std::size_t m_selected;
	mutable bool m_skip;

	bool isSelected(const scene::INodePtr& node) const {
		if (m_subsetOverride)
			return m_subsetOverride->count(node.get());
		else
			return Node_isSelected(node);
	}
	
	bool hasSelectedParent() const {
		return m_selected != 0;
	}

	bool hasSelectedChildren(const scene::INodePtr& node) const {
		bool selected = false;
		node->foreachNode([&] (const scene::INodePtr& child) -> bool {
			if (isSelected(child)) {
				selected = true;
				return false; // stop searching
			}
			return true;
		});
		return selected;
	}
public:
	IncludeSelectedWalker(scene::NodeVisitor& walker) :
		m_walker(walker),
		m_subsetOverride(nullptr),
		m_selected(0),
		m_skip(false)
	{}

	//stgatilov: override subset of selected nodes
	void overrideSelectedSubset(const std::set<scene::INode*> &nodes) {
		m_subsetOverride = &nodes;
	}

	virtual bool pre(const scene::INodePtr& node) {
		// include node if:
		// node is not a 'root' AND ( node is selected OR any child of node is selected OR any parent of node is selected )
		if (!node->isRoot() && (isSelected(node) || hasSelectedChildren(node) || hasSelectedParent()))
		{
			if (isSelected(node))
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
			if (isSelected(node)) {
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

void foreachVisiblePatch(const std::function<void(const IPatchNodePtr&)>& functor)
{
	GlobalSceneGraph().root()->foreachNode([&](const scene::INodePtr& node)->bool
	{
		if (Node_isPatch(node) && node->visible())
		{
			functor(std::dynamic_pointer_cast<IPatchNode>(node));
		}

		return true;
	});
}

std::function<void(const scene::INodePtr&, scene::NodeVisitor&)> traverseSubset(const std::set<scene::INode*> &nodes) {
	//note: copy nodes set into lambda
	auto functor = [nodes](const scene::INodePtr& root, scene::NodeVisitor& nodeExporter) {
		IncludeSelectedWalker visitor(nodeExporter);
		visitor.overrideSelectedSubset(nodes);
		root->traverseChildren(visitor);
	};
	return functor;
}

} // namespace map
