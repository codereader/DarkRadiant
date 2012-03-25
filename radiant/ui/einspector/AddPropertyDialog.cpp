#include "AddPropertyDialog.h"
#include "PropertyEditorFactory.h"

#include "gtkutil/RightAlignment.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/IconTextColumn.h"

#include "i18n.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "ieclass.h"
#include "igame.h"
#include "ientity.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/treeview.h>
#include <gtkmm/textview.h>

#include <boost/algorithm/string/predicate.hpp>

#include <map>

namespace ui
{

namespace
{
	// CONSTANTS
	const char* const ADDPROPERTY_TITLE = N_("Add property");
	const char* const PROPERTIES_XPATH = "/entityInspector//property";
	const char* const FOLDER_ICON = "folder16.png";

	const char* const CUSTOM_PROPERTY_TEXT = N_("Custom properties defined for this "
												"entity class, if any");

}

// Constructor creates GTK widgets

AddPropertyDialog::AddPropertyDialog(Entity* entity) :
	gtkutil::BlockingTransientWindow(
        _(ADDPROPERTY_TITLE), GlobalMainFrame().getTopLevelWindow()
    ),
    gtkutil::GladeWidgetHolder("AddPropertyDialog.glade"),
	_entity(entity)
{
    // Set size of dialog
	Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(
		GlobalMainFrame().getTopLevelWindow()
	);
	set_default_size(static_cast<int>(rect.get_width()/2), static_cast<int>(rect.get_height()*2/3));

    // Create components
    add(*gladeWidget<Gtk::Widget>("mainVbox"));
    g_assert(get_child() != 0);

	gladeWidget<Gtk::Button>("addButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &AddPropertyDialog::_onOK)
    );
	gladeWidget<Gtk::Button>("cancelButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &AddPropertyDialog::_onCancel)
    );

    // Populate the tree view with properties
    setupTreeView();
    populateTreeView();

	updateUsagePanel();
}

// Construct the tree view

void AddPropertyDialog::setupTreeView()
{
	// Set up the tree store
	_treeStore = Gtk::TreeStore::create(_columns);

    Gtk::TreeView* treeView = gladeWidget<Gtk::TreeView>(
        "propertyTreeView"
    );
    treeView->set_model(_treeStore);

	// Connect up selection changed callback
	_selection = treeView->get_selection();
	_selection->signal_changed().connect(
		sigc::mem_fun(*this, &AddPropertyDialog::_onSelectionChanged)
    );

	// Allow multiple selections
	_selection->set_mode(Gtk::SELECTION_MULTIPLE);

	// Display name column with icon
	treeView->append_column(*Gtk::manage(
		new gtkutil::IconTextColumn("", _columns.displayName, _columns.icon, true))
    );

	// Use the TreeModel's full string search function
	treeView->set_search_equal_func(
        sigc::ptr_fun(gtkutil::TreeModel::equalFuncStringContains)
    );
}

namespace
{

/* EntityClassAttributeVisitor instance to obtain custom properties from an
 * entityclass and add them into the provided GtkTreeStore under the provided
 * parent iter.
 */
class CustomPropertyAdder
{
	// Treestore to add to
	Glib::RefPtr<Gtk::TreeStore> _store;

	const AddPropertyDialog::TreeColumns& _columns;

	// Parent iter
	Gtk::TreeModel::iterator _parent;

	// The entity we're adding the properties to
	Entity* _entity;

public:

	// Constructor sets tree stuff
	CustomPropertyAdder(Entity* entity,
						const Glib::RefPtr<Gtk::TreeStore>& store,
						const AddPropertyDialog::TreeColumns& columns,
						Gtk::TreeModel::iterator parent) :
		_store(store),
		_columns(columns),
		_parent(parent),
		_entity(entity)
	{ }

