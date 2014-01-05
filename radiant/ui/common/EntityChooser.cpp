#include "EntityChooser.h"

#include "i18n.h"
#include "inode.h"
#include "ientity.h"
#include "imainframe.h"
#include "iscenegraph.h"

#include "gtkutil/dialog/Dialog.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/ScrolledFrame.h"

#include <gtkmm/box.h>
#include <gtkmm/treeview.h>

namespace ui
{

namespace
{
	enum
	{
		WIDGET_TOPLEVEL,
		WIDGET_TREEVIEW,
	};
}

EntityChooser::EntityChooser() :
	gtkutil::DialogElement(), // create an Element without label
	_entityStore(Gtk::ListStore::create(_listColumns))
{
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 6));
	_widgets[WIDGET_TOPLEVEL] = vbox;

	// Initialise the base class
	DialogElement::setValueWidget(_widgets[WIDGET_TOPLEVEL]);

	Gtk::TreeView* treeView = Gtk::manage(new Gtk::TreeView(_entityStore));
	_widgets[WIDGET_TREEVIEW] = treeView;

	treeView->set_headers_visible(false);

	// Use the TreeModel's full string search function
	treeView->set_search_equal_func(sigc::ptr_fun(&gtkutil::TreeModel::equalFuncStringContains));

	// Head Name column
	treeView->append_column("", _listColumns.name);

	// Set the tree store to sort on this column
	_entityStore->set_sort_column_id(_listColumns.name, Gtk::SORT_ASCENDING);

	_selection = treeView->get_selection();
	_selection->signal_changed().connect(sigc::mem_fun(*this, &EntityChooser::onSelectionChanged));

	// Scrolled Frame
	vbox->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*treeView)), true, true, 0);

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
	Gtk::TreeModel::Children children = _entityStore->children();

	for (Gtk::TreeModel::Children::iterator i = children.begin(); i != children.end(); ++i)
	{
		Gtk::TreeModel::Row row = *i;

		if (row[_listColumns.name] == str)
		{
			_selection->select(i);
			break;
		}
	}
}

std::string EntityChooser::ChooseEntity(const std::string& preSelectedEntity)
{
	gtkutil::Dialog dlg(_("Select Entity"), GlobalMainFrame().getTopLevelWindow());

	Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(GlobalMainFrame().getTopLevelWindow());

	dlg.set_default_size(static_cast<int>(rect.get_width()/2), static_cast<int>(2*rect.get_height()/3));

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
        Glib::RefPtr<Gtk::ListStore> _store;

		EntityChooserColumns& _columns;

        // Constructor
		EntityFinder(const Glib::RefPtr<Gtk::ListStore>& store,
					 EntityChooserColumns& columns) :
			_store(store),
			_columns(columns)
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
				Gtk::TreeModel::Row row = *_store->append();

				row[_columns.name] = entName;
            }

            return false; // don't traverse deeper, we're traversing root children
        }
    } finder(_entityStore, _listColumns);

	GlobalSceneGraph().root()->traverseChildren(finder);
}

void EntityChooser::onSelectionChanged()
{
	// Prepare to check for a selection
	Gtk::TreeModel::iterator iter = _selection->get_selected();

	if (iter)
	{
		_selectedEntityName = iter->get_value(_listColumns.name);
	}
	else
	{
		_selectedEntityName.clear();
	}
}

} // namespace
