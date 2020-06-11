#include "SelectionIndex.h"

#include <stdexcept>
#include "iscenegraph.h"
#include "ientity.h"
#include "scenelib.h"

namespace scene
{

class PrimitiveFindIndexWalker :
	public scene::NodeVisitor
{
private:
	scene::INodePtr _node;
	std::size_t _index;

public:
	PrimitiveFindIndexWalker(const scene::INodePtr& node) :
		_node(node),
		_index(0)
	{}

	// Returns the found index or throws a std::out_of_range exception
	std::size_t getIndex() const
	{
		if (_node)
		{
			throw std::out_of_range("Could not find the given node");
		}

		return _index;
	}

	bool pre(const scene::INodePtr& node) override
	{
		if (Node_isPrimitive(node))
		{
			// Have we found the node?
			if (_node == node)
			{
				// Yes, found, set needle to empty
				_node.reset();
			}

			// As long as the needle is non-NULL, increment the counter
			if (_node)
			{
				++_index;
			}
		}

		return true;
	}
};

class EntityFindIndexWalker :
	public scene::NodeVisitor
{
private:
	scene::INodePtr _node;
	std::size_t _index;
public:
	EntityFindIndexWalker(const scene::INodePtr& node) :
		_node(node),
		_index(0)
	{}

	// Returns the found index or throws a std::out_of_range exception
	std::size_t getIndex() const
	{
		if (_node)
		{
			throw std::out_of_range("Could not find the given node");
		}

		return _index;
	}

	bool pre(const scene::INodePtr& node) override
	{
		if (Node_isEntity(node))
		{
			// Have we found the node?
			if (_node == node)
			{
				// Yes, found, set needle to NULL
				_node.reset();
			}

			// As long as the needle is non-NULL, increment the counter
			if (_node)
			{
				++_index;
			}
		}

		return true;
	}
};

std::pair<std::size_t, std::size_t> getNodeIndices(const scene::INodePtr& node)
{
	auto result = std::make_pair<std::size_t, std::size_t>(0, 0);

	if (Node_isEntity(node))
	{
		// Selection is an entity, find its index
		EntityFindIndexWalker walker(node);
		GlobalSceneGraph().root()->traverse(walker);

		result.first = walker.getIndex(); // might throw
	}
	else if (Node_isPrimitive(node))
	{
		scene::INodePtr parent = node->getParent();

		// Node is a primitive, find parent entity and child index
		EntityFindIndexWalker walker(parent);
		GlobalSceneGraph().root()->traverse(walker);

		result.first = walker.getIndex(); // might throw

		PrimitiveFindIndexWalker brushWalker(node);
		parent->traverse(brushWalker);

		result.second = walker.getIndex(); // might throw
	}
	else
	{
		throw std::out_of_range("Invalid node type passed");
	}

	return result;
}

void selectNodeByIndex(const cmd::ArgumentList& args)
{
	if (args.size() != 2)
	{
		rWarning() << "Usage: SelectNodeByIndex <entityNumber> <brushNumber>" << std::endl;
		return;
	}

	int entityNumber = args[0].getInt();
	int brushNumber = args[1].getInt();

	// TODO
}

}
