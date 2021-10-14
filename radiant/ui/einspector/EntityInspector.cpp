#include "EntityInspector.h"
#include "PropertyEditorFactory.h"
#include "AddPropertyDialog.h"

#include "i18n.h"
#include "ientity.h"
#include "ieclass.h"
#include "iregistry.h"
#include "igame.h"
#include "imap.h"
#include "iundo.h"
#include "ui/igroupdialog.h"
#include "imainframe.h"
#include "itextstream.h"

#include "module/StaticModule.h"
#include "selectionlib.h"
#include "scene/SelectionIndex.h"
#include "scenelib.h"
#include "eclass.h"
#include "wxutil/dialog/MessageBox.h"
#include "wxutil/menu/IconTextMenuItem.h"
#include "wxutil/dataview/TreeModel.h"
#include "wxutil/dataview/TreeViewItemStyle.h"
#include "xmlutil/Document.h"

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
#include "wxutil/Bitmap.h"

#include <functional>
#include "string/replace.h"
#include "registry/Widgets.h"
#include <regex>

namespace ui {

/* CONSTANTS */

namespace
{
    const char* const PROPERTY_NODES_XPATH = "/entityInspector//property";

    const std::string RKEY_ROOT = "user/ui/entityInspector/";
    const std::string RKEY_PANE_STATE = RKEY_ROOT + "pane";
    const std::string RKEY_SHOW_HELP_AREA = RKEY_ROOT + "showHelpArea";
    const std::string RKEY_SHOW_INHERITED_PROPERTIES = RKEY_ROOT + "showInheritedProperties";
}

EntityInspector::EntityInspector() :
    _mainWidget(nullptr),
    _editorFrame(nullptr),
    _showInheritedCheckbox(nullptr),
    _showHelpColumnCheckbox(nullptr),
    _primitiveNumLabel(nullptr),
    _keyValueTreeView(nullptr),
    _booleanColumn(nullptr),
    _valueColumn(nullptr),
    _oldValueColumn(nullptr),
    _newValueColumn(nullptr),
    _keyEntry(nullptr),
    _valEntry(nullptr),
    _setButton(nullptr)
{}

void EntityInspector::construct()
{
    _emptyIcon.CopyFromBitmap(wxutil::GetLocalBitmap("empty.png"));
    wxASSERT(_emptyIcon.IsOk());

    wxFrame* temporaryParent = new wxFrame(NULL, wxID_ANY, "");

    _mainWidget = new wxPanel(temporaryParent, wxID_ANY);
    _mainWidget->SetName("EntityInspector");
    _mainWidget->SetSizer(new wxBoxSizer(wxVERTICAL));

    // Construct HBox with the display checkboxes
    wxBoxSizer* optionsHBox = new wxBoxSizer(wxHORIZONTAL);

    _showInheritedCheckbox = new wxCheckBox(_mainWidget, wxID_ANY, _("Show inherited properties"));
    _showInheritedCheckbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& ev) {
        handleShowInheritedChanged();
    });

    _showHelpColumnCheckbox = new wxCheckBox(_mainWidget, wxID_ANY, _("Show help"));
    _showHelpColumnCheckbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& ev) {
        handleShowHelpTextChanged();
    });

    _primitiveNumLabel = new wxStaticText(_mainWidget, wxID_ANY, "", wxDefaultPosition, wxDefaultSize);
    _primitiveNumLabel->SetFont(_primitiveNumLabel->GetFont().Bold());

    optionsHBox->Add(_primitiveNumLabel, 1, wxEXPAND | wxALL, 5);
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

    // Connect to registry key and handle the loaded value
    registry::bindWidget(_showHelpColumnCheckbox, RKEY_SHOW_HELP_AREA);
    registry::bindWidget(_showInheritedCheckbox, RKEY_SHOW_INHERITED_PROPERTIES);

    handleShowInheritedChanged();
    handleShowHelpTextChanged();

    // Reload the information from the registry
    restoreSettings();

    // Create the context menu
    createContextMenu();

    // Stimulate initial redraw to get the correct status
    requestIdleCallback();

    // Register self to the SelectionSystem to get notified upon selection
    // changes.
    GlobalSelectionSystem().addObserver(this);
    
    _defsReloadedHandler = GlobalEntityClassManager().defsReloadedSignal().connect(
        sigc::mem_fun(this, &EntityInspector::onDefsReloaded)
    );

    // Observe the Undo system for undo/redo operations, to refresh the
    // keyvalues when this happens
    _undoHandler = GlobalUndoSystem().signal_postUndo().connect(
        sigc::mem_fun(this, &EntityInspector::onUndoRedoOperation));
    _redoHandler = GlobalUndoSystem().signal_postRedo().connect(
        sigc::mem_fun(this, &EntityInspector::onUndoRedoOperation));

    _mapEditModeChangedHandler = GlobalMapModule().signal_editModeChanged().connect(
        sigc::mem_fun(this, &EntityInspector::onMapEditModeChanged)
    );

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

