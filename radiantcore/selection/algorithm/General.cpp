#include "General.h"

#include "imodel.h"
#include "iselection.h"
#include "iundo.h"
#include "igrid.h"
#include "iradiant.h"
#include "imodelsurface.h"
#include "scenelib.h"
#include "iselectiontest.h"
#include "itraceable.h"

#include "math/Ray.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
#include "string/convert.h"
#include "selectionlib.h"
#include "entitylib.h"
#include "scene/SelectionIndex.h"

#include "SelectionPolicies.h"
#include "selection/SceneWalkers.h"
#include "selection/algorithm/Primitives.h"
#include "selection/algorithm/Transformation.h"
#include "selection/algorithm/Group.h"
#include "selection/clipboard/Clipboard.h"
#include "selection/algorithm/Curves.h"
#include "selection/algorithm/Entity.h"
#include "selection/algorithm/GroupCycle.h"
#include "selection/algorithm/Shader.h"
#include "brush/BrushVisit.h"
#include "patch/Patch.h"
#include "patch/PatchNode.h"
#include "messages/GridSnapRequest.h"

#include <stack>

namespace selection
{

namespace algorithm
{

EntitySelectByClassnameWalker::EntitySelectByClassnameWalker(const ClassnameList& classnames) :
	_classnames(classnames)
{}

bool EntitySelectByClassnameWalker::pre(const scene::INodePtr& node) {
	// don't traverse invisible nodes
	if (!node->visible()) return false;

	Entity* entity = Node_getEntity(node);

	if (entity != NULL) {

		if (entityMatches(entity)) {
			// Got a matching entity
			Node_setSelected(node, true);
		}

		// Don't traverse entities
		return false;
	}

	// Not an entity, traverse
	return true;
}

bool EntitySelectByClassnameWalker::entityMatches(Entity* entity) const {
	for (ClassnameList::const_iterator i = _classnames.begin();
		 i != _classnames.end(); ++i)
	{
		if (entity->getKeyValue("classname") == *i) {
			return true;
		}
	}

	return false;
}

void selectAllOfType(const cmd::ArgumentList& args)
{
	if (GlobalSelectionSystem().getSelectionInfo().componentCount > 0 &&
		!FaceInstance::Selection().empty())
	{
		std::set<std::string> shaders;

		// SELECT DISTINCT shader FROM selected_faces
		forEachSelectedFaceComponent([&] (IFace& face)
		{
			shaders.insert(face.getShader());
		});

		// fall back to the one selected in the texture browser
		if (shaders.empty())
		{
			shaders.insert(ShaderClipboard::Instance().getSource().getShader());
		}

		// Deselect all faces
		GlobalSelectionSystem().setSelectedAllComponents(false);

		// Select all faces carrying any of the shaders in the set
		scene::foreachVisibleFaceInstance([&] (FaceInstance& instance)
		{
			if (shaders.find(instance.getFace().getShader()) != shaders.end())
			{
				instance.setSelected(selection::ComponentSelectionMode::Face, true);
			}
		});

		// Select all visible patches carrying any of the shaders in the set
		scene::foreachVisiblePatch([&] (const IPatchNodePtr& node)
		{
			if (shaders.find(node->getPatch().getShader()) != shaders.end())
			{
				Node_setSelected(std::dynamic_pointer_cast<scene::INode>(node), true);
			}
		});
	}
	else
	{
		// Find any classnames of selected entities
		ClassnameList classnames;
		GlobalSelectionSystem().foreachSelected([&] (const scene::INodePtr& node)
		{
			Entity* entity = Node_getEntity(node);

			if (entity != NULL)
			{
				classnames.push_back(entity->getKeyValue("classname"));
			}
		});

		// De-select everything
		GlobalSelectionSystem().setSelectedAll(false);

		if (!classnames.empty())
		{
			// Instantiate a selector class
			EntitySelectByClassnameWalker classnameSelector(classnames);

			// Traverse the scenegraph, select all matching the classname list
			GlobalSceneGraph().root()->traverse(classnameSelector);
		}
		else
		{
			// No entities found, select all elements with textures
			// matching the one in the texture browser
			auto shader = ShaderClipboard::Instance().getSource().getShader();

			scene::foreachVisibleBrush([&] (Brush& brush)
			{
				if (brush.hasShader(shader))
				{
					brush.getBrushNode().setSelected(true);
				}
			});

			// Select all visible patches carrying any of the shaders in the set
			scene::foreachVisiblePatch([&] (const IPatchNodePtr& node)
			{
				if (node->getPatch().getShader() == shader)
				{
					Node_setSelected(std::dynamic_pointer_cast<scene::INode>(node), true);
				}
			});
		}
	}

	SceneChangeNotify();
}

inline void hideSubgraph(const scene::INodePtr& node, bool hide)
{
	if (hide)
	{
        scene::hideSubgraph(node);
	}
	else
	{
        scene::showSubgraph(node);
	}
}

// If the given node has any components of any kind, set the component selection status to the given flag
inline void setComponentSelection(const scene::INodePtr& node, bool selected)
{
	ComponentSelectionTestablePtr componentSelectionTestable = Node_getComponentSelectionTestable(node);

	if (componentSelectionTestable)
	{
		componentSelectionTestable->setSelectedComponents(selected, selection::ComponentSelectionMode::Vertex);
		componentSelectionTestable->setSelectedComponents(selected, selection::ComponentSelectionMode::Edge);
		componentSelectionTestable->setSelectedComponents(selected, selection::ComponentSelectionMode::Face);
	}
}

void hideSelected(const cmd::ArgumentList& args)
{
	// Traverse the selection, hiding all nodes
	GlobalSelectionSystem().foreachSelected([] (const scene::INodePtr& node)
	{
		hideSubgraph(node, true);

		// De-select all components of the node that is going to be hidden (#5054)
		setComponentSelection(node, false);
	});

	// Then de-select the hidden nodes
	GlobalSelectionSystem().setSelectedAll(false);

	SceneChangeNotify();
}

// Hides all nodes that are not selected
class HideDeselectedWalker :
	public scene::NodeVisitor
{
	bool _hide;

	std::stack<bool> _stack;
public:
	HideDeselectedWalker(bool hide) :
		_hide(hide)
	{}

	bool pre(const scene::INodePtr& node)
	{
		// Check the selection status
		bool isSelected = Node_isSelected(node);

		// greebo: Don't check root nodes for selected state
		if (!node->isRoot() && isSelected)
		{
			// We have a selected instance, "remember" this by setting the parent
			// stack element to TRUE
			if (!_stack.empty())
			{
				_stack.top() = true;
			}
		}

		// We are going one level deeper, add a new stack element for this subtree
		_stack.push(false);

		// Try to go deeper, but don't do this for deselected instances
		return !isSelected;
	}

	void post(const scene::INodePtr& node)
	{
		// greebo: We've traversed this subtree, now check if we had selected children
		if (!node->isRoot() &&
			!_stack.empty() && _stack.top() == false &&
			!Node_isSelected(node))
		{
			// No selected child nodes, hide this node
			hideSubgraph(node, _hide);
		}

		// Go upwards again, one level
		_stack.pop();
	}
};

void hideDeselected(const cmd::ArgumentList& args) {
	HideDeselectedWalker walker(true);
	GlobalSceneGraph().root()->traverse(walker);

	// Hide all components, there might be faces selected
	GlobalSelectionSystem().setSelectedAllComponents(false);

	SceneChangeNotify();
}

class HideAllWalker :
	public scene::NodeVisitor
{
	bool _hide;
public:
	HideAllWalker(bool hide) :
		_hide(hide)
	{}

	bool pre(const scene::INodePtr& node) {
		scene::setNodeHidden(node, _hide);
		return true;
	}
};

void showAllHidden(const cmd::ArgumentList& args) {
	HideAllWalker walker(false);
	GlobalSceneGraph().root()->traverse(walker);
	SceneChangeNotify();
}

/**
 * Invert Selection walker
 * Worldspawn brushes and patches will be considered, unless
 * entity selection mode is active.
 * An entity with or without any child nodes will be considered.
 * The worldspawn node itself will be ignored.
 * func_static children will be ignored.
 */
class InvertSelectionWalker :
	public scene::NodeVisitor
{
private:
	SelectionSystem::EMode _mode;

public:
	InvertSelectionWalker(SelectionSystem::EMode mode) :
		_mode(mode)
	{}

	bool pre(const scene::INodePtr& node)
	{
		// Ignore hidden nodes (including their whole subgraph)
		if (!node->visible()) return false;

		Entity* entity = Node_getEntity(node);

		// Check if we have a selectable
		ISelectablePtr selectable = scene::node_cast<ISelectable>(node);

		if (selectable)
		{
			switch (_mode)
			{
				case SelectionSystem::eEntity:
					// Only consider non-worldspawn entities
					if (entity != nullptr && !entity->isWorldspawn())
					{
						selectable->setSelected(!selectable->isSelected());
					}
					break;
				case SelectionSystem::ePrimitive:
					// Never select the worldspawn entity
					if (entity == nullptr || !entity->isWorldspawn())
					{
						selectable->setSelected(!selectable->isSelected());
					}
					break;
				case SelectionSystem::eGroupPart:
					// All child primitives can be selected (worldspawn entity is filtered out)
					if (entity == nullptr && Node_isEntity(node->getParent()))
					{
						selectable->setSelected(!selectable->isSelected());
					}
					break;
				case SelectionSystem::eComponent:
					break; // not handled here
			}
		}

		// Determine whether we want to traverse child objects of entities
		if (entity != nullptr)
		{
			if (_mode == SelectionSystem::eGroupPart)
			{
				// In group part mode, we want to traverse all entities but worldspawn
				return !entity->isWorldspawn();
			}
			else if (_mode == SelectionSystem::eEntity)
			{
				// In Entity selection mode, we don't need to traverse entity subgraphs
				// as we only want the parent entity
				return false;
			}

			// In primitive mode, we want to traverse worldspawn only
			return entity->isWorldspawn();
		}

		// We can traverse all non-entities by default
		return true;
	}
};

class InvertComponentSelectionWalker :
	public scene::NodeVisitor
{
    selection::ComponentSelectionMode _mode;
	ComponentSelectionTestablePtr _selectable;
public:
	InvertComponentSelectionWalker(selection::ComponentSelectionMode mode) :
		_mode(mode)
	{}

	bool pre(const scene::INodePtr& node)
	{
		// Ignore hidden nodes
		if (!node->visible()) return false;

		Entity* entity = Node_getEntity(node);

		// Check if we have a selectable
		ISelectablePtr selectable = scene::node_cast<ISelectable>(node);

		if (selectable != NULL)
		{
			// Check if we have a componentselectiontestable instance
			ComponentSelectionTestablePtr compSelTestable =
				Node_getComponentSelectionTestable(node);

			// Only add it to the list if the instance has components and is already selected
			if (compSelTestable && selectable->isSelected())
			{
				_selectable = compSelTestable;
			}
		}

		// Do we have a groupnode? If yes, don't traverse the children
		if (entity != NULL && scene::hasChildPrimitives(node) && !entity->isWorldspawn())
		{
			// Don't traverse the children of this groupnode
			return false;
		}

		return true;
	}

	void post(const scene::INodePtr& node)
	{
		if (_selectable)
		{
			_selectable->invertSelectedComponents(_mode);
			_selectable.reset();
		}
	}
};

void invertSelection(const cmd::ArgumentList& args)
{
	if (GlobalSelectionSystem().Mode() == SelectionSystem::eComponent)
	{
		InvertComponentSelectionWalker walker(GlobalSelectionSystem().ComponentMode());
		GlobalSceneGraph().root()->traverse(walker);
	}
	else
	{
		InvertSelectionWalker walker(GlobalSelectionSystem().Mode());
		GlobalSceneGraph().root()->traverse(walker);
	}
}

void deleteSelection()
{
	std::set<scene::INodePtr> eraseList;

	// Traverse the scene, collecting all selected nodes
	GlobalSelectionSystem().foreachSelected([&] (const scene::INodePtr& node)
	{
		// Check for selected nodes whose parent is not NULL and are not root
		if (node->getParent() != NULL && !node->isRoot())
        {
			// Found a candidate
			eraseList.insert(node);
		}
	});

	std::for_each(eraseList.begin(), eraseList.end(), [] (const scene::INodePtr& node)
	{
		scene::INodePtr parent = node->getParent();

        // Check for NULL parents. It's possible that both parent and child are in the eraseList
        // and the parent has been deleted already.
        if (parent)
        {
            // Remove the childnodes
            scene::removeNodeFromParent(node);

            if (!parent->hasChildNodes())
            {
                // Remove the parent as well
                scene::removeNodeFromParent(parent);
            }
        }
	});

	SceneChangeNotify();
}

void deleteSelectionCmd(const cmd::ArgumentList& args)
{
	UndoableCommand undo("deleteSelected");
	deleteSelection();
}

/**
 * Selects all objects that intersect one of the bounding AABBs.
 * The exact intersection-method is specified through TSelectionPolicy,
 * which must implement an evalute() method taking an AABB and the scene::INodePtr.
 */
template<class TSelectionPolicy>
class SelectByBounds :
	public scene::NodeVisitor
{
    const std::vector<AABB>& _aabbs;	// selection aabbs
	TSelectionPolicy policy;	// type that contains a custom intersection method aabb<->aabb

public:
	SelectByBounds(const std::vector<AABB>& aabbs) :
		_aabbs(aabbs)
	{}

	bool pre(const scene::INodePtr& node) override
    {
		// Don't traverse hidden nodes
		if (!node->visible()) return false;

		ISelectablePtr selectable = scene::node_cast<ISelectable>(node);

		// ignore worldspawn
        Entity* entity = Node_getEntity(node);

		if (entity != NULL && entity->isWorldspawn())
        {
			return true;
		}

    	bool selected = false;

		if (selectable && node->getParent() && !node->isRoot())
        {
			for (const auto& aabb : _aabbs)
            {
				// Check if the selectable passes the AABB test
				if (policy.evaluate(aabb, node))
                {
					selectable->setSelected(true);
					selected = true;
					break;
				}
			}
		}

		// Only traverse the children of this node, if the node itself couldn't be selected
		return !selected;
	}

	/**
	 * Performs selection operation on the global scenegraph.
	 * If delete_bounds_src is true, then the objects which were
	 * used as source for the selection aabbs will be deleted.
	 */
	static void DoSelection(bool deleteBoundsSrc = true)
	{
		if (GlobalSelectionSystem().Mode() != SelectionSystem::ePrimitive)
		{
			return; // Wrong selection mode
		}

		// Loops over all selected brushes and stores their
		// world AABBs in the specified array.
        std::vector<AABB> aabbs;

		GlobalSelectionSystem().foreachSelected([&] (const scene::INodePtr& node)
		{
			if (Node_isSelected(node) && Node_isBrush(node))
			{
				aabbs.push_back(node->worldAABB());
			}
		});

		// nothing usable in selection
		if (aabbs.empty())
		{
			return;
		}

		// delete selected objects?
		if (deleteBoundsSrc)
		{
			UndoableCommand undo("deleteSelected");
			deleteSelection();
		}

        DoSelection(aabbs);
	}

    static void DoSelection(const std::vector<AABB>& aabbs)
    {
        SelectByBounds<TSelectionPolicy> walker(aabbs);
        GlobalSceneGraph().root()->traverse(walker);

        SceneChangeNotify();
    }

    static void DoSelection(const AABB& bounds)
    {
        DoSelection(std::vector<AABB>{ bounds });
    }
};

void selectInside(const cmd::ArgumentList& args)
{
    if (args.size() == 2)
    {
        auto bounds = AABB::createFromMinMax(args[0].getVector3(), args[1].getVector3());
        SelectByBounds<SelectionPolicy_Inside>::DoSelection(bounds);
        return;
    }

	SelectByBounds<SelectionPolicy_Inside>::DoSelection();
}

void selectFullyInside(const cmd::ArgumentList& args)
{
    if (args.size() == 2)
    {
        auto bounds = AABB::createFromMinMax(args[0].getVector3(), args[1].getVector3());
        SelectByBounds<SelectionPolicy_FullyInside>::DoSelection(bounds);
        return;
    }

    SelectByBounds<SelectionPolicy_FullyInside>::DoSelection();
}

void selectTouching(const cmd::ArgumentList& args)
{
    if (args.size() == 2)
    {
        auto bounds = AABB::createFromMinMax(args[0].getVector3(), args[1].getVector3());
        SelectByBounds<SelectionPolicy_Touching>::DoSelection(bounds);
        return;
    }

	SelectByBounds<SelectionPolicy_Touching>::DoSelection(false);
}

void selectCompleteTall(const cmd::ArgumentList& args)
{
    if (args.size() == 2)
    {
        auto bounds = AABB::createFromMinMax(args[0].getVector3(), args[1].getVector3());
        SelectByBounds<SelectionPolicy_Complete_Tall>::DoSelection(bounds);
        return;
    }

	SelectByBounds<SelectionPolicy_Complete_Tall>::DoSelection();
}

AABB getCurrentComponentSelectionBounds()
{
	AABB bounds;

	GlobalSelectionSystem().foreachSelected([&] (const scene::INodePtr& node)
	{
		ComponentEditablePtr componentEditable = Node_getComponentEditable(node);

		if (componentEditable != NULL)
		{
			bounds.includeAABB(AABB::createFromOrientedAABBSafe(
				componentEditable->getSelectedComponentsBounds(), node->localToWorld()));
		}
	});

	return bounds;
}

// This is the same as for selection
AABB getCurrentSelectionBounds(bool considerLightVolumes)
{
	AABB bounds;

	GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
	{
		if (considerLightVolumes)
		{
			bounds.includeAABB(node->worldAABB());
			return;
		}

		// We were asked to ignore light volumes, so for lights we'll only
		// sum up the small diamond AABB to calculate the selection bounds (#4578)
		ILightNodePtr lightNode = Node_getLightNode(node);

		if (lightNode)
		{
			bounds.includeAABB(lightNode->getSelectAABB());
		}
		else
		{
			bounds.includeAABB(node->worldAABB());
		}
	});

