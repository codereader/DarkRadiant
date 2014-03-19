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

#include <wx/frame.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/artprov.h>

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

// Constructor creates widgets

AddPropertyDialog::AddPropertyDialog(Entity* entity) :
	wxutil::DialogBase(_(ADDPROPERTY_TITLE)),
    _entity(entity)
{
	wxPanel* mainPanel = loadNamedPanel(this, "AddPropertyDialogPanel");

	_treeView = new wxDataViewCtrl(mainPanel, wxID_ANY, wxDefaultPosition, 
		wxDefaultSize, wxBORDER_STATIC | wxDV_MULTIPLE | wxDV_NO_HEADER);
	mainPanel->GetSizer()->Prepend(_treeView, 1, wxEXPAND | wxALL, 6);

	wxButton* okButton = findNamedObject<wxButton>(mainPanel, "OkButton");
	okButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(AddPropertyDialog::_onOK), NULL, this);

	wxButton* cancelButton = findNamedObject<wxButton>(mainPanel, "CancelButton");
	cancelButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(AddPropertyDialog::_onCancel), NULL, this);

	fitToScreen(0.5f, 0.6f);

    // Populate the tree view with properties
    setupTreeView();
    populateTreeView();

	updateUsagePanel();
}

// Construct the tree view

void AddPropertyDialog::setupTreeView()
{
	_treeStore = new wxutil::TreeModel(_columns);

	_treeView->AssociateModel(_treeStore);
	_treeStore->DecRef();

	_treeView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(AddPropertyDialog::_onSelectionChanged), NULL, this);
	_treeView->Connect(wxEVT_DATAVIEW_ITEM_EXPANDED, 
		wxDataViewEventHandler(AddPropertyDialog::_onItemExpanded), NULL, this);

	// Display name column with icon
	_treeView->AppendIconTextColumn(_("Property"), 
		_columns.displayName.getColumnIndex(), wxDATAVIEW_CELL_INERT, 
		wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

#if 0
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
#endif
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
	wxutil::TreeModel* _store;

	const AddPropertyDialog::TreeColumns& _columns;

	// Parent iter
	wxDataViewItem _parent;

	// The entity we're adding the properties to
	Entity* _entity;

public:

	// Constructor sets tree stuff
	CustomPropertyAdder(Entity* entity,
						wxutil::TreeModel* store,
						const AddPropertyDialog::TreeColumns& columns,
						const wxDataViewItem& parent) :
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
		// wxTODO? Glib::ustring escName = Glib::Markup::escape_text(attr.getName());

		wxutil::TreeModel::Row row = _store->AddItem(_parent);

		wxIcon icon;
		icon.CopyFromBitmap(PropertyEditorFactory::getBitmapFor(attr.getType()));

		row[_columns.displayName] = wxVariant(wxDataViewIconText(attr.getName(), icon));
		row[_columns.propertyName] = attr.getName();
		row[_columns.description] = attr.getDescription();

		_store->ItemAdded(_parent, row.getItem());
	}
};

} // namespace

// Populate tree view
void AddPropertyDialog::populateTreeView()
{
	// DEF-DEFINED PROPERTIES
	{
		// First add a top-level category named after the entity class, and populate
		// it with custom keyvals defined in the DEF for that class
		std::string cName = _entity->getEntityClass()->getName();

		wxutil::TreeModel::Row defRoot = _treeStore->AddItem();

		wxIcon folderIcon;
		folderIcon.CopyFromBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + FOLDER_ICON));

		wxDataViewItemAttr blueBold;
		blueBold.SetColour(wxColor(0,0,255));
		blueBold.SetBold(true);

		defRoot[_columns.displayName] = wxVariant(wxDataViewIconText(cName, folderIcon));
		defRoot[_columns.displayName] = blueBold;
		defRoot[_columns.propertyName] = "";
		defRoot[_columns.description] = _(CUSTOM_PROPERTY_TEXT);

		_treeStore->ItemAdded(_treeStore->GetRoot(), defRoot.getItem());

		// Use a CustomPropertyAdder class to visit the entityclass and add all
		// custom properties from it
		CustomPropertyAdder adder(_entity, _treeStore, _columns, defRoot.getItem());
		_entity->getEntityClass()->forEachClassAttribute(boost::ref(adder));
	}

#if 0
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
#endif
}

void AddPropertyDialog::_onDeleteEvent()
{
	// Reset the selection before closing the window
	_selectedProperties.clear();

	//BlockingTransientWindow::_onDeleteEvent();
}

// Static method to create and show an instance, and return the chosen
// property to calling function.
AddPropertyDialog::PropertyList AddPropertyDialog::chooseProperty(Entity* entity)
{
	// Construct a dialog and show the main widget
	AddPropertyDialog* dialog = new AddPropertyDialog(entity);

	if (dialog->ShowModal() == wxID_OK)
	{
		// Return the last selection to calling process
		return dialog->_selectedProperties;
	}

	return PropertyList();
}

void AddPropertyDialog::updateUsagePanel()
{
#if 0
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
#endif
}

void AddPropertyDialog::_onOK(wxCommandEvent& ev)
{
	EndModal(wxID_OK);
}

void AddPropertyDialog::_onCancel(wxCommandEvent& ev)
{
	_selectedProperties.clear();
	EndModal(wxID_CANCEL);
}

void AddPropertyDialog::_onSelectionChanged(wxDataViewEvent& ev)
{
	_selectedProperties.clear();

	wxDataViewItemArray selection;

	if (_treeView->GetSelections(selection) > 0)
	{
		std::for_each(selection.begin(), selection.end(), [&] (const wxDataViewItem& item)
		{
			wxutil::TreeModel::Row row(item, *_treeStore);

			_selectedProperties.push_back(row[_columns.propertyName]);
		});
	}

	updateUsagePanel();
}

void AddPropertyDialog::_onItemExpanded(wxDataViewEvent& ev)
{
	// This should force a recalculation of the column width
	_treeStore->ItemChanged(_treeStore->GetRoot());
}

} // namespace ui
