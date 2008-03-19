#include "FindMapElements.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "gtkutil/dialog.h"
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


class BrushFindIndexWalker : public scene::Graph::Walker
{
  mutable scene::INodePtr m_node;
  std::size_t& m_count;
public:
  BrushFindIndexWalker(const scene::INodePtr node, std::size_t& count)
    : m_node(node), m_count(count)
  {
  }
  bool pre(const scene::Path& path, const scene::INodePtr& node) const
  {
    if(Node_isPrimitive(node))
    {
      if(m_node == node) {
        m_node = scene::INodePtr();
      }
      if(m_node)
      {
        ++m_count;
      }
    }
    return true;
  }
};

class EntityFindIndexWalker : public scene::Graph::Walker
{
  mutable scene::INodePtr m_node;
  std::size_t& m_count;
public:
  EntityFindIndexWalker(const scene::INodePtr node, std::size_t& count)
    : m_node(node), m_count(count)
  {
  }
  bool pre(const scene::Path& path, const scene::INodePtr& node) const
  {
    if(Node_isEntity(path.top()))
    {
      if(m_node == path.top())
      {
        m_node = scene::INodePtr();
      }
      if(m_node)
      {
        ++m_count;
      }
    }
    return true;
  }
};

/*static void GetSelectionIndex (int *ent, int *brush)
{
  std::size_t count_brush = 0;
  std::size_t count_entity = 0;
  if(GlobalSelectionSystem().countSelected() != 0)
  {
    const scene::Path& path = GlobalSelectionSystem().ultimateSelected().path();

    GlobalSceneGraph().traverse(BrushFindIndexWalker(path.top(), count_brush));
    GlobalSceneGraph().traverse(EntityFindIndexWalker(path.parent(), count_entity));
  }
  *brush = int(count_brush);
  *ent = int(count_entity);
}*/

void DoFind()
{
  ModalDialog dialog;
  GtkEntry* entity;
  GtkEntry* brush;

  GtkWindow* window = create_dialog_window(GlobalRadiant().getMainWindow(), "Find Brush", G_CALLBACK(dialog_delete_callback), &dialog);

  GtkAccelGroup* accel = gtk_accel_group_new();
  gtk_window_add_accel_group(window, accel);

  {
    GtkVBox* vbox = create_dialog_vbox(4, 4);
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(vbox));
    {
      GtkTable* table = create_dialog_table(2, 2, 4, 4);
      gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(table), TRUE, TRUE, 0);
      {
        GtkWidget* label = gtk_label_new ("Entity number");
        gtk_widget_show (label);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                          (GtkAttachOptions) (0),
                          (GtkAttachOptions) (0), 0, 0);
      }
      {
        GtkWidget* label = gtk_label_new ("Brush number");
        gtk_widget_show (label);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                          (GtkAttachOptions) (0),
                          (GtkAttachOptions) (0), 0, 0);
      }
      {
        GtkEntry* entry = GTK_ENTRY(gtk_entry_new());
        gtk_widget_show(GTK_WIDGET(entry));
        gtk_table_attach(table, GTK_WIDGET(entry), 1, 2, 0, 1,
                          (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                          (GtkAttachOptions) (0), 0, 0);
        gtk_widget_grab_focus(GTK_WIDGET(entry));
        entity = entry;
      }
      {
        GtkEntry* entry = GTK_ENTRY(gtk_entry_new());
        gtk_widget_show(GTK_WIDGET(entry));
        gtk_table_attach(table, GTK_WIDGET(entry), 1, 2, 1, 2,
                          (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                          (GtkAttachOptions) (0), 0, 0);

        brush = entry;
      }
    }
    {
      GtkHBox* hbox = create_dialog_hbox(4);
      gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(hbox), TRUE, TRUE, 0);
      {
        GtkButton* button = create_dialog_button("Find", G_CALLBACK(dialog_button_ok), &dialog);
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(button), FALSE, FALSE, 0);
        widget_make_default(GTK_WIDGET(button));
        gtk_widget_add_accelerator(GTK_WIDGET(button), "clicked", accel, GDK_Return, (GdkModifierType)0, (GtkAccelFlags)0);
      }
      {
        GtkButton* button = create_dialog_button("Close", G_CALLBACK(dialog_button_cancel), &dialog);
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(button), FALSE, FALSE, 0);
        gtk_widget_add_accelerator(GTK_WIDGET(button), "clicked", accel, GDK_Escape, (GdkModifierType)0, (GtkAccelFlags)0);
      }
    }
  }

  // Initialize dialog
  char buf[16];
  int ent(0), br(0);

  //GetSelectionIndex (&ent, &br);
  sprintf (buf, "%i", ent);
  gtk_entry_set_text(entity, buf);
  sprintf (buf, "%i", br);
  gtk_entry_set_text(brush, buf);

  if(modal_dialog_show(window, dialog) == eIDOK)
  {
    const char *entstr = gtk_entry_get_text(entity);
    const char *brushstr = gtk_entry_get_text(brush);
    SelectBrush (atoi(entstr), atoi(brushstr));
  }

  gtk_widget_destroy(GTK_WIDGET(window));
}