	// Required visit function
	void operator() (const EntityClassAttribute& attr)
	{
		// greebo: Only add the property if it hasn't been set directly on the entity itself.
		if (!_entity->getKeyValue(attr.getName()).empty() && !_entity->isInherited(attr.getName()))
		{
			return;
		}

		// Also ignore all attributes with empty descriptions
		if (attr.getDescription().empty())
		{
			return;
		}

		// Escape any Pango markup in the attribute name (e.g. "<" or ">")
		Glib::ustring escName = Glib::Markup::escape_text(attr.getName());

		Gtk::TreeModel::Row row = *_store->append(_parent->children());

		row[_columns.displayName] = escName;
		row[_columns.propertyName] = attr.getName();
		row[_columns.icon] = PropertyEditorFactory::getPixbufFor(attr.getType());
		row[_columns.description] = attr.getDescription();
	}
};

} // namespace

// Populate tree view
void AddPropertyDialog::populateTreeView()
{
	/* DEF-DEFINED PROPERTIES */
	{
		// First add a top-level category named after the entity class, and populate
		// it with custom keyvals defined in the DEF for that class
		std::string cName = "<b><span foreground=\"blue\">"
							+ _entity->getEntityClass()->getName() + "</span></b>";

		Gtk::TreeModel::iterator cn = _treeStore->append();
		Gtk::TreeModel::Row row = *cn;

		row[_columns.displayName] = cName;
		row[_columns.propertyName] = "";
		row[_columns.icon] = GlobalUIManager().getLocalPixbuf(FOLDER_ICON);
		row[_columns.description] = _(CUSTOM_PROPERTY_TEXT);

		// Use a CustomPropertyAdder class to visit the entityclass and add all
		// custom properties from it
		CustomPropertyAdder adder(_entity, _treeStore, _columns, cn);
		_entity->getEntityClass()->forEachClassAttribute(boost::ref(adder));
	}

	/* REGISTRY (GAME FILE) DEFINED PROPERTIES */

	// Ask the XML registry for the list of properties
    game::IGamePtr currentGame = GlobalGameManager().currentGame();
    xml::NodeList propNodes = currentGame->getLocalXPath(PROPERTIES_XPATH);

	// Cache of property categories to GtkTreeIters, to allow properties
	// to be parented to top-level categories
	typedef std::map<std::string, Gtk::TreeModel::iterator> CategoryMap;
	CategoryMap categories;

	// Add each .game-specified property to the tree view
	for (xml::NodeList::const_iterator iter = propNodes.begin();
		 iter != propNodes.end();
		 ++iter)
	{
		// Skip hidden properties
		if (iter->getAttributeValue("hidden") == "1")
		{
			continue;
		}

		Gtk::TreeModel::iterator t;

		// If this property has a category, look up the top-level parent iter
		// or add it if necessary.
		std::string category = iter->getAttributeValue("category");

		if (!category.empty())
		{
			CategoryMap::iterator mIter = categories.find(category);

			if (mIter == categories.end())
			{
				// Not found, add to treestore
				Gtk::TreeModel::iterator catIter = _treeStore->append();

				Gtk::TreeModel::Row row = *catIter;

				row[_columns.displayName] = category;
				row[_columns.propertyName] = "";
				row[_columns.icon] = GlobalUIManager().getLocalPixbuf(FOLDER_ICON);
				row[_columns.description] = "";

				// Add to map
				mIter = categories.insert(CategoryMap::value_type(category, catIter)).first;
			}

			// Category sorted, add this property below it
			t = _treeStore->append(mIter->second->children());
		}
		else
		{
			// No category, add at toplevel
			t = _treeStore->append();
		}

		// Obtain information from the XML node and add it to the treeview
		std::string name = iter->getAttributeValue("match");
		std::string type = iter->getAttributeValue("type");
		std::string description = iter->getContent();

		Gtk::TreeModel::Row row = *t;

		row[_columns.displayName] = name;
		row[_columns.propertyName] = name;
		row[_columns.icon] = PropertyEditorFactory::getPixbufFor(type);
		row[_columns.description] = description;
	}
}

void AddPropertyDialog::_onDeleteEvent()
{
	// Reset the selection before closing the window
	_selectedProperties.clear();

	BlockingTransientWindow::_onDeleteEvent();
}

// Static method to create and show an instance, and return the chosen
// property to calling function.
AddPropertyDialog::PropertyList AddPropertyDialog::chooseProperty(Entity* entity)
{
	// Construct a dialog and show the main widget
	AddPropertyDialog dialog(entity);

	dialog.show(); // and block

	// Return the last selection to calling process
	return dialog._selectedProperties;
}

void AddPropertyDialog::updateUsagePanel()
{
    Gtk::TextView* usageTextView = gladeWidget<Gtk::TextView>(
        "usageTextView"
    );
	Glib::RefPtr<Gtk::TextBuffer> buf = usageTextView->get_buffer();

	if (_selectedProperties.size() != 1)
	{
		buf->set_text("");
		usageTextView->set_sensitive(false);
	}
	else
	{
		// Load the description
		Gtk::TreeSelection::ListHandle_Path handle = _selection->get_selected_rows();

		for (Gtk::TreeSelection::ListHandle_Path::iterator i = handle.begin();
			 i != handle.end(); ++i)
		{
			Gtk::TreeModel::Row row = *(_treeStore->get_iter(*i));

			std::string desc = Glib::ustring(row[_columns.description]);
			buf->set_text(desc);
		}

		usageTextView->set_sensitive(true);
	}
}

void AddPropertyDialog::_onOK()
{
	destroy();
}

void AddPropertyDialog::_onCancel()
{
	_selectedProperties.clear();
	destroy();
}

void AddPropertyDialog::_onSelectionChanged()
{
	_selectedProperties.clear();

	Gtk::TreeSelection::ListHandle_Path handle = _selection->get_selected_rows();

	for (Gtk::TreeSelection::ListHandle_Path::iterator i = handle.begin();
		 i != handle.end(); ++i)
	{
		Gtk::TreeModel::Row row = *(_treeStore->get_iter(*i));

		_selectedProperties.push_back(Glib::ustring(row[_columns.propertyName]));
	}

	updateUsagePanel();
}

} // namespace ui
