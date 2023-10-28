#include "SelectionIndex.h"

#include <stdexcept>
#include "iscenegraph.h"
#include "iorthoview.h"
#include "ientity.h"
#include "scenelib.h"
#include "command/ExecutionFailure.h"

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
		auto parent = node->getParent();

		// In rare cases, such as when a drag-selection brush is deleted
		// but still selected, we might reach this point with a primitive
		// node without a parent. At least we shouldn't crash.
		if (parent)
		{
			// Node is a primitive, find parent entity and child index
			EntityFindIndexWalker walker(parent);
			GlobalSceneGraph().root()->traverse(walker);

			result.first = walker.getIndex(); // might throw

			PrimitiveFindIndexWalker brushWalker(node);
			parent->traverseChildren(brushWalker);

			result.second = brushWalker.getIndex(); // might throw
		}
	}
	else
	{
		throw std::out_of_range("Invalid node type passed");
	}

	return result;
}

class BrushFindByIndexWalker :
	public scene::NodeVisitor
{
private:
	std::size_t _index;
	scene::INodePtr _brush;

public:
	BrushFindByIndexWalker(std::size_t index) :
		_index(index)
	{}

	const scene::INodePtr& getFoundNode() const
	{
		return _brush;
	}

	bool pre(const scene::INodePtr& node) override
	{
		if (_brush) return false; // already found

		if (Node_isPrimitive(node) && _index-- == 0)
		{
			_brush = node; // found it
		}

		return false;
	}
};

class EntityFindByIndexWalker :
	public scene::NodeVisitor
{
private:
	std::size_t _index;
	scene::INodePtr _entity;

public:
	EntityFindByIndexWalker(std::size_t index) :
		_index(index)
	{}

	const scene::INodePtr& getFoundNode() const
	{
		return _entity;
	}

	bool pre(const scene::INodePtr& node) override
	{
		if (_entity) return false; // already found

		if (Node_isEntity(node) && _index-- == 0)
		{
			_entity = node;
		}

		return false;
	}
};

scene::Path findMapElementByIndex(std::size_t entityNum, std::size_t brushNum)
{
	scene::Path path;
	path.push(GlobalSceneGraph().root());

	EntityFindByIndexWalker entityFinder(entityNum);
	GlobalSceneGraph().root()->traverseChildren(entityFinder);

	auto foundEntity = entityFinder.getFoundNode();

	if (foundEntity)
	{
		path.push(foundEntity);

		BrushFindByIndexWalker brushFinder(brushNum);
		foundEntity->traverseChildren(brushFinder);

		auto foundBrush = brushFinder.getFoundNode();

		if (foundBrush)
		{
			path.push(foundBrush);
		}
	}

	return path;
}

inline bool Node_hasChildren(scene::INodePtr node)
{
	return node->hasChildNodes();
}

void selectNodeByIndex(std::size_t entitynum, std::size_t brushnum)
{
	scene::Path path = findMapElementByIndex(entitynum, brushnum);

	if (path.size() == 3 || (path.size() == 2 && !Node_hasChildren(path.top())))
	{
		Node_setSelected(path.top(), true);

		GlobalOrthoViewManager().positionActiveView(path.top()->worldAABB().origin);
	}
}

void selectNodeByIndexCmd(const cmd::ArgumentList& args)
{
	if (args.size() != 2)
	{
		rWarning() << "Usage: SelectNodeByIndex <entityNumber> <brushNumber>" << std::endl;
		return;
	}

	int entityNumber = args[0].getInt();
	int brushNumber = args[1].getInt();

	if (entityNumber < 0 && brushNumber < 0)
	{
		throw cmd::ExecutionFailure("The Entity and Brush numbers must not be negative.");
	}

	selectNodeByIndex(static_cast<std::size_t>(entityNumber), static_cast<std::size_t>(brushNumber));
}

}
