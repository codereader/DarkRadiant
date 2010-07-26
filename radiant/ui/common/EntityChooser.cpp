#include "EntityChooser.h"

#include "i18n.h"
#include "inode.h"
#include "ientity.h"
#include "imainframe.h"
#include "iscenegraph.h"
#include <gtk/gtk.h>
#include "gtkutil/dialog/Dialog.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/ScrolledFrame.h"

namespace ui
{

namespace
{
	// ListStore columns
	enum
	{
		NAME_COLUMN,
		NUM_COLUMNS,
	};

	enum
	{
		WIDGET_TOPLEVEL,
		WIDGET_TREEVIEW,
	};
}

EntityChooser::EntityChooser() :
	gtkutil::DialogElement(), // create an Element without label
	_entityStore(gtk_list_store_new(NUM_COLUMNS, G_TYPE_STRING))
{
	_widgets[WIDGET_TOPLEVEL] = gtk_vbox_new(FALSE, 6);

	// Initialise the base class
	DialogElement::setWidget(_widgets[WIDGET_TOPLEVEL]);

	_widgets[WIDGET_TREEVIEW] = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_entityStore));

	GtkTreeView* treeView = GTK_TREE_VIEW(_widgets[WIDGET_TREEVIEW]);
	gtk_tree_view_set_headers_visible(treeView, FALSE);

	// Use the TreeModel's full string search function
	gtk_tree_view_set_search_equal_func(
		GTK_TREE_VIEW(treeView), 
		gtkutil::TreeModel::equalFuncStringContains, 
		NULL, 
		NULL
	);

	// Head Name column
	gtk_tree_view_append_column(treeView, gtkutil::TextColumn("", NAME_COLUMN));
	
	// Set the tree store to sort on this column
    gtk_tree_sortable_set_sort_column_id(
        GTK_TREE_SORTABLE(_entityStore),
        NAME_COLUMN,
        GTK_SORT_ASCENDING
    );

	_selection = gtk_tree_view_get_selection(treeView);
	g_signal_connect(G_OBJECT(_selection), "changed", G_CALLBACK(onSelectionChanged), this);
	
	// Scrolled Frame
	gtk_box_pack_start(GTK_BOX(_widgets[WIDGET_TOPLEVEL]),
		gtkutil::ScrolledFrame(GTK_WIDGET(treeView)), TRUE, TRUE, 0);

	populateEntityList();
}

std::string EntityChooser::getSelectedEntity() const
{
	return exportToString();
}

void EntityChooser::setSelectedEntity(const std::string& name)
{
	importFromString(name);
}

std::string EntityChooser::exportToString() const
{
	return _selectedEntityName;
}

void EntityChooser::importFromString(const std::string& str)
{
	// Instantiate a finder
	gtkutil::TreeModel::SelectionFinder finder(str, NAME_COLUMN);

	gtk_tree_model_foreach(GTK_TREE_MODEL(_entityStore), 
		gtkutil::TreeModel::SelectionFinder::forEach, &finder);

	if (finder.getPath() != NULL)
	{
		gtk_tree_selection_select_path(_selection, finder.getPath());
	}
}

std::string EntityChooser::ChooseEntity(const std::string& preSelectedEntity)
{
	gtkutil::Dialog dlg(_("Select Entity"), GlobalMainFrame().getTopLevelWindow());

	Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(GlobalMainFrame().getTopLevelWindow());

	gtk_window_set_default_size(
		GTK_WINDOW(dlg.getWindow()), static_cast<int>(rect.get_width()/2), static_cast<int>(2*rect.get_height()/3)
	);
	
	// Instantiate a new chooser class
	EntityChooserPtr chooser(new EntityChooser);
	chooser->setSelectedEntity(preSelectedEntity);

	// add this to the dialog window
	IDialog::Handle handle = dlg.addElement(chooser);

	if (dlg.run() == IDialog::RESULT_OK)
	{
		return dlg.getElementValue(handle);
	}
	else
	{
		// Cancelled
		return "";
	}
}

void EntityChooser::populateEntityList()
{
	struct EntityFinder: 
		public scene::NodeVisitor
	{
        // List store to add to
        GtkListStore* _store;
        
        // Constructor
		EntityFinder(GtkListStore* store) :
			_store(store)
		{}
            
        // Visit function
        bool pre(const scene::INodePtr& node)
		{
			// Check for an entity
            Entity* entity = Node_getEntity(node);

            if (entity != NULL)
			{
				// Get the entity name
                std::string entName = entity->getKeyValue("name");

				// Append the name to the list store
                GtkTreeIter iter;
                gtk_list_store_append(_store, &iter);
                gtk_list_store_set(_store, &iter, 
                				   0, entName.c_str(), 
                				   -1);
            }
            
            return false; // don't traverse deeper, we're traversing root children
        }
    } finder(_entityStore);

	GlobalSceneGraph().root()->traverse(finder);
}

void EntityChooser::onSelectionChanged(GtkTreeSelection* sel, EntityChooser* self)
{
	// Prepare to check for a selection
	GtkTreeIter iter;
	GtkTreeModel* model;

	// Add button is enabled if there is a selection and it is not a folder.
	if (gtk_tree_selection_get_selected(sel, &model, &iter)) 
	{
		// Set the selected name
		self->_selectedEntityName = gtkutil::TreeModel::getString(model, &iter, NAME_COLUMN); 
	}
	else
	{
		self->_selectedEntityName.clear();
	}
}

} // namespace
