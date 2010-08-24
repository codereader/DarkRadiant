#include "FindMapElements.h"

#include "i18n.h"
#include "idialogmanager.h"
#include "iscenegraph.h"
#include "iradiant.h"
#include "scenelib.h"
#include "xyview/GlobalXYWnd.h"

/** greebo:  This file contains code from map.cpp concerning the lookup
 * 			 of map elements (primitives and entities) by number.
 *
 * 			 No refactoring done yet, I just copied and pasted the stuff here.  
 */
 
 
class BrushFindByIndexWalker : 
	public scene::NodeVisitor
{
  mutable std::size_t m_index;
  scene::Path& m_path;
public:
  BrushFindByIndexWalker(std::size_t index, scene::Path& path)
    : m_index(index), m_path(path)
  {
  }
  virtual bool pre(const scene::INodePtr& node)
  {
    if(Node_isPrimitive(node) && m_index-- == 0)
    {
      m_path.push(node);
    }
    return false;
  }
};

class EntityFindByIndexWalker : 
	public scene::NodeVisitor
{
  mutable std::size_t m_index;
  scene::Path& m_path;
public:
  EntityFindByIndexWalker(std::size_t index, scene::Path& path)
    : m_index(index), m_path(path)
  {
  }
  virtual bool pre(const scene::INodePtr& node) {
    if(Node_isEntity(node) && m_index-- == 0)
    {
      m_path.push(node);
    }
    return false;
  }
};

void Scene_FindEntityBrush(std::size_t entity, std::size_t brush, scene::Path& path)
{
  path.push(GlobalSceneGraph().root());
  {
	  EntityFindByIndexWalker visitor(entity, path);
	  path.top()->traverse(visitor);
  }

  if(path.size() == 2)
  {
	  BrushFindByIndexWalker visitor(brush, path);
	  path.top()->traverse(visitor);
  }
}

inline bool Node_hasChildren(scene::INodePtr node)
{
	return node->hasChildNodes();
}

void SelectBrush (int entitynum, int brushnum)
{
  scene::Path path;
  Scene_FindEntityBrush(entitynum, brushnum, path);
  if (path.size() == 3 || (path.size() == 2 && !Node_hasChildren(path.top())))
  {
	  Node_setSelected(path.top(), true);
    
    XYWndPtr xyView = GlobalXYWnd().getActiveXY();
    
    if (xyView) {
    	xyView->positionView(path.top()->worldAABB().origin);
    }
  }
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

	bool pre(const scene::INodePtr& node) {
		if (Node_isPrimitive(node)) {
			// Have we found the node?
			if (_node == node) {
				// Yes, found, set needle to NULL
				_node = scene::INodePtr();
			}

			// As long as the needle is non-NULL, increment the counter
			if (_node != NULL) {
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

	bool pre(const scene::INodePtr& node) {
		if (Node_isEntity(node)) {
			// Have we found the node?
			if (_node == node) {
				// Yes, found, set needle to NULL
				_node = scene::INodePtr();
			}

			// As long as the needle is non-NULL, increment the counter
			if (_node != NULL) {
				++_count;
			}
		}

		return true;
	}
};

void GetSelectionIndex(std::size_t& ent, std::size_t& brush)
{
	if (GlobalSelectionSystem().countSelected() != 0)
	{
		scene::INodePtr node = GlobalSelectionSystem().ultimateSelected();
		scene::INodePtr parent = node->getParent();

		if (Node_isEntity(node)) {
			// Selection is an entity, find its index
			EntityFindIndexWalker walker(node, ent);
			Node_traverseSubgraph(GlobalSceneGraph().root(), walker);
		}
		else if (Node_isPrimitive(node)) {
			// Node is a primitive, find parent entity and child index
			EntityFindIndexWalker walker(parent, ent);
			Node_traverseSubgraph(GlobalSceneGraph().root(), walker);

			PrimitiveFindIndexWalker brushWalker(node, brush);
			Node_traverseSubgraph(parent, brushWalker);
		}
	}
}

void DoFind(const cmd::ArgumentList& args)
{
	ui::IDialogPtr dialog = GlobalDialogManager().createDialog(_("Find Brush"));

	ui::IDialog::Handle entityEntry = dialog->addEntryBox(_("Entity Number:"));
	ui::IDialog::Handle brushEntry = dialog->addEntryBox(_("Brush Number:"));

	std::size_t ent(0), br(0);
	GetSelectionIndex(ent, br);

	dialog->setElementValue(entityEntry, sizetToStr(ent));
	dialog->setElementValue(brushEntry, sizetToStr(br));
	
	if (dialog->run() == ui::IDialog::RESULT_OK)
	{
		std::string entityValue = dialog->getElementValue(entityEntry);
		std::string brushValue = dialog->getElementValue(brushEntry);
		
		SelectBrush(strToInt(entityValue), strToInt(brushValue));
	}
}
