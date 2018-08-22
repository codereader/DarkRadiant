#include "EntityInspector.h"
#include "PropertyEditorFactory.h"
#include "AddPropertyDialog.h"

#include "i18n.h"
#include "ientity.h"
#include "ieclass.h"
#include "iregistry.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "igame.h"
#include "iundo.h"
#include "igroupdialog.h"
#include "imainframe.h"
#include "itextstream.h"

#include "modulesystem/StaticModule.h"
#include "selectionlib.h"
#include "scenelib.h"
#include "wxutil/dialog/MessageBox.h"
#include "wxutil/menu/IconTextMenuItem.h"
#include "wxutil/TreeModel.h"
#include "xmlutil/Document.h"
#include "map/Map.h"
#include "selection/algorithm/Entity.h"
#include "selection/algorithm/General.h"

#include <map>
#include <string>

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/frame.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/splitter.h>
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/artprov.h>

#include <functional>
#include "string/replace.h"
#include <regex>

namespace ui {

/* CONSTANTS */

namespace
{
    const char* const PROPERTY_NODES_XPATH = "/entityInspector//property";

    const std::string RKEY_ROOT = "user/ui/entityInspector/";
    const std::string RKEY_PANE_STATE = RKEY_ROOT + "pane";

    const std::string HELP_ICON_NAME = "helpicon.png";
}

EntityInspector::EntityInspector() :
    _mainWidget(NULL),
    _editorFrame(NULL),
    _showInheritedCheckbox(NULL),
    _showHelpColumnCheckbox(NULL),
    _primitiveNumLabel(NULL),
    _keyValueTreeView(NULL),
    _helpColumn(NULL),
    _keyEntry(NULL),
    _valEntry(NULL)
{}

namespace
{
    wxVariant HELP_ICON()
    {
        static wxBitmap _helpBitmap = wxArtProvider::GetBitmap(
            GlobalUIManager().ArtIdPrefix() + HELP_ICON_NAME
        );
        wxASSERT(_helpBitmap.IsOk());

        return wxVariant(_helpBitmap);
    }

    wxVariant BLANK_ICON()
    {
        static const char* EMPTY_XPM[] = { "1 1 1 1", "* c none", "*" };
        return wxVariant(wxBitmap(EMPTY_XPM));
    }
}

void EntityInspector::construct()
{
    _emptyIcon.CopyFromBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "empty.png"));
    wxASSERT(_emptyIcon.IsOk());

    wxFrame* temporaryParent = new wxFrame(NULL, wxID_ANY, "");

    _mainWidget = new wxPanel(temporaryParent, wxID_ANY);
    _mainWidget->SetName("EntityInspector");
    _mainWidget->SetSizer(new wxBoxSizer(wxVERTICAL));

    // Construct HBox with the display checkboxes
    wxBoxSizer* optionsHBox = new wxBoxSizer(wxHORIZONTAL);

    _showInheritedCheckbox = new wxCheckBox(_mainWidget, wxID_ANY, _("Show inherited properties"));
    _showInheritedCheckbox->Connect(wxEVT_CHECKBOX, 
        wxCommandEventHandler(EntityInspector::_onToggleShowInherited), NULL, this);
    
    _showHelpColumnCheckbox = new wxCheckBox(_mainWidget, wxID_ANY, _("Show help"));
    _showHelpColumnCheckbox->SetValue(false);
    _showHelpColumnCheckbox->Connect(wxEVT_CHECKBOX, 
        wxCommandEventHandler(EntityInspector::_onToggleShowHelpIcons), NULL, this);

    _primitiveNumLabel = new wxStaticText(_mainWidget, wxID_ANY, "",
                                          wxDefaultPosition, wxDefaultSize);
    optionsHBox->Add(_primitiveNumLabel, 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);
    optionsHBox->AddStretchSpacer();
    optionsHBox->Add(_showInheritedCheckbox, 1, wxEXPAND);
    optionsHBox->Add(_showHelpColumnCheckbox, 0, wxEXPAND);

    // Pane with treeview and editor panel
    _paned = new wxSplitterWindow(_mainWidget, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);
    _paned->SetMinimumPaneSize(80);

    _paned->SplitHorizontally(createTreeViewPane(_paned), createPropertyEditorPane(_paned));
    _panedPosition.connect(_paned);

    _helpText = new wxTextCtrl(_mainWidget, wxID_ANY, "", 
        wxDefaultPosition, wxDefaultSize, wxTE_LEFT | wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
    _helpText->SetMinClientSize(wxSize(-1, 60));
    
    _mainWidget->GetSizer()->Add(optionsHBox, 0, wxEXPAND | wxALL, 3);
    _mainWidget->GetSizer()->Add(_paned, 1, wxEXPAND);
    _mainWidget->GetSizer()->Add(_helpText, 0, wxEXPAND);

    _helpText->Hide();
    
    // Reload the information from the registry
    restoreSettings();

    // Create the context menu
    createContextMenu();

    // Stimulate initial redraw to get the correct status
    requestIdleCallback();

    // Register self to the SelectionSystem to get notified upon selection
    // changes.
    GlobalSelectionSystem().addObserver(this);

    // Observe the Undo system for undo/redo operations, to refresh the
    // keyvalues when this happens
    _undoHandler = GlobalUndoSystem().signal_postUndo().connect(
        sigc::mem_fun(this, &EntityInspector::onUndoRedoOperation));
    _redoHandler = GlobalUndoSystem().signal_postRedo().connect(
        sigc::mem_fun(this, &EntityInspector::onUndoRedoOperation));

    // initialise the properties
    loadPropertyMap();
}

