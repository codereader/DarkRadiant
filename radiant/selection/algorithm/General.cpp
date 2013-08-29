#include "General.h"

#include <stack>
#include "iselection.h"
#include "iundo.h"
#include "scenelib.h"
#include "iselectiontest.h"

#include "map/Map.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
#include "ui/texturebrowser/TextureBrowser.h"

#include "SelectionPolicies.h"
#include "selection/SceneWalkers.h"
#include "selection/algorithm/Primitives.h"
#include "selection/algorithm/Transformation.h"
#include "selection/algorithm/Group.h"
#include "selection/clipboard/Clipboard.h"
#include "selection/algorithm/Curves.h"
#include "selection/algorithm/Entity.h"
#include "selection/algorithm/GroupCycle.h"
#include "brush/BrushVisit.h"
#include "patch/PatchSceneWalk.h"
#include "patch/Patch.h"
#include "patch/PatchNode.h"

#include <boost/scoped_array.hpp>

namespace selection
{

namespace algorithm
{

namespace
{
    const char* const RKEY_FREE_MODEL_ROTATION = "user/ui/freeModelRotation";
}

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
		forEachSelectedFaceComponent([&] (Face& face)
		{
			shaders.insert(face.getShader());
		});

		// fall back to the one selected in the texture browser
		if (shaders.empty())
		{
			shaders.insert(GlobalTextureBrowser().getSelectedShader());
		}

		// Deselect all faces
		GlobalSelectionSystem().setSelectedAllComponents(false);

		// Select all faces carrying any of the shaders in the set
		scene::foreachVisibleFaceInstance([&] (FaceInstance& instance)
		{
			if (shaders.find(instance.getFace().getShader()) != shaders.end())
			{
				instance.setSelected(SelectionSystem::eFace, true);
			} 
		});

