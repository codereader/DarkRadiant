#include "AddPropertyDialog.h"
#include "PropertyEditorFactory.h"

#include "i18n.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "ieclass.h"
#include "igame.h"
#include "ientity.h"

#include <wx/frame.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/artprov.h>
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

    enum
    {
        NAME_COLUMN = 0,
        DESCRIPTION_COLUMN,
        N_COLUMNS
    };
}

// Constructor creates widgets

AddPropertyDialog::AddPropertyDialog(Entity* entity) :
    wxutil::DialogBase(_(ADDPROPERTY_TITLE)),
    _entity(entity)
{
    Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(AddPropertyDialog::_onDeleteEvent), NULL, this);

    wxPanel* mainPanel = loadNamedPanel(this, "AddPropertyDialogPanel");

    _treeView = new wxTreeListCtrl(mainPanel, wxID_ANY, wxDefaultPosition,
                                   wxDefaultSize, wxTL_MULTIPLE,
                                   ADDPROPERTY_TITLE);
    mainPanel->GetSizer()->Prepend(_treeView.get(), 1, wxEXPAND | wxALL, 6);

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
    // Display name column
    _treeView->AppendColumn(_("Property"));
    _treeView->AppendColumn(_("Description"));

    // Listen for selection changes
    _treeView->Bind(wxEVT_TREELIST_SELECTION_CHANGED,
                    &AddPropertyDialog::_onSelectionChanged,
                    this);
}

namespace
{

// A wxImageList containing property type bitmaps
class PropertyImageList
{
    // Underlying wxImageList, allocated on the heap because the wxTreeListCtrl
    // will take ownership.
    wxImageList* _imageList;

    // Map storing wxImageList ids for types we have already added
    std::map<std::string, int> _imageListIDs;

public:

    // Construct and allocate image list
    PropertyImageList()
    : _imageList(new wxImageList)
    { }

    // Get the ID for the given bitmap, retrieving it from the
    // PropertyEditorFactory and storing it if necessary.
    int getImageIDForType(const std::string& type)
    {
        auto iter = _imageListIDs.find(type);
        if (iter == _imageListIDs.end())
        {
            // Retrieve image and cache in map
            wxBitmap bitmap = PropertyEditorFactory::getBitmapFor(type);
            int id = _imageList->Add(bitmap);
            _imageListIDs[type] = id;

            return id;
        }
        else
        {
            // Found in map, just return the id
            return iter->second;
        }
    }

    // Get the wxImageList pointer to transfer to the wxTreeListCtrl
    wxImageList* getImageList()
    {
        return _imageList;
    }
};

/* EntityClassAttributeVisitor instance to obtain custom properties from an
 * entityclass and add them into the provided GtkTreeStore under the provided
 * parent iter.
 */
class CustomPropertyAdder
{
    // Treestore to add to
    wxTreeListCtrl& _treeCtrl;

    // Parent iter
    wxTreeListItem _parent;

    // The entity we're adding the properties to
    Entity* _entity;

    // Image list of property type bitmaps
    PropertyImageList& _imageList;

public:

    // Constructor sets tree stuff
    CustomPropertyAdder(Entity* entity, wxTreeListCtrl& ctrl,
                        const wxTreeListItem& parent,
                        PropertyImageList& imageList)
    : _treeCtrl(ctrl), _parent(parent), _entity(entity), _imageList(imageList)
    { }

    // Required visit function
    void operator() (const EntityClassAttribute& attr)
    {
        // greebo: Only add the property if it hasn't been set directly on the
        // entity itself.
        if (!_entity->getKeyValue(attr.getName()).empty()
            && !_entity->isInherited(attr.getName()))
        {
            return;
        }

        // Also ignore all attributes with empty descriptions
        if (attr.getDescription().empty())
        {
            return;
        }

        // Add row, populating name and image
        wxTreeListItem item = _treeCtrl.AppendItem(
            _parent, attr.getName(),
            _imageList.getImageIDForType(attr.getType())
        );
        _treeCtrl.SetItemText(item, DESCRIPTION_COLUMN, attr.getDescription());
    }
};

} // namespace

// Populate tree view
void AddPropertyDialog::populateTreeView()
{
    // Construct the image list and add the folder icon (which is not
    // associated with any property type);
    PropertyImageList imageList;
    int folderImageID = imageList.getImageList()->Add(
        wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + FOLDER_ICON)
    );

    // DEF-DEFINED PROPERTIES
    {
        // First add a top-level category named after the entity class, and
        // populate it with custom keyvals defined in the DEF for that class
        std::string cName = _entity->getEntityClass()->getName();

        wxTreeListItem defRoot = _treeView->AppendItem(_treeView->GetRootItem(),
                                                       cName, folderImageID);
        _treeView->SetItemText(defRoot, DESCRIPTION_COLUMN,
                               _(CUSTOM_PROPERTY_TEXT));

        // Use a CustomPropertyAdder class to visit the entityclass and add all
        // custom properties from it
        CustomPropertyAdder adder(_entity, *_treeView, defRoot, imageList);
        _entity->getEntityClass()->forEachClassAttribute(boost::ref(adder));
    }

    // REGISTRY (GAME FILE) DEFINED PROPERTIES 

    // Ask the XML registry for the list of properties
    game::IGamePtr currentGame = GlobalGameManager().currentGame();
    xml::NodeList propNodes = currentGame->getLocalXPath(PROPERTIES_XPATH);

    // Cache of property categories to wxTreeListItems, to allow properties to
    // be parented to top-level categories
    typedef std::map<std::string, wxTreeListItem> CategoryMap;
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

        wxTreeListItem item;
        std::string name = iter->getAttributeValue("match");

        // If this property has a category, look up the top-level parent iter
        // or add it if necessary.
        std::string category = iter->getAttributeValue("category");

        wxTreeListItem parent = _treeView->GetRootItem();
        if (!category.empty())
        {
            CategoryMap::iterator mIter = categories.find(category);

            if (mIter == categories.end())
            {
                // Not found, add to treestore
                wxTreeListItem catRow = _treeView->AppendItem(
                    _treeView->GetRootItem(), category, folderImageID
                );

                // Add to map
                mIter = categories.insert(
                    CategoryMap::value_type(category, catRow)
                ).first;
            }

            parent = mIter->second;
        }

        // Add the property item itself
        item = _treeView->AppendItem(
            parent, name,
            imageList.getImageIDForType(iter->getAttributeValue("type"))
        );
        _treeView->SetItemText(item, DESCRIPTION_COLUMN, iter->getContent());
    }

    // Pass the image list to the tree ctrl. The wx docs do not make clear
    // whether it is valid to pass the image list AFTER adding rows with image
    // IDs, so hopefully this will work reliably on all platforms.
    _treeView->AssignImageList(imageList.getImageList());
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
        wxTreeListItems items;
        _treeView->GetSelections(items);

        if (!items.empty())
        {
            wxString desc = _treeView->GetItemText(items.front(),
                                                   DESCRIPTION_COLUMN);
            usageText->SetValue(desc);
            usageText->Enable(true);
        }
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

void AddPropertyDialog::_onSelectionChanged(wxTreeListEvent& ev)
{
    _selectedProperties.clear();

    // Add selected properties to _selectedProperties list
    wxTreeListItems items;
    if (_treeView->GetSelections(items) > 0)
    {
        std::for_each(
            items.begin(), items.end(),
            [&] (const wxTreeListItem& item)
            {
                wxString name = _treeView->GetItemText(item, NAME_COLUMN);
                _selectedProperties.push_back(name.ToStdString());
            }
        );
    }

    updateUsagePanel();
}

} // namespace ui