void EntityInspector::restoreSettings()
{
    // Find the information stored in the registry
    if (GlobalRegistry().keyExists(RKEY_PANE_STATE))
    {
        _panedPosition.loadFromPath(RKEY_PANE_STATE);
    }
    else
    {
        // No saved information, apply standard value
        _panedPosition.setPosition(400);
    }
}

// Entity::Observer implementation

void EntityInspector::onKeyInsert(const std::string& key,
                                  EntityKeyValue& value)
{
    onKeyChange(key, value.get());
}

void EntityInspector::onKeyChange(const std::string& key,
                                  const std::string& value)
{
    wxDataViewItem keyValueIter;
    bool added = false;

    // Check if we already have an iter for this key (i.e. this is a
    // modification).
    TreeIterMap::const_iterator i = _keyValueIterMap.find(key);

    if (i != _keyValueIterMap.end())
    {
        keyValueIter = i->second;
    }
    else
    {
        // Append a new row to the list store and add it to the iter map
        keyValueIter = _kvStore->AddItem().getItem();
        _keyValueIterMap.insert(TreeIterMap::value_type(key, keyValueIter));

        added = true;
    }

    // Look up type for this key. First check the property parm map,
    // then the entity class itself. If nothing is found, leave blank.
    // Get the type for this key if it exists, and the options
    PropertyParms parms = getPropertyParmsForKey(key);

    assert(!_selectedEntity.expired());
    Entity* selectedEntity = Node_getEntity(_selectedEntity.lock());

    // Check the entityclass (which will return blank if not found)
    IEntityClassConstPtr eclass = selectedEntity->getEntityClass();
    const EntityClassAttribute& attr = eclass->getAttribute(key);

    if (parms.type.empty())
    {
        parms.type = attr.getType();
    }

    bool hasDescription = !attr.getDescription().empty();

    // Set the values for the row
    wxutil::TreeModel::Row row(keyValueIter, *_kvStore);

    wxDataViewItemAttr black;
    black.SetColour(wxColor(0,0,0));

    wxIcon icon;
    icon.CopyFromBitmap(parms.type.empty() ? _emptyIcon : PropertyEditorFactory::getBitmapFor(parms.type));

    row[_columns.name] = wxVariant(wxDataViewIconText(key, icon));
    row[_columns.value] = value;

    if (parms.type == "bool")
    {
        // Render a checkbox for boolean values (store an actual bool)
        row[_columns.booleanValue] = value == "1";

        // Column is enabled by default after assignment
    }
    else
    {
        // Store false to render the checkbox as unchecked
        row[_columns.booleanValue] = false;
        row[_columns.booleanValue].setEnabled(false);
    }

    // Text colour
    row[_columns.name] = black;
    row[_columns.value] = black;

    row[_columns.isInherited] = false;
    row[_columns.hasHelpText] = hasDescription;
    row[_columns.helpIcon] = hasDescription ? HELP_ICON() : BLANK_ICON();

    if (added)
    {
        row.SendItemAdded();
    }
    else
    {
        row.SendItemChanged();
    }

    // Check if we should update the key/value entry boxes
    std::string curKey = _keyEntry->GetValue().ToStdString();
    std::string selectedKey = getSelectedKey();

    // If the key in the entry box matches the key which got changed,
    // update the value accordingly, otherwise leave it alone. This is to fix
    // the entry boxes not being updated when a PropertyEditor is changing the value.
    // Therefore only do this if the selectedKey is matching too.
    if (curKey == key && selectedKey == key)
    {
        _valEntry->SetValue(value);
    }

    // Also update the property editor if the changed key is highlighted
    if (_currentPropertyEditor && key == selectedKey)
    {
        _currentPropertyEditor->updateFromEntity();
    }
}

void EntityInspector::onKeyErase(const std::string& key,
                                 EntityKeyValue& value)
{
    // Look up iter in the TreeIter map, and delete it from the list store
    TreeIterMap::iterator i = _keyValueIterMap.find(key);
    if (i != _keyValueIterMap.end())
    {
        // Erase row from tree store
        _kvStore->RemoveItem(i->second);

        // Erase iter from iter map
        _keyValueIterMap.erase(i);
    }
    else
    {
        rConsoleError() << "EntityInspector: warning: removed key '" << key
                  << "' not found in map." << std::endl;
    }
}

