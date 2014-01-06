#include "FindMapElements.h"

#include "i18n.h"
#include "idialogmanager.h"
#include "iscenegraph.h"
#include "iradiant.h"
#include "scenelib.h"
#include "xyview/GlobalXYWnd.h"
#include "selection/algorithm/General.h"

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
    if (Node_isPrimitive(node) && m_index-- == 0)
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
	  path.top()->traverseChildren(visitor);
  }

  if(path.size() == 2)
  {
	  BrushFindByIndexWalker visitor(brush, path);
	  path.top()->traverseChildren(visitor);
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

void DoFind(const cmd::ArgumentList& args)
{
	ui::IDialogPtr dialog = GlobalDialogManager().createDialog(_("Find Brush"));

	ui::IDialog::Handle entityEntry = dialog->addEntryBox(_("Entity Number:"));
	ui::IDialog::Handle brushEntry = dialog->addEntryBox(_("Brush Number:"));

	std::size_t ent(0), br(0);
	selection::algorithm::getSelectionIndex(ent, br);

	dialog->setElementValue(entityEntry, string::to_string(ent));
	dialog->setElementValue(brushEntry, string::to_string(br));

	if (dialog->run() == ui::IDialog::RESULT_OK)
	{
		std::string entityValue = dialog->getElementValue(entityEntry);
		std::string brushValue = dialog->getElementValue(brushEntry);

		SelectBrush(string::convert<int>(entityValue),
                    string::convert<int>(brushValue));
	}
}