	return bounds;
}

AABB getCurrentSelectionBounds()
{
	// Consider light volumes by default
	return getCurrentSelectionBounds(true);
}

// Snap vector components to whole numbers
Vector3 snapToInt(const Vector3& v)
{
    return Vector3(float_to_integer(v.x()), float_to_integer(v.y()),
                   float_to_integer(v.z()));
}

Vector3 getCurrentSelectionCenter()
{
	return snapToInt(getCurrentSelectionBounds().getOrigin());
}

void snapSelectionToGrid(const cmd::ArgumentList& args)
{
    // Send out the event in case other views want that event
    GridSnapRequest request;
    GlobalRadiantCore().getMessageBus().sendMessage(request);

    if (request.isHandled())
    {
        return; // done here
    }

	auto gridSize = GlobalGrid().getGridSize();
	UndoableCommand undo("snapSelected -grid " + string::to_string(gridSize));

	if (GlobalSelectionSystem().Mode() == SelectionSystem::eComponent)
	{
		// Component mode
		GlobalSelectionSystem().foreachSelectedComponent([&] (const scene::INodePtr& node)
		{
			// Don't do anything with hidden nodes
			if (!node->visible()) return;

    		// Check if the visited instance is componentSnappable
			ComponentSnappablePtr componentSnappable = Node_getComponentSnappable(node);

			if (componentSnappable)
			{
				componentSnappable->snapComponents(gridSize);
			}
		});
	}
	else
	{
		// Non-component mode
		GlobalSelectionSystem().foreachSelected([&] (const scene::INodePtr& node)
		{
			// Don't do anything with hidden nodes
			if (!node->visible()) return;

			SnappablePtr snappable = Node_getSnappable(node);

			if (snappable)
			{
				snappable->snapto(gridSize);
			}
		});
	}
}

class IntersectionFinder :
	public scene::NodeVisitor
{
private:
	const Ray& _ray;

	Vector3 _bestPoint;

	const scene::INodePtr& _self;

public:
	IntersectionFinder(const Ray& ray, const scene::INodePtr& self) :
		_ray(ray),
		_bestPoint(_ray.origin),
		_self(self)
	{}

	const Vector3& getIntersection() const
	{
		return _bestPoint;
	}

	bool pre(const scene::INodePtr& node)
	{
		if (node == _self) return false;
		if (!node->visible()) return true;

		const AABB& aabb = node->worldAABB();
		Vector3 intersection;

		if (_ray.intersectAABB(aabb, intersection))
		{
			rMessage() << "Ray intersects with node " << node->name() << " at " << intersection;

			// We have an intersection, let's attempt a full trace against the object
			ITraceablePtr traceable = std::dynamic_pointer_cast<ITraceable>(node);

			if (traceable && traceable->getIntersection(_ray, intersection))
			{
				rMessage() << " impacting at " << intersection;
			}
			else
			{
				// rMessage() << " (no detailed intersection)";
				return true; // ignore this node
			}

			auto oldDistSquared = (_bestPoint - _ray.origin).getLengthSquared();
			auto newDistSquared = (intersection - _ray.origin).getLengthSquared();

			if ((oldDistSquared == 0 && newDistSquared > 0) || newDistSquared < oldDistSquared)
			{
				_bestPoint = intersection;
			}

			rMessage() << std::endl;
		}

		return true;
	}
};

Vector3 getLowestVertexOfModel(const model::IModel& model, const Matrix4& localToWorld)
{
	Vector3 bestValue = Vector3(0,0,1e16);

	for (int surfaceIndex = 0; surfaceIndex < model.getSurfaceCount(); ++surfaceIndex)
	{
		const model::IModelSurface& surface = model.getSurface(surfaceIndex);

		for (int v = 0; v < surface.getNumVertices(); ++v)
		{
			Vector3 candidate = localToWorld.transformPoint(surface.getVertex(v).vertex);

			if (candidate.z() < bestValue.z())
			{
				bestValue = candidate;
			}
		}
	}

	return bestValue;
}

class ChildModelFinder :
	public scene::NodeVisitor
{
	model::ModelNodePtr _modelNode;
public:
	const model::ModelNodePtr& getModelNode() const
	{
		return _modelNode;
	}

	bool pre(const scene::INodePtr& node)
	{
		model::ModelNodePtr modelNode = Node_getModel(node);

		if (modelNode != NULL)
		{
			_modelNode = modelNode;
			return false;
		}

		return true;
	}
};

Vector3 getOriginForFloorTrace(const scene::INodePtr& node)
{
	Vector3 origin = node->worldAABB().getOrigin();

	Entity* entity = Node_getEntity(node);

	if (entity != NULL)
	{
		// Next iteration: use the entity's origin
		origin = string::convert<Vector3>(entity->getKeyValue("origin"));

		// Finally, find the lowest vertex (with regard to the z direction) of models
		ChildModelFinder modelFinder;
		node->traverseChildren(modelFinder);

		if (modelFinder.getModelNode())
		{
			origin = getLowestVertexOfModel(modelFinder.getModelNode()->getIModel(), node->localToWorld());
		}
	}

	return origin;
}

void floorNode(const scene::INodePtr& node)
{
	Vector3 objectOrigin = getOriginForFloorTrace(node);

	// Move up the trace start point by 1 unit to avoid falling further down
	// when hitting "floor" multiple times in a row
	Ray ray(objectOrigin + Vector3(0, 0, 1), Vector3(0, 0, -1));

	IntersectionFinder finder(ray, node);
	GlobalSceneGraph().root()->traverseChildren(finder);

	if ((finder.getIntersection() - ray.origin).getLengthSquared() > 0)
	{
		Vector3 translation = finder.getIntersection() - objectOrigin;

		ITransformablePtr transformable = scene::node_cast<ITransformable>(node);

		if (transformable)
		{
    		transformable->setType(TRANSFORM_PRIMITIVE);
    		transformable->setTranslation(translation);
			transformable->freezeTransform();
		}
	}
	else
	{
		rMessage() << "No suitable floor points found." << std::endl;
	}
}

void floorSelection(const cmd::ArgumentList& args)
{
	UndoableCommand undo("floorSelected");

	GlobalSelectionSystem().foreachSelected([] (const scene::INodePtr& node)
	{
		floorNode(node);
	});
}

void registerCommands()
{
    GlobalCommandSystem().addCommand("CloneSelection", cloneSelected);
    GlobalCommandSystem().addCommand("DeleteSelection", deleteSelectionCmd);
    GlobalCommandSystem().addCommand("ParentSelection", parentSelection);
    GlobalCommandSystem().addCommand("ParentSelectionToWorldspawn", parentSelectionToWorldspawn);

    GlobalCommandSystem().addCommand("InvertSelection", invertSelection);
    GlobalCommandSystem().addCommand("SelectInside", selectInside,
        { cmd::ARGTYPE_VECTOR3 | cmd::ARGTYPE_OPTIONAL, cmd::ARGTYPE_VECTOR3 | cmd::ARGTYPE_OPTIONAL });
    GlobalCommandSystem().addCommand("SelectFullyInside", selectFullyInside,
        { cmd::ARGTYPE_VECTOR3 | cmd::ARGTYPE_OPTIONAL, cmd::ARGTYPE_VECTOR3 | cmd::ARGTYPE_OPTIONAL });
	GlobalCommandSystem().addCommand("SelectTouching", selectTouching,
        { cmd::ARGTYPE_VECTOR3 | cmd::ARGTYPE_OPTIONAL, cmd::ARGTYPE_VECTOR3 | cmd::ARGTYPE_OPTIONAL });
	GlobalCommandSystem().addCommand("SelectCompleteTall", selectCompleteTall,
        { cmd::ARGTYPE_VECTOR3 | cmd::ARGTYPE_OPTIONAL, cmd::ARGTYPE_VECTOR3 | cmd::ARGTYPE_OPTIONAL });
	GlobalCommandSystem().addCommand("ExpandSelectionToSiblings", expandSelectionToSiblings);
	GlobalCommandSystem().addCommand("SelectParentEntities", selectParentEntitiesOfSelected);
	GlobalCommandSystem().addCommand("MergeSelectedEntities", mergeSelectedEntities);
	GlobalCommandSystem().addCommand("SelectChildren", selectChildren);

	GlobalCommandSystem().addCommand("ShowHidden", showAllHidden);
	GlobalCommandSystem().addCommand("HideSelected", hideSelected);
	GlobalCommandSystem().addCommand("HideDeselected", hideDeselected);

	GlobalCommandSystem().addCommand("MirrorSelectionX", mirrorSelectionX);
	GlobalCommandSystem().addCommand("RotateSelectionX", rotateSelectionX);
	GlobalCommandSystem().addCommand("MirrorSelectionY", mirrorSelectionY);
	GlobalCommandSystem().addCommand("RotateSelectionY", rotateSelectionY);
	GlobalCommandSystem().addCommand("MirrorSelectionZ", mirrorSelectionZ);
	GlobalCommandSystem().addCommand("RotateSelectionZ", rotateSelectionZ);

	GlobalCommandSystem().addCommand("ConvertSelectedToFuncStatic", convertSelectedToFuncStatic);
	GlobalCommandSystem().addCommand("RevertToWorldspawn", revertGroupToWorldSpawn);

	GlobalCommandSystem().addCommand("SnapToGrid", snapSelectionToGrid);

	GlobalCommandSystem().addCommand("SelectAllOfType", selectAllOfType);
	GlobalCommandSystem().addCommand("GroupCycleForward", selection::GroupCycle::cycleForward);
	GlobalCommandSystem().addCommand("GroupCycleBackward", selection::GroupCycle::cycleBackward);

	GlobalCommandSystem().addCommand("TexRotate", rotateTexture, { cmd::ARGTYPE_INT | cmd::ARGTYPE_STRING });
	GlobalCommandSystem().addCommand("TexScale", scaleTexture, { cmd::ARGTYPE_VECTOR2 | cmd::ARGTYPE_STRING });
	GlobalCommandSystem().addCommand("TexShift", shiftTextureCmd, { cmd::ARGTYPE_VECTOR2 | cmd::ARGTYPE_STRING });

	GlobalCommandSystem().addCommand("TexAlign", alignTextureCmd, { cmd::ARGTYPE_STRING });
	GlobalCommandSystem().addCommand("FitTexture", fitTextureCmd, { cmd::ARGTYPE_DOUBLE, cmd::ARGTYPE_DOUBLE });

	// Add the nudge commands (one general, four specialised ones)
	GlobalCommandSystem().addCommand("NudgeSelected", nudgeSelectedCmd, { cmd::ARGTYPE_STRING });

	GlobalCommandSystem().addCommand("NormaliseTexture", normaliseTexture);

	GlobalCommandSystem().addCommand("CopyShader", pickShaderFromSelection);
	GlobalCommandSystem().addCommand("PasteShader", pasteShaderToSelection);
	GlobalCommandSystem().addCommand("PasteShaderNatural", pasteShaderNaturalToSelection);
	GlobalCommandSystem().addCommand("SetShaderOnSelection", applyShaderToSelectionCmd, { cmd::ARGTYPE_STRING });

	GlobalCommandSystem().addCommand("SelectItemsByShader", selectItemsByShaderCmd, { cmd::ARGTYPE_STRING });
	GlobalCommandSystem().addCommand("DeselectItemsByShader", deselectItemsByShaderCmd, { cmd::ARGTYPE_STRING });

    GlobalCommandSystem().addCommand("SelectItemsByModel", selectItemsByModelCmd, { cmd::ARGTYPE_STRING });
    GlobalCommandSystem().addCommand("DeselectItemsByModel", deselectItemsByModelCmd, { cmd::ARGTYPE_STRING });

	GlobalCommandSystem().addCommand("FlipTextureX", flipTextureS);
	GlobalCommandSystem().addCommand("FlipTextureY", flipTextureT);

	GlobalCommandSystem().addCommand("MoveSelectionVertically", moveSelectedVerticallyCmd, { cmd::ARGTYPE_STRING });
    GlobalCommandSystem().addCommand("MoveSelection", moveSelectedCmd, { cmd::ARGTYPE_VECTOR3 });

	GlobalCommandSystem().addCommand("CurveAppendControlPoint", appendCurveControlPoint);
	GlobalCommandSystem().addCommand("CurveRemoveControlPoint", removeCurveControlPoints);
	GlobalCommandSystem().addCommand("CurveInsertControlPoint", insertCurveControlPoints);
	GlobalCommandSystem().addCommand("CurveConvertType", convertCurveTypes);

	GlobalCommandSystem().addCommand("ExportSelectedAsCollisionModel", createCMFromSelection, { cmd::ARGTYPE_STRING });

	GlobalCommandSystem().addCommand("CreateDecalsForFaces", createDecalsForSelectedFaces);

	GlobalCommandSystem().addCommand("Copy", selection::clipboard::copy);
	GlobalCommandSystem().addCommand("Paste", selection::clipboard::paste);
	GlobalCommandSystem().addCommand("PasteToCamera", selection::clipboard::pasteToCamera);

	GlobalCommandSystem().addCommand("ConnectSelection", connectSelectedEntities);
    GlobalCommandSystem().addCommand("BindSelection", bindEntities);
    GlobalCommandSystem().addCommand("PlacePlayerStart", placePlayerStart, { cmd::ARGTYPE_VECTOR3 });
	GlobalCommandSystem().addCommand("SetEntityKeyValue", setEntityKeyValueOnSelection, { cmd::ARGTYPE_STRING, cmd::ARGTYPE_STRING });
    GlobalCommandSystem().addCommand("CreateCurveNURBS", createCurveNURBS);
    GlobalCommandSystem().addCommand("CreateCurveCatmullRom", createCurveCatmullRom);

	GlobalCommandSystem().addCommand("FloorSelection", floorSelection);
	GlobalCommandSystem().addCommand("BrushSetDetailFlag", brushSetDetailFlag, { cmd::ARGTYPE_STRING });
	GlobalCommandSystem().addStatement("BrushMakeDetail", "BrushSetDetailFlag detail", false);
	GlobalCommandSystem().addStatement("BrushMakeStructural", "BrushSetDetailFlag structural", false);

	GlobalCommandSystem().addCommand(scene::SELECT_NODE_BY_INDEX_CMD, scene::selectNodeByIndexCmd,
		{ cmd::ARGTYPE_INT, cmd::ARGTYPE_INT });
}

	} // namespace algorithm
} // namespace selection