// Create the context menu
void EntityInspector::createContextMenu()
{
    _contextMenu.reset(new wxutil::PopupMenu);

    _contextMenu->addItem(
        new wxutil::StockIconTextMenuItem(_("Add property..."), wxART_PLUS),
        std::bind(&EntityInspector::_onAddKey, this),
        std::bind(&EntityInspector::_testAddKey, this)
    );
    _contextMenu->addItem(
        new wxutil::StockIconTextMenuItem(_("Delete property"), wxART_MINUS),
        std::bind(&EntityInspector::_onDeleteKey, this),
        std::bind(&EntityInspector::_testDeleteKey, this)
    );

    _contextMenu->addSeparator();

    _contextMenu->addItem(
        new wxutil::StockIconTextMenuItem(_("Copy Spawnarg(s)"), wxART_COPY),
        std::bind(&EntityInspector::_onCopyKey, this),
        std::bind(&EntityInspector::_testCopyKey, this)
    );
    _contextMenu->addItem(
        new wxutil::StockIconTextMenuItem(_("Cut Spawnarg(s)"), wxART_CUT),
        std::bind(&EntityInspector::_onCutKey, this),
        std::bind(&EntityInspector::_testCutKey, this)
    );
    _contextMenu->addItem(
        new wxutil::StockIconTextMenuItem(_("Paste Spawnarg(s)"), wxART_PASTE),
        std::bind(&EntityInspector::_onPasteKey, this),
        std::bind(&EntityInspector::_testPasteKey, this)
    );
}

void EntityInspector::onRadiantStartup()
{
    // Add entity inspector to the group dialog
    IGroupDialog::PagePtr page(new IGroupDialog::Page);

    page->name = "entity";
    page->windowLabel = _("Entity");
    page->page = getWidget();
    page->tabIcon = "cmenu_add_entity.png";
    page->tabLabel = _("Entity");
    page->position = IGroupDialog::Page::Position::EntityInspector;

    GlobalGroupDialog().addPage(page);
}

void EntityInspector::onRadiantShutdown()
{
    _undoHandler.disconnect();
    _redoHandler.disconnect();

    // Remove all previously stored pane information
    _panedPosition.saveToPath(RKEY_PANE_STATE);

    // Remove the current property editor to prevent destructors
    // from firing too late in the shutdown process
    _currentPropertyEditor.reset();
}

void EntityInspector::onUndoRedoOperation()
{
    // Clear the previous entity (detaches this class as observer)
    changeSelectedEntity(nullptr);

    // Now rescan the selection and update the stores
    requestIdleCallback();
}

const std::string& EntityInspector::getName() const
{
    static std::string _name(MODULE_ENTITYINSPECTOR);
    return _name;
}

const StringSet& EntityInspector::getDependencies() const
{
    static StringSet _dependencies;

    if (_dependencies.empty())
    {
        _dependencies.insert(MODULE_XMLREGISTRY);
        _dependencies.insert(MODULE_UIMANAGER);
        _dependencies.insert(MODULE_SELECTIONSYSTEM);
        _dependencies.insert(MODULE_UNDOSYSTEM);
        _dependencies.insert(MODULE_GAMEMANAGER);
        _dependencies.insert(MODULE_COMMANDSYSTEM);
        _dependencies.insert(MODULE_EVENTMANAGER);
        _dependencies.insert(MODULE_MAINFRAME);
    }

    return _dependencies;
}

void EntityInspector::initialiseModule(const ApplicationContext& ctx)
{
    construct();

    GlobalRadiant().signal_radiantStarted().connect(
        sigc::mem_fun(this, &EntityInspector::onRadiantStartup)
    );
    GlobalRadiant().signal_radiantShutdown().connect(
        sigc::mem_fun(this, &EntityInspector::onRadiantShutdown)
    );

    GlobalCommandSystem().addCommand("ToggleEntityInspector", toggle);
    GlobalEventManager().addCommand("ToggleEntityInspector", "ToggleEntityInspector");
}

void EntityInspector::registerPropertyEditor(const std::string& key, const IPropertyEditorPtr& editor)
{
    PropertyEditorFactory::registerPropertyEditor(key, editor);
}

IPropertyEditorPtr EntityInspector::getRegisteredPropertyEditor(const std::string& key)
{
    return PropertyEditorFactory::getRegisteredPropertyEditor(key);
}

void EntityInspector::unregisterPropertyEditor(const std::string& key)
{
    PropertyEditorFactory::unregisterPropertyEditor(key);
}

// Return the Gtk widget for the EntityInspector dialog.

wxPanel* EntityInspector::getWidget()
{
    return _mainWidget;
}

// Create the dialog pane
wxWindow* EntityInspector::createPropertyEditorPane(wxWindow* parent)
{
    _editorFrame = new wxPanel(parent, wxID_ANY);
    _editorFrame->SetSizer(new wxBoxSizer(wxVERTICAL));
    _editorFrame->SetMinClientSize(wxSize(-1, 50));
    return _editorFrame;
}

// Create the TreeView pane