		// Select all visible patches carrying any of the shaders in the set
		scene::foreachVisiblePatch([&] (Patch& patch)
		{
			if (shaders.find(patch.getShader()) != shaders.end())
			{
				patch.getPatchNode().setSelected(true);
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
			Node_traverseSubgraph(GlobalSceneGraph().root(), classnameSelector);
		}
		else
		{
			// No entities found, select all elements with textures 
			// matching the one in the texture browser
			const std::string& shader = GlobalTextureBrowser().getSelectedShader();

			scene::foreachVisibleBrush([&] (Brush& brush)
			{
				if (brush.hasShader(shader))
				{
					brush.getBrushNode().setSelected(true);
				} 
			});

			// Select all visible patches carrying any of the shaders in the set
			scene::foreachVisiblePatch([&] (Patch& patch)
			{
				if (patch.getShader() == shader)
				{
					patch.getPatchNode().setSelected(true);
				} 
			});
		}
	}

	SceneChangeNotify();
}

class HideSubgraphWalker :
	public scene::NodeVisitor
{
public:
	bool pre(const scene::INodePtr& node)
	{
		node->enable(scene::Node::eHidden);
		return true;
	}
};

class ShowSubgraphWalker :
	public scene::NodeVisitor
{
public:
	bool pre(const scene::INodePtr& node)
	{
		node->disable(scene::Node::eHidden);
		return true;
	}
};

inline void hideSubgraph(const scene::INodePtr& node, bool hide)
{
	if (hide)
	{
		HideSubgraphWalker walker;
		Node_traverseSubgraph(node, walker);
	}
	else
	{
		ShowSubgraphWalker walker;
		Node_traverseSubgraph(node, walker);
	}
}

inline void hideNode(const scene::INodePtr& node, bool hide)
{
	if (hide)
	{
		node->enable(scene::Node::eHidden);
	}
	else
	{
		node->disable(scene::Node::eHidden);
	}
}

void hideSelected(const cmd::ArgumentList& args)
{
	// Traverse the selection, hiding all nodes
	GlobalSelectionSystem().foreachSelected([] (const scene::INodePtr& node) 
	{
		hideSubgraph(node, true);
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
	Node_traverseSubgraph(GlobalSceneGraph().root(), walker);

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
		hideNode(node, _hide);
		return true;
	}
};

void showAllHidden(const cmd::ArgumentList& args) {
	HideAllWalker walker(false);
	Node_traverseSubgraph(GlobalSceneGraph().root(), walker);
	SceneChangeNotify();
}

class InvertSelectionWalker :
	public scene::NodeVisitor
{
	SelectionSystem::EMode _mode;
	SelectablePtr _selectable;
public:
	InvertSelectionWalker(SelectionSystem::EMode mode) :
		_mode(mode)
	{}

	bool pre(const scene::INodePtr& node)
	{
		// Ignore hidden nodes
		if (!node->visible()) return false;

		Entity* entity = Node_getEntity(node);

		// Check if we have a selectable
		SelectablePtr selectable = Node_getSelectable(node);

		if (selectable != NULL)
		{
			switch (_mode)
			{
				case SelectionSystem::eEntity:
					if (entity != NULL && entity->getKeyValue("classname") != "worldspawn")
					{
						_selectable = selectable;
					}
					break;
				case SelectionSystem::ePrimitive:
					_selectable = selectable;
					break;
				case SelectionSystem::eComponent:
					// Check if we have a componentselectiontestable instance
					ComponentSelectionTestablePtr compSelTestable =
						Node_getComponentSelectionTestable(node);

					// Only add it to the list if the instance has components and is already selected
					if (compSelTestable != NULL && selectable->isSelected())
					{
						_selectable = selectable;
					}
					break;
			}
		}

		// Do we have a groupnode? If yes, don't traverse the children
		if (entity != NULL && node_is_group(node) &&
			entity->getKeyValue("classname") != "worldspawn")
		{
			// Don't traverse the children of this groupnode
			return false;
		}

		return true;
	}

	void post(const scene::INodePtr& node)
	{
		if (_selectable != NULL)
		{
			_selectable->invertSelected();
			_selectable = SelectablePtr();
		}
	}
};

void invertSelection(const cmd::ArgumentList& args) {
	InvertSelectionWalker walker(GlobalSelectionSystem().Mode());
	Node_traverseSubgraph(GlobalSceneGraph().root(), walker);
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

		// Remove the childnodes
		scene::removeNodeFromParent(node);

		if (!parent->hasChildNodes())
		{
			// Remove the parent as well
			scene::removeNodeFromParent(parent);
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
	AABB* _aabbs;				// selection aabbs
	std::size_t _count;			// number of aabbs in _aabbs
	TSelectionPolicy policy;	// type that contains a custom intersection method aabb<->aabb

public:
	SelectByBounds(AABB* aabbs, std::size_t count) :
		_aabbs(aabbs),
        _count(count)
	{}

	bool pre(const scene::INodePtr& node) {
		// Don't traverse hidden nodes
		if (!node->visible()) {
			return false;
		}

		SelectablePtr selectable = Node_getSelectable(node);

		// ignore worldspawn
		Entity* entity = Node_getEntity(node);
		if (entity != NULL) {
			if (entity->getKeyValue("classname") == "worldspawn") {
				return true;
			}
		}

    	bool selected = false;

		if (selectable != NULL && node->getParent() != NULL && !node->isRoot()) {
			for (std::size_t i = 0; i < _count; ++i) {
				// Check if the selectable passes the AABB test
				if (policy.evaluate(_aabbs[i], node)) {
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

		// we may not need all AABBs since not all selected objects have to be brushes
		const std::size_t max = GlobalSelectionSystem().countSelected();
		boost::scoped_array<AABB> aabbs(new AABB[max]);

		// Loops over all selected brushes and stores their
		// world AABBs in the specified array.
		std::size_t aabbCount = 0; // number of aabbs in aabbs

		GlobalSelectionSystem().foreachSelected([&] (const scene::INodePtr& node)
		{
			ASSERT_MESSAGE(aabbCount <= max, "Invalid _count in CollectSelectedBrushesBounds");

			// stop if the array is already full
			if (aabbCount == max) return;

			if (Node_isSelected(node) && Node_isBrush(node))
			{
				aabbs[aabbCount] = node->worldAABB();
				++aabbCount;
			}
		});

		// nothing usable in selection
		if (!aabbCount)
		{
			return;
		}

		// delete selected objects?
		if (deleteBoundsSrc)
		{
			UndoableCommand undo("deleteSelected");
			deleteSelection();
		}

		// Instantiate a "self" object SelectByBounds and use it as visitor
		SelectByBounds<TSelectionPolicy> walker(aabbs.get(), aabbCount);
		Node_traverseSubgraph(GlobalSceneGraph().root(), walker);

		SceneChangeNotify();
	}
};

void selectInside(const cmd::ArgumentList& args)
{
	SelectByBounds<SelectionPolicy_Inside>::DoSelection();
}

void selectTouching(const cmd::ArgumentList& args)
{
	SelectByBounds<SelectionPolicy_Touching>::DoSelection(false);
}

void selectCompleteTall(const cmd::ArgumentList& args)
{
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

AABB getCurrentSelectionBounds()
{
	AABB bounds;

	GlobalSelectionSystem().foreachSelected([&] (const scene::INodePtr& node)
	{
		bounds.includeAABB(Node_getPivotBounds(node));
	});

	return bounds;
}

Vector3 getCurrentSelectionCenter()
{
	return getCurrentSelectionBounds().getOrigin().getSnapped();
}

class PrimitiveFindIndexWalker :
	public scene::NodeVisitor
{
	scene::INodePtr _node;
	std::size_t& _count;
public:
	PrimitiveFindIndexWalker(const scene::INodePtr& node, std::size_t& count) :
		_node(node),
		_count(count)
	{}

	bool pre(const scene::INodePtr& node)
	{
		if (Node_isPrimitive(node))
		{
			// Have we found the node?
			if (_node == node)
			{
				// Yes, found, set needle to NULL
				_node = scene::INodePtr();
			}

			// As long as the needle is non-NULL, increment the counter
			if (_node != NULL)
			{
				++_count;
			}
		}

		return true;
	}
};

class EntityFindIndexWalker :
	public scene::NodeVisitor
{
	scene::INodePtr _node;
	std::size_t& _count;
public:
	EntityFindIndexWalker(const scene::INodePtr& node, std::size_t& count) :
		_node(node),
		_count(count)
	{}

	bool pre(const scene::INodePtr& node)
	{
		if (Node_isEntity(node))
		{
			// Have we found the node?
			if (_node == node)
			{
				// Yes, found, set needle to NULL
				_node = scene::INodePtr();
			}

			// As long as the needle is non-NULL, increment the counter
			if (_node != NULL)
			{
				++_count;
			}
		}

		return true;
	}
};

void getSelectionIndex(std::size_t& ent, std::size_t& brush)
{
	if (GlobalSelectionSystem().countSelected() != 0)
	{
		scene::INodePtr node = GlobalSelectionSystem().ultimateSelected();
		
		if (Node_isEntity(node))
		{
			// Selection is an entity, find its index
			EntityFindIndexWalker walker(node, ent);
			Node_traverseSubgraph(GlobalSceneGraph().root(), walker);
		}
		else if (Node_isPrimitive(node))
		{
			scene::INodePtr parent = node->getParent();

			// Node is a primitive, find parent entity and child index
			EntityFindIndexWalker walker(parent, ent);
			Node_traverseSubgraph(GlobalSceneGraph().root(), walker);

			PrimitiveFindIndexWalker brushWalker(node, brush);
			Node_traverseSubgraph(parent, brushWalker);
		}
	}
}

void snapSelectionToGrid(const cmd::ArgumentList& args)
{
	float gridSize = GlobalGrid().getGridSize();
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

			if (componentSnappable != NULL)
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

			if (snappable != NULL)
			{
				snappable->snapto(gridSize);
			}
		});
	}
}

void registerCommands()
{
	GlobalCommandSystem().addCommand("CloneSelection", selection::algorithm::cloneSelected);
	GlobalCommandSystem().addCommand("DeleteSelection", selection::algorithm::deleteSelectionCmd);
	GlobalCommandSystem().addCommand("ParentSelection", selection::algorithm::parentSelection);
	GlobalCommandSystem().addCommand("ParentSelectionToWorldspawn", selection::algorithm::parentSelectionToWorldspawn);

	GlobalCommandSystem().addCommand("InvertSelection", selection::algorithm::invertSelection);
	GlobalCommandSystem().addCommand("SelectInside", selection::algorithm::selectInside);
	GlobalCommandSystem().addCommand("SelectTouching", selection::algorithm::selectTouching);
	GlobalCommandSystem().addCommand("SelectCompleteTall", selection::algorithm::selectCompleteTall);
	GlobalCommandSystem().addCommand("ExpandSelectionToEntities", selection::algorithm::expandSelectionToEntities);
	GlobalCommandSystem().addCommand("MergeSelectedEntities", selection::algorithm::mergeSelectedEntities);
	GlobalCommandSystem().addCommand("SelectChildren", selection::algorithm::selectChildren);

	GlobalCommandSystem().addCommand("ShowHidden", selection::algorithm::showAllHidden);
	GlobalCommandSystem().addCommand("HideSelected", selection::algorithm::hideSelected);
	GlobalCommandSystem().addCommand("HideDeselected", selection::algorithm::hideDeselected);

	GlobalCommandSystem().addCommand("MirrorSelectionX", selection::algorithm::mirrorSelectionX);
	GlobalCommandSystem().addCommand("RotateSelectionX", selection::algorithm::rotateSelectionX);
	GlobalCommandSystem().addCommand("MirrorSelectionY", selection::algorithm::mirrorSelectionY);
	GlobalCommandSystem().addCommand("RotateSelectionY", selection::algorithm::rotateSelectionY);
	GlobalCommandSystem().addCommand("MirrorSelectionZ", selection::algorithm::mirrorSelectionZ);
	GlobalCommandSystem().addCommand("RotateSelectionZ", selection::algorithm::rotateSelectionZ);

	GlobalCommandSystem().addCommand("ConvertSelectedToFuncStatic", selection::algorithm::convertSelectedToFuncStatic);
	GlobalCommandSystem().addCommand("RevertToWorldspawn", selection::algorithm::revertGroupToWorldSpawn);

	GlobalCommandSystem().addCommand("SnapToGrid", selection::algorithm::snapSelectionToGrid);

	GlobalCommandSystem().addCommand("SelectAllOfType", selection::algorithm::selectAllOfType);
	GlobalCommandSystem().addCommand("GroupCycleForward", selection::GroupCycle::cycleForward);
	GlobalCommandSystem().addCommand("GroupCycleBackward", selection::GroupCycle::cycleBackward);

	GlobalCommandSystem().addCommand("TexRotate", selection::algorithm::rotateTexture, cmd::ARGTYPE_INT|cmd::ARGTYPE_STRING);
	GlobalCommandSystem().addCommand("TexScale", selection::algorithm::scaleTexture, cmd::ARGTYPE_VECTOR2|cmd::ARGTYPE_STRING);
	GlobalCommandSystem().addCommand("TexShift", selection::algorithm::shiftTextureCmd, cmd::ARGTYPE_VECTOR2|cmd::ARGTYPE_STRING);

	GlobalCommandSystem().addCommand("TexAlign", selection::algorithm::alignTextureCmd, cmd::ARGTYPE_STRING);

	// Add the nudge commands (one general, four specialised ones)
	GlobalCommandSystem().addCommand("NudgeSelected", selection::algorithm::nudgeSelectedCmd, cmd::ARGTYPE_STRING);

	GlobalCommandSystem().addCommand("NormaliseTexture", selection::algorithm::normaliseTexture);

	GlobalCommandSystem().addCommand("CopyShader", selection::algorithm::pickShaderFromSelection);
	GlobalCommandSystem().addCommand("PasteShader", selection::algorithm::pasteShaderToSelection);
	GlobalCommandSystem().addCommand("PasteShaderNatural", selection::algorithm::pasteShaderNaturalToSelection);

	GlobalCommandSystem().addCommand("FlipTextureX", selection::algorithm::flipTextureS);
	GlobalCommandSystem().addCommand("FlipTextureY", selection::algorithm::flipTextureT);

	GlobalCommandSystem().addCommand("MoveSelectionVertically", selection::algorithm::moveSelectedCmd, cmd::ARGTYPE_STRING);
	
	GlobalCommandSystem().addCommand("CurveAppendControlPoint", selection::algorithm::appendCurveControlPoint);
	GlobalCommandSystem().addCommand("CurveRemoveControlPoint", selection::algorithm::removeCurveControlPoints);
	GlobalCommandSystem().addCommand("CurveInsertControlPoint", selection::algorithm::insertCurveControlPoints);
	GlobalCommandSystem().addCommand("CurveConvertType", selection::algorithm::convertCurveTypes);

	GlobalCommandSystem().addCommand("BrushExportCM", selection::algorithm::createCMFromSelection);

	GlobalCommandSystem().addCommand("CreateDecalsForFaces", selection::algorithm::createDecalsForSelectedFaces);

	GlobalCommandSystem().addCommand("Copy", selection::clipboard::copy);
	GlobalCommandSystem().addCommand("Paste", selection::clipboard::paste);
	GlobalCommandSystem().addCommand("PasteToCamera", selection::clipboard::pasteToCamera);

	GlobalCommandSystem().addCommand("ConnectSelection", selection::algorithm::connectSelectedEntities);
    GlobalCommandSystem().addCommand("BindSelection", selection::algorithm::bindEntities);
    GlobalCommandSystem().addCommand("CreateCurveNURBS", selection::algorithm::createCurveNURBS);
    GlobalCommandSystem().addCommand("CreateCurveCatmullRom", selection::algorithm::createCurveCatmullRom);

	GlobalEventManager().addCommand("CloneSelection", "CloneSelection", true); // react on keyUp
	GlobalEventManager().addCommand("DeleteSelection", "DeleteSelection");
	GlobalEventManager().addCommand("ParentSelection", "ParentSelection");
	GlobalEventManager().addCommand("ParentSelectionToWorldspawn", "ParentSelectionToWorldspawn");

	GlobalEventManager().addCommand("InvertSelection", "InvertSelection");
	GlobalEventManager().addCommand("SelectInside", "SelectInside");
	GlobalEventManager().addCommand("SelectTouching", "SelectTouching");
	GlobalEventManager().addCommand("SelectCompleteTall", "SelectCompleteTall");
	GlobalEventManager().addCommand("ExpandSelectionToEntities", "ExpandSelectionToEntities");
	GlobalEventManager().addCommand("MergeSelectedEntities", "MergeSelectedEntities");
	GlobalEventManager().addCommand("SelectChildren", "SelectChildren");

	GlobalEventManager().addCommand("ShowHidden", "ShowHidden");
	GlobalEventManager().addCommand("HideSelected", "HideSelected");
	GlobalEventManager().addCommand("HideDeselected", "HideDeselected");

	GlobalEventManager().addCommand("MirrorSelectionX", "MirrorSelectionX");
	GlobalEventManager().addCommand("RotateSelectionX", "RotateSelectionX");
	GlobalEventManager().addCommand("MirrorSelectionY", "MirrorSelectionY");
	GlobalEventManager().addCommand("RotateSelectionY", "RotateSelectionY");
	GlobalEventManager().addCommand("MirrorSelectionZ", "MirrorSelectionZ");
	GlobalEventManager().addCommand("RotateSelectionZ", "RotateSelectionZ");

	GlobalEventManager().addCommand("ConvertSelectedToFuncStatic", "ConvertSelectedToFuncStatic");
	GlobalEventManager().addCommand("RevertToWorldspawn", "RevertToWorldspawn");

	GlobalEventManager().addCommand("SnapToGrid", "SnapToGrid");

	GlobalEventManager().addCommand("SelectAllOfType", "SelectAllOfType");
	GlobalEventManager().addCommand("GroupCycleForward", "GroupCycleForward");
	GlobalEventManager().addCommand("GroupCycleBackward", "GroupCycleBackward");

	GlobalEventManager().addCommand("TexRotateClock", "TexRotateClock");
	GlobalEventManager().addCommand("TexRotateCounter", "TexRotateCounter");
	GlobalEventManager().addCommand("TexScaleUp", "TexScaleUp");
	GlobalEventManager().addCommand("TexScaleDown", "TexScaleDown");
	GlobalEventManager().addCommand("TexScaleLeft", "TexScaleLeft");
	GlobalEventManager().addCommand("TexScaleRight", "TexScaleRight");
	GlobalEventManager().addCommand("TexShiftUp", "TexShiftUp");
	GlobalEventManager().addCommand("TexShiftDown", "TexShiftDown");
	GlobalEventManager().addCommand("TexShiftLeft", "TexShiftLeft");
	GlobalEventManager().addCommand("TexShiftRight", "TexShiftRight");
	GlobalEventManager().addCommand("TexAlignTop", "TexAlignTop");
	GlobalEventManager().addCommand("TexAlignBottom", "TexAlignBottom");
	GlobalEventManager().addCommand("TexAlignLeft", "TexAlignLeft");
	GlobalEventManager().addCommand("TexAlignRight", "TexAlignRight");

	GlobalEventManager().addCommand("NormaliseTexture", "NormaliseTexture");

	GlobalEventManager().addCommand("CopyShader", "CopyShader");
	GlobalEventManager().addCommand("PasteShader", "PasteShader");
	GlobalEventManager().addCommand("PasteShaderNatural", "PasteShaderNatural");

	GlobalEventManager().addCommand("FlipTextureX", "FlipTextureX");
	GlobalEventManager().addCommand("FlipTextureY", "FlipTextureY");

	GlobalEventManager().addCommand("MoveSelectionDOWN", "MoveSelectionDOWN");
	GlobalEventManager().addCommand("MoveSelectionUP", "MoveSelectionUP");
	
	GlobalEventManager().addCommand("SelectNudgeLeft", "SelectNudgeLeft");
	GlobalEventManager().addCommand("SelectNudgeRight", "SelectNudgeRight");
	GlobalEventManager().addCommand("SelectNudgeUp", "SelectNudgeUp");
	GlobalEventManager().addCommand("SelectNudgeDown", "SelectNudgeDown");

	GlobalEventManager().addCommand("CurveAppendControlPoint", "CurveAppendControlPoint");
	GlobalEventManager().addCommand("CurveRemoveControlPoint", "CurveRemoveControlPoint");
	GlobalEventManager().addCommand("CurveInsertControlPoint", "CurveInsertControlPoint");
	GlobalEventManager().addCommand("CurveConvertType", "CurveConvertType");

	GlobalEventManager().addCommand("BrushExportCM", "BrushExportCM");
	GlobalEventManager().addCommand("CreateDecalsForFaces", "CreateDecalsForFaces");

	GlobalEventManager().addCommand("Copy", "Copy");
	GlobalEventManager().addCommand("Paste", "Paste");
	GlobalEventManager().addCommand("PasteToCamera", "PasteToCamera");

	GlobalEventManager().addRegistryToggle("ToggleRotationPivot", "user/ui/rotationPivotIsOrigin");
	GlobalEventManager().addRegistryToggle("ToggleOffsetClones", selection::algorithm::RKEY_OFFSET_CLONED_OBJECTS);

	GlobalEventManager().addCommand("ConnectSelection", "ConnectSelection");
    GlobalEventManager().addCommand("BindSelection", "BindSelection");
    GlobalEventManager().addRegistryToggle("ToggleFreeModelRotation", RKEY_FREE_MODEL_ROTATION);
    GlobalEventManager().addCommand("CreateCurveNURBS", "CreateCurveNURBS");
    GlobalEventManager().addCommand("CreateCurveCatmullRom", "CreateCurveCatmullRom");
}

	} // namespace algorithm
} // namespace selection