void EntityInspector::onMapEditModeChanged(IMap::EditMode mode)
{
    // Clear the selection to not hold any references to merge nodes
    changeSelectedEntity(scene::INodePtr(), scene::INodePtr());
    requestIdleCallback();
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
    auto i = _keyValueIterMap.find(key);

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

    wxDataViewItemAttr style;

    // Check if this key is affected by a merge operation
    auto action = _mergeActions.find(key);

    if (action != _mergeActions.end())
    {
        switch (action->second->getType())
        {
        case scene::merge::ActionType::AddKeyValue:
            wxutil::TreeViewItemStyle::ApplyKeyValueAddedStyle(style);
            break;
        case scene::merge::ActionType::ChangeKeyValue:
            wxutil::TreeViewItemStyle::ApplyKeyValueChangedStyle(style);
            break;
        case scene::merge::ActionType::RemoveKeyValue:
            wxutil::TreeViewItemStyle::ApplyKeyValueRemovedStyle(style);
            break;
        }
    }

    // Conflicting actions get a special render style
    auto conflict = _conflictActions.find(key);

    if (conflict != _conflictActions.end() && conflict->second->getResolution() == scene::merge::ResolutionType::Unresolved)
    {
        wxutil::TreeViewItemStyle::ApplyKeyValueConflictStyle(style);
    }

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

    if (action != _mergeActions.end())
    {
        if (action->second->getType() == scene::merge::ActionType::AddKeyValue)
        {
            row[_columns.oldValue] = std::string(); // no old value to show
            row[_columns.oldValue] = style;
        }
        else
        {
            wxDataViewItemAttr oldAttr = style;
            wxutil::TreeViewItemStyle::SetStrikethrough(oldAttr, true);
            row[_columns.oldValue] = value;
            row[_columns.oldValue] = oldAttr;
        }

        row[_columns.newValue] = action->second->getValue();

        wxDataViewItemAttr newAttr = style;
        newAttr.SetBold(true);

        row[_columns.newValue] = newAttr;
    }
    else
    {
        row[_columns.oldValue] = std::string();
        row[_columns.newValue] = std::string();
    }

    // Apply background style to all other columns
    row[_columns.name] = style;
    row[_columns.value] = style;
    row[_columns.booleanValue] = style;

    row[_columns.isInherited] = false;
    row[_columns.hasHelpText] = key == "classname" || hasDescription; // classname always has a description

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

    _contextMenu->addSeparator();

    _contextMenu->addItem(
        new wxutil::StockIconTextMenuItem(_("Accept selected Changes"), wxART_TICK_MARK),
        std::bind(&EntityInspector::_onAcceptMergeAction, this),
        std::bind(&EntityInspector::_testAcceptMergeAction, this),
        [] { return GlobalMapModule().getEditMode() == IMap::EditMode::Merge; }
    );

    _contextMenu->addItem(
        new wxutil::StockIconTextMenuItem(_("Reject selected Changes"), wxART_UNDO),
        std::bind(&EntityInspector::_onRejectMergeAction, this),
        std::bind(&EntityInspector::_testRejectMergeAction, this),
        [] { return GlobalMapModule().getEditMode() == IMap::EditMode::Merge; }
    );
}