wxWindow* EntityInspector::createTreeViewPane(wxWindow* parent)
{
    wxPanel* treeViewPanel = new wxPanel(parent, wxID_ANY);
    treeViewPanel->SetSizer(new wxBoxSizer(wxVERTICAL));
    treeViewPanel->SetMinClientSize(wxSize(-1, 150));

    _kvStore = new wxutil::TreeModel(_columns, true); // this is a list model

    _keyValueTreeView = wxutil::TreeView::CreateWithModel(treeViewPanel, _kvStore, wxDV_MULTIPLE);

    // Search in both name and value columns
    _keyValueTreeView->AddSearchColumn(_columns.name);
    _keyValueTreeView->AddSearchColumn(_columns.value);

    // Add the checkbox for boolean properties
    _keyValueTreeView->AppendToggleColumn("", _columns.booleanValue.getColumnIndex(),
        wxDATAVIEW_CELL_ACTIVATABLE, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);

    // Create the Property column (has an icon)
    _keyValueTreeView->AppendIconTextColumn(_("Property"), 
        _columns.name.getColumnIndex(), wxDATAVIEW_CELL_INERT, 
        wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    // Create the value column
    _keyValueTreeView->AppendTextColumn(_("Value"), 
        _columns.value.getColumnIndex(), wxDATAVIEW_CELL_INERT, 
        wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    // Add the help icon
    _helpColumn = _keyValueTreeView->AppendBitmapColumn(_("?"), 
        _columns.helpIcon.getColumnIndex(), wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);
    _helpColumn->SetHidden(true);

    // Used to update the help text
    _keyValueTreeView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
        wxDataViewEventHandler(EntityInspector::_onTreeViewSelectionChanged), NULL, this);
    _keyValueTreeView->Connect(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, 
        wxDataViewEventHandler(EntityInspector::_onContextMenu), NULL, this);

    // When the toggle column is clicked to check/uncheck the box, the model's column value
    // is directly changed by the wxWidgets event handlers. On model value change, this event is fired afterwards
    _keyValueTreeView->Connect(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED,
        wxDataViewEventHandler(EntityInspector::_onDataViewItemChanged), NULL, this);

    wxBoxSizer* buttonHbox = new wxBoxSizer(wxHORIZONTAL);

    // Pack in the key and value edit boxes
    _keyEntry = new wxTextCtrl(treeViewPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    _valEntry = new wxTextCtrl(treeViewPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);

    wxBitmap icon = wxArtProvider::GetBitmap(wxART_TICK_MARK, wxART_MENU);
    wxBitmapButton* setButton = new wxBitmapButton(treeViewPanel, wxID_APPLY, icon);

    buttonHbox->Add(_valEntry, 1, wxEXPAND);
    buttonHbox->Add(setButton, 0, wxEXPAND);

    treeViewPanel->GetSizer()->Add(_keyValueTreeView, 1, wxEXPAND);
    treeViewPanel->GetSizer()->Add(_keyEntry, 0, wxEXPAND);
    treeViewPanel->GetSizer()->Add(buttonHbox, 0, wxEXPAND);

    setButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(EntityInspector::_onSetProperty), NULL, this);
    _keyEntry->Connect(wxEVT_TEXT_ENTER, wxCommandEventHandler(EntityInspector::_onEntryActivate), NULL, this);
    _valEntry->Connect(wxEVT_TEXT_ENTER, wxCommandEventHandler(EntityInspector::_onEntryActivate), NULL, this);

    return treeViewPanel;
}

// Retrieve the selected string from the given property in the list store

std::string EntityInspector::getSelectedKey()
{
    wxDataViewItemArray selectedItems;
    if (_keyValueTreeView->GetSelections(selectedItems) != 1)
    {
        // Multiple or nothing selected, return empty string
        return std::string();
    }

    const wxDataViewItem& item = selectedItems.front();

    if (!item.IsOk()) return "";

    wxutil::TreeModel::Row row(item, *_keyValueTreeView->GetModel());

    wxDataViewIconText iconAndName = static_cast<wxDataViewIconText>(row[_columns.name]);
    return iconAndName.GetText().ToStdString();
}

std::string EntityInspector::getListSelection(const wxutil::TreeModel::Column& col)
{
    wxDataViewItemArray selectedItems;
    if (_keyValueTreeView->GetSelections(selectedItems) != 1)
    {
        // Multiple or nothing selected, return empty string
        return std::string();
    }

    const wxDataViewItem& item = selectedItems.front();

    if (!item.IsOk()) return "";

    wxutil::TreeModel::Row row(item, *_keyValueTreeView->GetModel());

    return row[col];
}

bool EntityInspector::getListSelectionBool(const wxutil::TreeModel::Column& col)
{
    wxDataViewItemArray selectedItems;
    if (_keyValueTreeView->GetSelections(selectedItems) != 1)
    {
        // Multiple or nothing selected, return false
        return false;
    }

    const wxDataViewItem& item = selectedItems.front();

    if (!item.IsOk()) return false;

    wxutil::TreeModel::Row row(item, *_keyValueTreeView->GetModel());

    return row[col].getBool();
}

// Redraw the GUI elements
void EntityInspector::updateGUIElements()
{
    // Update from selection system
    getEntityFromSelectionSystem();

    if (!_selectedEntity.expired())
    {
        _editorFrame->Enable(true);
        _keyValueTreeView->Enable(true);
        _showInheritedCheckbox->Enable(true);
        _showHelpColumnCheckbox->Enable(true);
    }
    else  // no selected entity
    {
        // Remove the displayed PropertyEditor
        if (_currentPropertyEditor)
        {
            _currentPropertyEditor = PropertyEditorPtr();
        }

        _helpText->SetValue("");

        // Disable the dialog and clear the TreeView
        _editorFrame->Enable(false);
        _keyValueTreeView->Enable(true); // leave the treeview enabled
        _showInheritedCheckbox->Enable(false);
        _showHelpColumnCheckbox->Enable(false);
    }
}

void EntityInspector::onIdle()
{
    updateGUIElements();
}

// Selection changed callback
void EntityInspector::selectionChanged(const scene::INodePtr& node, bool isComponent)
{
    requestIdleCallback();
}

std::string EntityInspector::cleanInputString(const std::string &input)
{
    std::string ret = input;

    string::replace_all(ret, "\n", "");
    string::replace_all(ret, "\r", "");

    return ret;
}

// Set entity property from entry boxes
void EntityInspector::setPropertyFromEntries()
{
    // Get the key from the entry box
    std::string key = cleanInputString(_keyEntry->GetValue().ToStdString());
    std::string val = cleanInputString(_valEntry->GetValue().ToStdString());

    // Update the entry boxes
    _keyEntry->SetValue(key);
    _valEntry->SetValue(val);

    UndoableCommand cmd("entitySetProperty");

    // Pass the call to the specialised routine
    applyKeyValueToSelection(key, val);
}

void EntityInspector::applyKeyValueToSelection(const std::string& key, const std::string& val)
{
    try
    {
        selection::algorithm::setEntityKeyvalue(key, val);
    }
    catch (std::runtime_error& ex)
    {
        wxutil::Messagebox::ShowError(ex.what());
    }
}

void EntityInspector::loadPropertyMap()
{
    _propertyTypes.clear();

    xml::NodeList pNodes = GlobalGameManager().currentGame()->getLocalXPath(PROPERTY_NODES_XPATH);

    for (xml::NodeList::const_iterator iter = pNodes.begin();
         iter != pNodes.end();
         ++iter)
    {
        PropertyParms parms;
        parms.type = iter->getAttributeValue("type");
        parms.options = iter->getAttributeValue("options");

        _propertyTypes.insert(PropertyParmMap::value_type(
            iter->getAttributeValue("match"), parms)
        );
    }
}

// Popup menu callbacks (see wxutil::PopupMenu)

void EntityInspector::_onAddKey()
{
    assert(!_selectedEntity.expired());

    Entity* selectedEntity = Node_getEntity(_selectedEntity.lock());

    // Obtain the entity class to provide to the AddPropertyDialog
    IEntityClassConstPtr ec = selectedEntity->getEntityClass();

    // Choose a property, and add to entity with a default value
    AddPropertyDialog::PropertyList properties = AddPropertyDialog::chooseProperty(selectedEntity);

    for (std::size_t i = 0; i < properties.size(); ++i)
    {
        const std::string& key = properties[i];

        // Add all keys, skipping existing ones to not overwrite any values on the entity
        if (selectedEntity->getKeyValue(key) == "" || selectedEntity->isInherited(key))
        {
            // Add the keyvalue on the entity (triggering the refresh)
            selectedEntity->setKeyValue(key, "-");
        }
    }
}

bool EntityInspector::_testAddKey()
{
    return !_selectedEntity.expired();
}

void EntityInspector::_onDeleteKey()
{
    assert(!_selectedEntity.expired());

    std::string key = getSelectedKey();

    if (!key.empty())
    {
        UndoableCommand cmd("deleteProperty");
        
        Entity* selectedEntity = Node_getEntity(_selectedEntity.lock());
        selectedEntity->setKeyValue(key, "");
    }
}

bool EntityInspector::_testDeleteKey()
{
    if (_selectedEntity.expired()) return false;

    if (getSelectedKey().empty()) return false;

    // Make sure the Delete item is only available for explicit
    // (non-inherited) properties
    if (getListSelectionBool(_columns.isInherited) == false)
        return true;
    else
        return false;
}

void EntityInspector::_onCopyKey()
{
    wxDataViewItemArray selectedItems;
    _keyValueTreeView->GetSelections(selectedItems);

    if (selectedItems.Count() == 0) return;

    _clipBoard.clear();

    for (const wxDataViewItem& item : selectedItems)
    {
        wxutil::TreeModel::Row row(item, *_kvStore);

        wxDataViewIconText iconAndName = static_cast<wxDataViewIconText>(row[_columns.name]);

        std::string key = iconAndName.GetText().ToStdString();
        std::string value = row[_columns.value];

        _clipBoard.push_back(std::make_pair(key, value));
    }
#if 0
    std::string key = getSelectedKey();
    std::string value = getListSelection(_columns.value);

    if (!key.empty())
    {
        _clipBoard.key = key;
        _clipBoard.value = value;
    }
#endif
}

bool EntityInspector::_testCopyKey()
{
    return _keyValueTreeView->HasSelection();
}

void EntityInspector::_onCutKey()
{
    wxDataViewItemArray selectedItems;
    _keyValueTreeView->GetSelections(selectedItems);

    if (selectedItems.Count() == 0) return;

    assert(!_selectedEntity.expired());
    Entity* selectedEntity = Node_getEntity(_selectedEntity.lock());

    _clipBoard.clear();

    for (const wxDataViewItem& item : selectedItems)
    {
        wxutil::TreeModel::Row row(item, *_kvStore);

        wxDataViewIconText iconAndName = static_cast<wxDataViewIconText>(row[_columns.name]);

        std::string key = iconAndName.GetText().ToStdString();
        std::string value = row[_columns.value];

        _clipBoard.push_back(std::make_pair(key, value));

        // We don't delete any inherited key values
        if (row[_columns.isInherited].getBool()) continue;

        UndoableCommand cmd("cutProperty");

        // Clear the key after copying
        selectedEntity->setKeyValue(key, "");
    }

#if 0
    std::string key = getSelectedKey();
    std::string value = getListSelection(_columns.value);

    assert(!_selectedEntity.expired());
    Entity* selectedEntity = Node_getEntity(_selectedEntity.lock());

    if (!key.empty() && selectedEntity != NULL)
    {
        _clipBoard.key = key;
        _clipBoard.value = value;

        UndoableCommand cmd("cutProperty");

        // Clear the key after copying
        selectedEntity->setKeyValue(key, "");
    }
#endif
}

bool EntityInspector::_testCutKey()
{
    wxDataViewItemArray selectedItems;
    _keyValueTreeView->GetSelections(selectedItems);

    for (const wxDataViewItem& item : selectedItems)
    {
        wxutil::TreeModel::Row row(item, *_kvStore);

        if (!row[_columns.isInherited].getBool())
        {
            return true; // we have at least one non-inherited value
        }
    }

    return false;
#if 0
    // Make sure the Delete item is only available for explicit
    // (non-inherited) properties
    if (getListSelectionBool(_columns.isInherited) == false)
    {
        // return true only if selection is not empty
        return !getSelectedKey().empty();
    }
    else
    {
        return false;
    }
#endif
}

void EntityInspector::_onPasteKey()
{
    // greebo: Instantiate a scoped object to make this operation undoable
    UndoableCommand command("entitySetProperties");

    for (const KeyValuePair& kv : _clipBoard)
    {
        // skip empty entries
        if (kv.first.empty() || kv.second.empty()) continue;

        // Pass the call
        applyKeyValueToSelection(kv.first, kv.second);
    }
}

bool EntityInspector::_testPasteKey()
{
    if (GlobalSelectionSystem().getSelectionInfo().entityCount == 0)
    {
        // No entities selected
        return false;
    }

    // Return true if the clipboard contains data
    return !_clipBoard.empty();
}

// wxWidget callbacks

void EntityInspector::_onContextMenu(wxDataViewEvent& ev)
{
    _contextMenu->show(_keyValueTreeView);
}

void EntityInspector::_onDataViewItemChanged(wxDataViewEvent& ev)
{
    if (ev.GetDataViewColumn() != nullptr && 
        static_cast<int>(ev.GetDataViewColumn()->GetModelColumn()) == _columns.booleanValue.getColumnIndex())
    {
        // Model value in the boolean column has changed, this means
        // the user has clicked the checkbox, send the value to the entity/entities
        wxutil::TreeModel::Row row(ev.GetItem(), *_kvStore);

        wxDataViewIconText iconAndName = static_cast<wxDataViewIconText>(row[_columns.name]);

        std::string key = iconAndName.GetText().ToStdString();
        bool updatedValue = row[_columns.booleanValue].getBool();

        UndoableCommand cmd("entitySetProperty");
        applyKeyValueToSelection(key, updatedValue ? "1" : "0");
        
        // Check if the property was an inherited one.
        // The applyKeyValue function produced a non-inherited entry
        // which should be visible once we're done
        // Note: selecting the non-inherited property instead of this one
        // is not as easy as it may appear, since the user is yet to release
        // the mouse button (we're in the middle of the click event here)
        // and the MouseUp handler will select this row again
        if (row[_columns.isInherited].getBool())
        {
            _kvStore->ForeachNode([&](wxutil::TreeModel::Row& row)
            {
                wxDataViewIconText nameVal = static_cast<wxDataViewIconText>(row[_columns.name]);

                if (nameVal.GetText() == key && !row[_columns.isInherited].getBool())
                {
                    _keyValueTreeView->EnsureVisible(row.getItem());
                }
            });
        }
    }
}

void EntityInspector::_onSetProperty(wxCommandEvent& ev)
{
    setPropertyFromEntries();
}

// ENTER key in entry boxes
void EntityInspector::_onEntryActivate(wxCommandEvent& ev)
{
    // Set property and move back to key entry
    setPropertyFromEntries();
    _keyEntry->SetFocus();
}

void EntityInspector::_onToggleShowInherited(wxCommandEvent& ev)
{
    if (_showInheritedCheckbox->IsChecked())
    {
        addClassProperties();
    }
    else
    {
        removeClassProperties();
    }
}

void EntityInspector::_onToggleShowHelpIcons(wxCommandEvent& ev)
{
    // Set the visibility of the column accordingly
    _helpColumn->SetHidden(!_showHelpColumnCheckbox->IsChecked());
    _helpText->Show(_showHelpColumnCheckbox->IsChecked());

    // After showing a packed control we need to call the sizer's layout() method
    _mainWidget->GetSizer()->Layout();
}

void EntityInspector::updateHelpText(const wxutil::TreeModel::Row& row)
{
    _helpText->SetValue("");

    if (!row.getItem().IsOk()) return;

    // Get the key pointed at
    bool hasHelp = row[_columns.hasHelpText].getBool();

    if (hasHelp)
    {
        std::string key = getSelectedKey();

        assert(!_selectedEntity.expired());
        Entity* selectedEntity = Node_getEntity(_selectedEntity.lock());

        IEntityClassConstPtr eclass = selectedEntity->getEntityClass();
        assert(eclass != NULL);

        // Find the attribute on the eclass, that's where the descriptions are defined
        const EntityClassAttribute& attr = eclass->getAttribute(key);

        if (!attr.getDescription().empty())
        {
            // Check the description of the focused item
            _helpText->SetValue(attr.getDescription());
        }
    }
}

// Update the PropertyEditor pane, displaying the PropertyEditor if necessary
// and making sure it refers to the currently-selected Entity.
void EntityInspector::_onTreeViewSelectionChanged(wxDataViewEvent& ev)
{
    ev.Skip();

    // Abort if called without a valid entity selection (may happen during
    // various cleanup operations).
    if (_selectedEntity.expired()) return;

    wxDataViewItemArray selectedItems;
    _keyValueTreeView->GetSelections(selectedItems);

    if (selectedItems.Count() == 1)
    {
        wxDataViewItem selectedItem = selectedItems.front();

        wxutil::TreeModel::Row row(selectedItem, *_kvStore);

        if (_showHelpColumnCheckbox->IsChecked())
        {
            updateHelpText(row);
        }

        // Don't go further without a proper tree selection
        if (!selectedItem.IsOk()) return;

        // Get the selected key and value in the tree view
        std::string key = getSelectedKey();
        std::string value = getListSelection(_columns.value);

        // Get the type for this key if it exists, and the options
        PropertyParms parms = getPropertyParmsForKey(key);

        Entity* selectedEntity = Node_getEntity(_selectedEntity.lock());

        // If the type was not found, also try looking on the entity class
        if (parms.type.empty())
        {
            IEntityClassConstPtr eclass = selectedEntity->getEntityClass();
            parms.type = eclass->getAttribute(key).getType();
        }

        // Construct and add a new PropertyEditor
        _currentPropertyEditor = PropertyEditorFactory::create(_editorFrame,
            parms.type, selectedEntity, key, parms.options);

        if (_currentPropertyEditor)
        {
            // Don't use wxEXPAND to allow for horizontal centering, just add a 6 pixel border
            // Using wxALIGN_CENTER_HORIZONTAL will position the property editor's panel in the middle
            _editorFrame->GetSizer()->Add(_currentPropertyEditor->getWidget(), 1, wxALIGN_CENTER_HORIZONTAL | wxALL, 6);
            _editorFrame->GetSizer()->Layout();
        }

        // Update key and value entry boxes, but only if there is a key value. If
        // there is no selection we do not clear the boxes, to allow keyval copying
        // between entities.
        if (!key.empty())
        {
            _keyEntry->SetValue(key);
            _valEntry->SetValue(value);
        }
    }
    else if (selectedItems.Count() > 1)
    {
        // When multiple items are selected, clear the property editor
        _currentPropertyEditor.reset();
    }
}

EntityInspector::PropertyParms EntityInspector::getPropertyParmsForKey(
    const std::string& key
)
{
    PropertyParms returnValue;

    // First, attempt to find the key in the property map
    for (PropertyParmMap::const_iterator i = _propertyTypes.begin();
         i != _propertyTypes.end(); ++i)
    {
        if (i->first.empty()) continue; // safety check

        // Try to match the entity key against the regex (i->first)
        std::regex expr(i->first);
        std::smatch matches;

        if (!std::regex_match(key, matches, expr)) continue;

        // We have a match
        returnValue.type = i->second.type;
        returnValue.options = i->second.options;
    }

    return returnValue;
}

void EntityInspector::addClassAttribute(const EntityClassAttribute& a)
{
    // Only add properties with values, we don't want the optional
    // "editor_var xxx" properties here.
    if (!a.getValue().empty())
    {
        bool hasDescription = !a.getDescription().empty();

        wxutil::TreeModel::Row row = _kvStore->AddItem();

        wxDataViewItemAttr grey;
        grey.SetColour(wxColor(112, 112, 112));

        row[_columns.name] = wxVariant(wxDataViewIconText(a.getName(), _emptyIcon)); 
        row[_columns.value] = a.getValue();

        // Inherited values have an inactive checkbox, so assign a false value and disable
        if (a.getType() == "bool")
        {
            row[_columns.booleanValue] = a.getValue() == "1";
        }
        else
        {
            row[_columns.booleanValue] = false;
            row[_columns.booleanValue].setEnabled(false);
        }

        row[_columns.name] = grey;
        row[_columns.value] = grey;

        row[_columns.isInherited] = true;
        row[_columns.hasHelpText] = hasDescription;
        row[_columns.helpIcon] = hasDescription ? HELP_ICON() : BLANK_ICON();

        row.SendItemAdded();
    }
}

// Append inherited (entityclass) properties
void EntityInspector::addClassProperties()
{
    assert(!_selectedEntity.expired());
    Entity* selectedEntity = Node_getEntity(_selectedEntity.lock());

    // Get the entityclass for the current entity
    std::string className = selectedEntity->getKeyValue("classname");
    IEntityClassPtr eclass = GlobalEntityClassManager().findOrInsert(
        className, true
    );

    // Visit the entity class
    eclass->forEachClassAttribute(
        std::bind(&EntityInspector::addClassAttribute, this, std::placeholders::_1)
    );
}

// Remove the inherited properties
void EntityInspector::removeClassProperties()
{
    _kvStore->RemoveItems([&] (const wxutil::TreeModel::Row& row)->bool
    {
        // If this is an inherited row, remove it
        return row[_columns.isInherited].getBool();
    });
}

// Update the selected Entity pointer from the selection system
void EntityInspector::getEntityFromSelectionSystem()
{
    // A single entity must be selected
    if (GlobalSelectionSystem().countSelected() != 1)
    {
        changeSelectedEntity(scene::INodePtr());
        _primitiveNumLabel->SetLabelMarkup("");
        return;
    }

    scene::INodePtr selectedNode = GlobalSelectionSystem().ultimateSelected();

    // The root node must not be selected (this can happen if Invert Selection is
    // activated with an empty scene, or by direct selection in the entity list).
    if (selectedNode->isRoot())
    {
        changeSelectedEntity(scene::INodePtr());
        _primitiveNumLabel->SetLabelMarkup("");
        return;
    }

    // Try both the selected node (if an entity is selected) or the parent node
    // (if a brush is selected).
    Entity* newSelectedEntity = Node_getEntity(selectedNode);

    if (newSelectedEntity)
    {
        // Node was an entity, use this
        changeSelectedEntity(selectedNode);

        // Just set the entity number
        std::size_t ent(0), prim(0);
        selection::algorithm::getSelectionIndex(ent, prim);

        _primitiveNumLabel->SetLabelMarkup(fmt::format(_("<b>Entity {0}</b>"), ent));
    }
    else
    {
        // Node was not an entity, try parent instead
        scene::INodePtr selectedNodeParent = selectedNode->getParent();
        changeSelectedEntity(selectedNodeParent);
        
        std::size_t ent(0), prim(0);
        selection::algorithm::getSelectionIndex(ent, prim);

        _primitiveNumLabel->SetLabelMarkup(fmt::format(_("<b>Entity {0}, Primitive {1}</b>"), ent, prim));
    }
}

// Change selected entity pointer
void EntityInspector::changeSelectedEntity(const scene::INodePtr& newEntity)
{
    // Check what we need to do with the existing entity
    scene::INodePtr oldEntity = _selectedEntity.lock();

    if (oldEntity)
    {
        // The old entity still exists
        if (oldEntity != newEntity)
        {
            // Entity change, disconnect from previous entity
            Node_getEntity(oldEntity)->detachObserver(this);
            removeClassProperties();
        }
        else
        {
            // No change detected
            return;
        }
    }

    // At this point, we either disconnected from the old entity or
    // it has already been deleted (no disconnection necessary)
    _selectedEntity.reset();

    // Clear the view. If the old entity has been destroyed before we had 
    // a chance to disconnect the list store might contain remnants
    _keyValueIterMap.clear();
    _kvStore->Clear();

    // Reset the sorting when changing entities
    _keyValueTreeView->ResetSortingOnAllColumns();
    
    // Attach to new entity if it is non-NULL
    if (newEntity)
    {
        _selectedEntity = newEntity;

        // Attach as observer to fill the listview
        Node_getEntity(newEntity)->attachObserver(this);

        // Add inherited properties if the checkbox is set
        if (_showInheritedCheckbox->IsChecked())
        {
            addClassProperties();
        }
    }
}

void EntityInspector::toggle(const cmd::ArgumentList& args)
{
    GlobalGroupDialog().togglePage("entity");
}

// Define the static EntityInspector module
module::StaticModule<EntityInspector> entityInspectorModule;

} // namespace ui
