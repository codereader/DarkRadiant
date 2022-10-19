#include "AddPropertyDialog.h"
#include "PropertyEditorFactory.h"

#include "i18n.h"
#include "ui/imainframe.h"
#include "ieclass.h"
#include "igame.h"
#include "ientity.h"

#include "wxutil/dataview/TreeView.h"
#include <wx/frame.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/panel.h>
#include "wxutil/Bitmap.h"
#include "wxutil/Icon.h"
#include <wx/textctrl.h>

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
	Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(AddPropertyDialog::_onDeleteEvent), NULL, this);

	wxPanel* mainPanel = loadNamedPanel(this, "AddPropertyDialogPanel");

	_treeView = wxutil::TreeView::Create(mainPanel, wxBORDER_STATIC | wxDV_MULTIPLE | wxDV_NO_HEADER);
	mainPanel->GetSizer()->Prepend(_treeView, 1, wxEXPAND | wxALL, 6);

	wxButton* okButton = findNamedObject<wxButton>(mainPanel, "OkButton");
	okButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(AddPropertyDialog::_onOK), NULL, this);

	wxButton* cancelButton = findNamedObject<wxButton>(mainPanel, "CancelButton");
	cancelButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(AddPropertyDialog::_onCancel), NULL, this);

	FitToScreen(0.5f, 0.6f);

    // Populate the tree view with properties
    setupTreeView();
    populateTreeView();

	updateUsagePanel();
}

// Construct the tree view

void AddPropertyDialog::setupTreeView()
{
	_treeStore = new wxutil::TreeModel(_columns);

	_treeView->AssociateModel(_treeStore.get());

	_treeView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED,
		wxDataViewEventHandler(AddPropertyDialog::_onSelectionChanged), NULL, this);

	// Display name column with icon
	_treeView->AppendIconTextColumn(_("Property"),
		_columns.displayName.getColumnIndex(), wxDATAVIEW_CELL_INERT,
		wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	// Use the TreeModel's full string search function
	_treeView->AddSearchColumn(_columns.displayName);
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
	wxutil::TreeModel::Ptr _store;

	const AddPropertyDialog::TreeColumns& _columns;

	// Parent iter
	wxDataViewItem _parent;

	// The entity we're adding the properties to
	Entity* _entity;

public:

	// Constructor sets tree stuff
	CustomPropertyAdder(Entity* entity,
						wxutil::TreeModel::Ptr store,
						const AddPropertyDialog::TreeColumns& columns,
						const wxDataViewItem& parent) :
		_store(store),
		_columns(columns),
		_parent(parent),
		_entity(entity)
	{ }

	// Required visit function
	void operator() (const EntityClassAttribute& attr, bool)
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

		wxutil::TreeModel::Row row = _store->AddItem(_parent);

        wxutil::Icon icon(PropertyEditorFactory::getBitmapFor(attr.getType()));

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
    wxutil::Icon folderIcon(wxutil::GetLocalBitmap(FOLDER_ICON));

	// DEF-DEFINED PROPERTIES
	{
		// First add a top-level category named after the entity class, and populate
		// it with custom keyvals defined in the DEF for that class
		std::string cName = _entity->getEntityClass()->getDeclName();

		wxutil::TreeModel::Row defRoot = _treeStore->AddItem();

		wxDataViewItemAttr blueBold;
		blueBold.SetColour(wxColor(0,0,255));
		blueBold.SetBold(true);

		defRoot[_columns.displayName] = wxVariant(wxDataViewIconText(cName, folderIcon));
		defRoot[_columns.displayName].setAttr(blueBold);
		defRoot[_columns.propertyName] = "";
		defRoot[_columns.description] = _(CUSTOM_PROPERTY_TEXT);

		defRoot.SendItemAdded();

		// Use a CustomPropertyAdder class to visit the entityclass and add all
		// custom properties from it
		CustomPropertyAdder adder(_entity, _treeStore, _columns, defRoot.getItem());
		_entity->getEntityClass()->forEachAttribute(std::ref(adder));
	}

	// REGISTRY (GAME FILE) DEFINED PROPERTIES

	// Ask the XML registry for the list of properties
    game::IGamePtr currentGame = GlobalGameManager().currentGame();
    xml::NodeList propNodes = currentGame->getLocalXPath(PROPERTIES_XPATH);

	// Cache of property categories to GtkTreeIters, to allow properties
	// to be parented to top-level categories
	typedef std::map<std::string, wxDataViewItem> CategoryMap;
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

		wxDataViewItem item;

		// If this property has a category, look up the top-level parent iter
		// or add it if necessary.
		std::string category = iter->getAttributeValue("category");

		if (!category.empty())
		{
			CategoryMap::iterator mIter = categories.find(category);

			if (mIter == categories.end())
			{
				// Not found, add to treestore
				wxutil::TreeModel::Row catRow = _treeStore->AddItem();

				catRow[_columns.displayName] = wxVariant(wxDataViewIconText(category, folderIcon));;
				catRow[_columns.propertyName] = "";
				catRow[_columns.description] = "";

				// Add to map
				mIter = categories.insert(CategoryMap::value_type(category, catRow.getItem())).first;

				catRow.SendItemAdded();
			}

			// Category sorted, add this property below it
			item = _treeStore->AddItem(mIter->second).getItem();

			_treeStore->ItemAdded(mIter->second, item);
		}
		else
		{
			// No category, add at toplevel
			item = _treeStore->AddItem().getItem();

			_treeStore->ItemAdded(_treeStore->GetRoot(), item);
		}

		// Obtain information from the XML node and add it to the treeview
		std::string name = iter->getAttributeValue("match");
		std::string type = iter->getAttributeValue("type");
		std::string description = iter->getContent();

		wxutil::TreeModel::Row row(item, *_treeStore);

        wxutil::Icon icon(PropertyEditorFactory::getBitmapFor(type));

		row[_columns.displayName] = wxVariant(wxDataViewIconText(name, icon));
		row[_columns.propertyName] = name;
		row[_columns.description] = description;

		_treeStore->ItemChanged(item);
	}
}

void AddPropertyDialog::_onDeleteEvent(wxCloseEvent& ev)
{
	// Reset the selection before closing the window
	_selectedProperties.clear();
	EndModal(wxID_CANCEL);
}

// Static method to create and show an instance, and return the chosen
// property to calling function.
AddPropertyDialog::PropertyList AddPropertyDialog::chooseProperty(Entity* entity)
{
	PropertyList returnValue;

	// Construct a dialog and show the main widget
	AddPropertyDialog* dialog = new AddPropertyDialog(entity);

	if (dialog->ShowModal() == wxID_OK)
	{
		// Return the last selection to calling process
		returnValue = dialog->_selectedProperties;
	}

	dialog->Destroy();

	return returnValue;
}

void AddPropertyDialog::updateUsagePanel()
{
	wxTextCtrl* usageText = findNamedObject<wxTextCtrl>(this, "Description");

	if (_selectedProperties.size() != 1)
	{
		usageText->SetValue("");
		usageText->Enable(false);
	}
	else
	{
		// Load the description
		wxutil::TreeModel::Row row(_treeView->GetSelection(), *_treeStore);

		usageText->SetValue(row[_columns.description].getVariant().GetString());
		usageText->Enable(true);
	}
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

} // namespace ui