void EntityInspector::onMainFrameConstructed()
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

void EntityInspector::onMainFrameShuttingDown()
{
    _mergeActions.clear();
    _conflictActions.clear();

    _mapEditModeChangedHandler.disconnect();
    _undoHandler.disconnect();
    _redoHandler.disconnect();
    _defsReloadedHandler.disconnect();

    // Remove all previously stored pane information
    _panedPosition.saveToPath(RKEY_PANE_STATE);

    // Remove the current property editor to prevent destructors
    // from firing too late in the shutdown process
    _currentPropertyEditor.reset();
}

void EntityInspector::onUndoRedoOperation()
{
    refresh();
}

void EntityInspector::onDefsReloaded()
{
    refresh();
}

void EntityInspector::refresh()
{
    // Clear the previous entity (detaches this class as observer)
    changeSelectedEntity(scene::INodePtr(), scene::INodePtr());

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
    static StringSet _dependencies
    {
        MODULE_XMLREGISTRY,
        MODULE_GROUPDIALOG,
        MODULE_SELECTIONSYSTEM,
        MODULE_UNDOSYSTEM,
        MODULE_GAMEMANAGER,
        MODULE_COMMANDSYSTEM,
        MODULE_MAINFRAME
    };

    return _dependencies;
}

void EntityInspector::initialiseModule(const IApplicationContext& ctx)
{
    construct();

    GlobalMainFrame().signal_MainFrameConstructed().connect(
        sigc::mem_fun(this, &EntityInspector::onMainFrameConstructed)
    );
    GlobalMainFrame().signal_MainFrameShuttingDown().connect(
        sigc::mem_fun(this, &EntityInspector::onMainFrameShuttingDown)
    );

    GlobalCommandSystem().addCommand("ToggleEntityInspector", toggle);
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

    _keyValueTreeView = wxutil::TreeView::CreateWithModel(treeViewPanel, _kvStore.get(), wxDV_MULTIPLE);
    _keyValueTreeView->EnableAutoColumnWidthFix(true);

    // Search in both name and value columns
    _keyValueTreeView->AddSearchColumn(_columns.name);
    _keyValueTreeView->AddSearchColumn(_columns.value);

    // Add the checkbox for boolean properties
    _booleanColumn = _keyValueTreeView->AppendToggleColumn("", _columns.booleanValue.getColumnIndex(),
        wxDATAVIEW_CELL_ACTIVATABLE, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);

    // Create the Property column (has an icon)
    _keyValueTreeView->AppendIconTextColumn(_("Property"),
        _columns.name.getColumnIndex(), wxDATAVIEW_CELL_INERT,
        wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);

    // Create the value column
    _valueColumn = _keyValueTreeView->AppendTextColumn(_("Value"),
        _columns.value.getColumnIndex(), wxDATAVIEW_CELL_INERT,
        wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);

    _oldValueColumn = _keyValueTreeView->AppendTextColumn(_("Old Value"),
        _columns.oldValue.getColumnIndex(), wxDATAVIEW_CELL_INERT,
        wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);
    _newValueColumn = _keyValueTreeView->AppendTextColumn(_("New Value"),
        _columns.newValue.getColumnIndex(), wxDATAVIEW_CELL_INERT,
        wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);

    // Used to update the help text
    _keyValueTreeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &EntityInspector::_onTreeViewSelectionChanged, this);
    _keyValueTreeView->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &EntityInspector::_onContextMenu, this);

    // When the toggle column is clicked to check/uncheck the box, the model's column value
    // is directly changed by the wxWidgets event handlers. On model value change, this event is fired afterwards
    _keyValueTreeView->Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &EntityInspector::_onDataViewItemChanged, this);

    wxBoxSizer* buttonHbox = new wxBoxSizer(wxHORIZONTAL);

    // Pack in the key and value edit boxes
    _keyEntry = new wxTextCtrl(treeViewPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    _valEntry = new wxTextCtrl(treeViewPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);

    wxBitmap icon = wxArtProvider::GetBitmap(wxART_TICK_MARK, wxART_MENU);
    _setButton = new wxBitmapButton(treeViewPanel, wxID_APPLY, icon);

    buttonHbox->Add(_valEntry, 1, wxEXPAND);
    buttonHbox->Add(_setButton, 0, wxEXPAND);

    treeViewPanel->GetSizer()->Add(_keyValueTreeView, 1, wxEXPAND);
    treeViewPanel->GetSizer()->Add(_keyEntry, 0, wxEXPAND);
    treeViewPanel->GetSizer()->Add(buttonHbox, 0, wxEXPAND);

    _setButton->Bind(wxEVT_BUTTON, &EntityInspector::_onSetProperty, this);
    _keyEntry->Bind(wxEVT_TEXT_ENTER, &EntityInspector::_onEntryActivate, this);
    _valEntry->Bind(wxEVT_TEXT_ENTER, &EntityInspector::_onEntryActivate, this);

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

bool EntityInspector::canUpdateEntity()
{
    return GlobalMapModule().getEditMode() != IMap::EditMode::Merge;
}

// Redraw the GUI elements
void EntityInspector::updateGUIElements()
{
    // Update from selection system
    getEntityFromSelectionSystem();

    auto entityCanBeUpdated = canUpdateEntity();

    auto isMergeMode = GlobalMapModule().getEditMode() == IMap::EditMode::Merge;
    _oldValueColumn->SetHidden(!isMergeMode);
    _newValueColumn->SetHidden(!isMergeMode);

    // Set the value column back to the default AUTO setting
    _valueColumn->SetWidth(wxCOL_WIDTH_AUTOSIZE);

    if (!_selectedEntity.expired())
    {
        _editorFrame->Enable(entityCanBeUpdated);
        _keyValueTreeView->Enable(true);
        _showInheritedCheckbox->Enable(true);
        _showHelpColumnCheckbox->Enable(true);
        _keyEntry->Enable(entityCanBeUpdated);
        _valEntry->Enable(entityCanBeUpdated);
        _setButton->Enable(entityCanBeUpdated);
        _booleanColumn->GetRenderer()->SetMode(entityCanBeUpdated ? wxDATAVIEW_CELL_ACTIVATABLE : wxDATAVIEW_CELL_INERT);

        if (!entityCanBeUpdated)
        {
            _currentPropertyEditor.reset();
        }
		// Update the target entity on any active property editor (#5092)
        else if (_currentPropertyEditor)
		{
			auto newEntity = Node_getEntity(_selectedEntity.lock());
			assert(newEntity != nullptr);

			_currentPropertyEditor->setEntity(newEntity);
		}
    }
    else  // no selected entity
    {
        // Remove the displayed PropertyEditor
		_currentPropertyEditor.reset();

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

    // Pass the call to the specialised routine
    applyKeyValueToSelection(key, val);
}

void EntityInspector::applyKeyValueToSelection(const std::string& key, const std::string& val)
{
    GlobalCommandSystem().executeCommand("SetEntityKeyValue", key, val);
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
    return !_selectedEntity.expired() && canUpdateEntity();
}

void EntityInspector::_onDeleteKey()
{
    wxDataViewItemArray selectedItems;
    _keyValueTreeView->GetSelections(selectedItems);

    if (selectedItems.Count() == 0) return;

    assert(!_selectedEntity.expired());
    Entity* selectedEntity = Node_getEntity(_selectedEntity.lock());

    std::unique_ptr<UndoableCommand> cmd;

    for (const wxDataViewItem& item : selectedItems)
    {
        wxutil::TreeModel::Row row(item, *_kvStore);

        if (!isItemDeletable(row))
        {
            continue;
        }

        if (!cmd)
        {
            cmd.reset(new UndoableCommand("deleteProperty"));
        }

        auto iconAndName = static_cast<wxDataViewIconText>(row[_columns.name]);
        auto key = iconAndName.GetText().ToStdString();

        // Clear the key after copying
        selectedEntity->setKeyValue(key, "");
    }
}

bool EntityInspector::_testDeleteKey()
{
    return _testNonEmptyAndDeletableSelection();
}

void EntityInspector::_onCopyKey()
{
    wxDataViewItemArray selectedItems;
    _keyValueTreeView->GetSelections(selectedItems);

    if (selectedItems.Count() == 0) return;

    _clipboard.clear();

    for (const wxDataViewItem& item : selectedItems)
    {
        wxutil::TreeModel::Row row(item, *_kvStore);

        wxDataViewIconText iconAndName = static_cast<wxDataViewIconText>(row[_columns.name]);

        std::string key = iconAndName.GetText().ToStdString();
        std::string value = row[_columns.value];

        _clipboard.emplace_back(key, value);
    }
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

    _clipboard.clear();
    std::unique_ptr<UndoableCommand> cmd;

    for (const wxDataViewItem& item : selectedItems)
    {
        wxutil::TreeModel::Row row(item, *_kvStore);

        if (!isItemDeletable(row))
        {
            continue;
        }

        if (!cmd)
        {
            cmd.reset(new UndoableCommand("cutProperty"));
        }

        auto iconAndName = static_cast<wxDataViewIconText>(row[_columns.name]);
        auto key = iconAndName.GetText().ToStdString();
        auto value = row[_columns.value];

        _clipboard.emplace_back(key, value);

        // Clear the key after copying
        selectedEntity->setKeyValue(key, "");
    }
}

bool EntityInspector::isItemDeletable(const wxutil::TreeModel::Row& row)
{
    auto iconAndName = static_cast<wxDataViewIconText>(row[_columns.name]);
    auto key = iconAndName.GetText().ToStdString();

    // We don't delete any inherited key values
    if (row[_columns.isInherited].getBool()) return false;

    // Don't delete any classnames either
    if (key == "classname") return false;

    return true;
}

bool EntityInspector::_testNonEmptyAndDeletableSelection()
{
    if (_selectedEntity.expired() || !canUpdateEntity()) return false;

    wxDataViewItemArray selectedItems;
    _keyValueTreeView->GetSelections(selectedItems);

    for (const wxDataViewItem& item : selectedItems)
    {
        wxutil::TreeModel::Row row(item, *_kvStore);

        if (isItemDeletable(row))
        {
            return true; // we have at least one non-inherited value that is not "classname"
        }
    }

    // Either all keys are inherited or classname, or nothing selected at all
    return false;
}

bool EntityInspector::_testCutKey()
{
    return _testNonEmptyAndDeletableSelection();
}

void EntityInspector::_onPasteKey()
{
    // greebo: Instantiate a scoped object to make this operation undoable
    UndoableCommand command("entitySetProperties");

    for (const KeyValuePair& kv : _clipboard)
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
    return !_clipboard.empty() && canUpdateEntity();
}

void EntityInspector::_onAcceptMergeAction()
{
    wxDataViewItemArray selectedItems;
    _keyValueTreeView->GetSelections(selectedItems);

    for (const wxDataViewItem& item : selectedItems)
    {
        wxutil::TreeModel::Row row(item, *_kvStore);

        auto key = row[_columns.name].getString().ToStdString();

        auto conflict = _conflictActions.find(key);

        if (conflict != _conflictActions.end())
        {
            conflict->second->setResolution(scene::merge::ResolutionType::ApplySourceChange);
        }
    }

    // We perform a full refresh of the view
    changeSelectedEntity(scene::INodePtr(), scene::INodePtr());
    getEntityFromSelectionSystem();
}

void EntityInspector::_onRejectMergeAction()
{
    wxDataViewItemArray selectedItems;
    _keyValueTreeView->GetSelections(selectedItems);

    for (const wxDataViewItem& item : selectedItems)
    {
        wxutil::TreeModel::Row row(item, *_kvStore);

        auto key = row[_columns.name].getString().ToStdString();
        
        auto conflict = _conflictActions.find(key);

        if (conflict != _conflictActions.end())
        {
            conflict->second->setResolution(scene::merge::ResolutionType::RejectSourceChange);
            // Deactivate the conflict action itself too, such that the node can be removed once the last action is gone
            conflict->second->deactivate();
        }

        auto action = _mergeActions.find(key);
        
        if (action != _mergeActions.end())
        {
            action->second->deactivate();
        }
    }

    // Check if the merge node is now completely empty, then remove it from the scene
    // A single entity must be selected
    if (GlobalSelectionSystem().countSelected() == 1)
    {
        auto selectedNode = GlobalSelectionSystem().ultimateSelected();

        if (selectedNode && selectedNode->getNodeType() == scene::INode::Type::MergeAction)
        {
            auto mergeNode = std::dynamic_pointer_cast<scene::IMergeActionNode>(selectedNode);
            assert(mergeNode);

            if (!mergeNode->hasActiveActions())
            {
                // Remove this node from the scene, it's empty now
                scene::removeNodeFromParent(mergeNode);
            }
        }
    }

    // We perform a full refresh of the view
    changeSelectedEntity(scene::INodePtr(), scene::INodePtr());
    getEntityFromSelectionSystem();
}

bool EntityInspector::_testAcceptMergeAction()
{
    if (GlobalMapModule().getEditMode() != IMap::EditMode::Merge)
    {
        return false;
    }

    wxDataViewItemArray selectedItems;
    _keyValueTreeView->GetSelections(selectedItems);

    for (const wxDataViewItem& item : selectedItems)
    {
        wxutil::TreeModel::Row row(item, *_kvStore);

        if (isItemAffecedByMergeConflict(row))
        {
            return true;
        }
    }

    return false;
}

bool EntityInspector::_testRejectMergeAction()
{
    if (GlobalMapModule().getEditMode() != IMap::EditMode::Merge)
    {
        return false;
    }

    wxDataViewItemArray selectedItems;
    _keyValueTreeView->GetSelections(selectedItems);

    for (const wxDataViewItem& item : selectedItems)
    {
        wxutil::TreeModel::Row row(item, *_kvStore);

        if (isItemAffecedByMergeOperation(row))
        {
            return true;
        }
    }

    return false;
}

bool EntityInspector::isItemAffecedByMergeConflict(const wxutil::TreeModel::Row& row)
{
    auto key = row[_columns.name].getString().ToStdString();
    return _mergeActions.count(key) > 0 && _conflictActions.count(key) > 0;
}

bool EntityInspector::isItemAffecedByMergeOperation(const wxutil::TreeModel::Row& row)
{
    auto key = row[_columns.name].getString().ToStdString();
    return _mergeActions.count(key) > 0 || _conflictActions.count(key) > 0;
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

void EntityInspector::handleShowInheritedChanged()
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

void EntityInspector::handleShowHelpTextChanged()
{
    bool helpIsVisible = _showHelpColumnCheckbox->IsChecked();

    // Set the visibility of the help text panel
    _helpText->Show(helpIsVisible);

    if (helpIsVisible)
    {
        // Trigger an update of the help contents (#5148)
        wxDataViewItemArray selectedItems;
        _keyValueTreeView->GetSelections(selectedItems);

        if (selectedItems.Count() == 1)
        {
            wxutil::TreeModel::Row row(selectedItems.front(), *_kvStore);
            updateHelpText(row);
        }
    }

    // After showing a packed control we need to call the sizer's layout() method
    _mainWidget->GetSizer()->Layout();
}

void EntityInspector::updateHelpText(const wxutil::TreeModel::Row& row)
{
    _helpText->SetValue("");

    if (!row.getItem().IsOk()) return;

    // Get the key pointed at
    bool hasHelp = row[_columns.hasHelpText].getBool();

    if (!hasHelp) return;
    
    std::string key = getSelectedKey();

    assert(!_selectedEntity.expired());
    auto* selectedEntity = Node_getEntity(_selectedEntity.lock());

    auto eclass = selectedEntity->getEntityClass();
    assert(eclass);

    if (key == "classname")
    {
        // #5621: Show the editor_usage string when the classname is selected
        _helpText->SetValue(eclass::getUsage(*eclass));
    }
    else
    {
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

        // Update key and value entry boxes, but only if there is a key value. If
        // there is no selection we do not clear the boxes, to allow keyval copying
        // between entities.
        if (!key.empty())
        {
            _keyEntry->SetValue(key);
            _valEntry->SetValue(value);
        }

        // Update property editor, unless we're in merge mode
        if (canUpdateEntity())
        {
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
        grey.SetItalic(true);

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
        row[_columns.oldValue] = std::string();
        row[_columns.newValue] = std::string();

        row[_columns.isInherited] = true;
        row[_columns.hasHelpText] = hasDescription;

        row.SendItemAdded();
    }
}

// Append inherited (entityclass) properties
void EntityInspector::addClassProperties()
{
    auto selectedNode = _selectedEntity.lock();

    if (!selectedNode)
    {
        return;
    }

    Entity* selectedEntity = Node_getEntity(selectedNode);

    // Get the entityclass for the current entity
    std::string className = selectedEntity->getKeyValue("classname");
    IEntityClassPtr eclass = GlobalEntityClassManager().findOrInsert(
        className, true
    );

    if (!eclass)
    {
        rWarning() << "No classname on entity '" << selectedEntity->getKeyValue("name") << "'" << std::endl;
        return;
    }

    // Visit the entity class
    eclass->forEachAttribute(
        [&](const EntityClassAttribute& a, bool) { addClassAttribute(a); }
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
        changeSelectedEntity(scene::INodePtr(), scene::INodePtr());
        _primitiveNumLabel->SetLabelText("");
        return;
    }

    auto selectedNode = GlobalSelectionSystem().ultimateSelected();

    // The root node must not be selected (this can happen if Invert Selection is
    // activated with an empty scene, or by direct selection in the entity list).
    if (selectedNode->isRoot())
    {
        changeSelectedEntity(scene::INodePtr(), selectedNode);
        _primitiveNumLabel->SetLabelText("");
        return;
    }

    // Try both the selected node (if an entity is selected) or the parent node
    // (if a brush is selected).
    Entity* newSelectedEntity = Node_getEntity(selectedNode);

    if (newSelectedEntity)
    {
        // Node was an entity, use this
        changeSelectedEntity(selectedNode, selectedNode);

        // Just set the entity number
        auto indices = scene::getNodeIndices(selectedNode);

        _primitiveNumLabel->SetLabelText(fmt::format(_("Entity {0}"), indices.first));
    }
    else
    {
        // Check if this is a special merge node
        if (selectedNode->getNodeType() == scene::INode::Type::MergeAction &&
            GlobalMapModule().getEditMode() == IMap::EditMode::Merge)
        {
            auto mergeAction = std::dynamic_pointer_cast<scene::IMergeActionNode>(selectedNode);
            assert(mergeAction);

            if (mergeAction && Node_isEntity(mergeAction->getAffectedNode()))
            {
                // Use the entity of the merge node
                changeSelectedEntity(mergeAction->getAffectedNode(), selectedNode);
                _primitiveNumLabel->SetLabelText(_("Merge Preview"));
                return;
            }
        }

        // Node was not an entity, try parent instead
        scene::INodePtr selectedNodeParent = selectedNode->getParent();
        changeSelectedEntity(selectedNodeParent, selectedNode);

        if (!_selectedEntity.lock())
        {
            return;
        }

        try
        {
            auto indices = scene::getNodeIndices(selectedNode);

            _primitiveNumLabel->SetLabelText(fmt::format(_("Entity {0}, Primitive {1}"), indices.first, indices.second));
        }
        catch (const std::out_of_range& ex)
        {
            rWarning() << ex.what() << std::endl;
            _primitiveNumLabel->SetLabelText("-");
        }
    }
}

void EntityInspector::changeSelectedEntity(const scene::INodePtr& newEntity, const scene::INodePtr& selectedNode)
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
    _mergeActions.clear();
    _conflictActions.clear();

    // Reset the sorting when changing entities
    _keyValueTreeView->ResetSortingOnAllColumns();

    // Attach to new entity if it is non-NULL
    if (newEntity && newEntity->getNodeType() == scene::INode::Type::Entity)
    {
        _selectedEntity = newEntity;

        // Any possible merge actions go in first
        handleMergeActions(selectedNode);

        // Attach as observer to fill the listview
        Node_getEntity(newEntity)->attachObserver(this);

        // Add inherited properties if the checkbox is set
        if (_showInheritedCheckbox->IsChecked())
        {
            addClassProperties();
        }
    }
}

void EntityInspector::handleKeyValueMergeAction(const scene::merge::IEntityKeyValueMergeAction::Ptr& mergeAction)
{
    // Remember this action in the map, it will be used in onKeyChange()
    _mergeActions[mergeAction->getKey()] = mergeAction;

    auto selectedEntity = _selectedEntity.lock();

    // Keys added by a merge operation won't be handled in onKeyChange(), so do this here
    if (mergeAction->getType() == scene::merge::ActionType::AddKeyValue ||
        (mergeAction->getType() == scene::merge::ActionType::ChangeKeyValue && selectedEntity && 
         Node_getEntity(selectedEntity)->getKeyValue(mergeAction->getKey()).empty()))
    {
        onKeyChange(mergeAction->getKey(), mergeAction->getValue());
    }
}

void EntityInspector::handleMergeActions(const scene::INodePtr& selectedNode)
{
    // Any possible merge actions go in first
    if (GlobalMapModule().getEditMode() != IMap::EditMode::Merge || !selectedNode ||
        selectedNode->getNodeType() != scene::INode::Type::MergeAction)
    {
        return;
    }

    auto mergeNode = std::dynamic_pointer_cast<scene::IMergeActionNode>(selectedNode);

    if (!mergeNode) return;
    
    mergeNode->foreachMergeAction([&](const scene::merge::IMergeAction::Ptr& action)
    {
        if (!action->isActive()) return; // don't apply inactive actions to our view

        auto entityKeyValueAction = std::dynamic_pointer_cast<scene::merge::IEntityKeyValueMergeAction>(action);

        if (entityKeyValueAction)
        {
            handleKeyValueMergeAction(entityKeyValueAction);
        }

        auto conflictAction = std::dynamic_pointer_cast<scene::merge::IConflictResolutionAction>(action);

        if (conflictAction)
        {
            bool conflictIsUnresolved = conflictAction->getResolution() == scene::merge::ResolutionType::Unresolved;

            if (conflictIsUnresolved || conflictAction->getResolution() == scene::merge::ResolutionType::ApplySourceChange)
            {
                auto sourceAction = std::dynamic_pointer_cast<scene::merge::IEntityKeyValueMergeAction>(conflictAction->getSourceAction());

                if (sourceAction)
                {
                    handleKeyValueMergeAction(sourceAction);

                    // Remember the conflict action if it's not yet resolved
                    if (conflictIsUnresolved)
                    {
                        _conflictActions[sourceAction->getKey()] = conflictAction;
                    }
                }
            }
        }
    });
}

void EntityInspector::toggle(const cmd::ArgumentList& args)
{
    GlobalGroupDialog().togglePage("entity");
}

// Define the static EntityInspector module
module::StaticModule<EntityInspector> entityInspectorModule;

} // namespace ui
