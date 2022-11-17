#include "EntityInspector.h"
#include "EntityInspectorModule.h"
#include "PropertyEditorFactory.h"
#include "AddPropertyDialog.h"

#include "i18n.h"
#include "ientity.h"
#include "ideclmanager.h"
#include "ieclass.h"
#include "iregistry.h"
#include "igame.h"
#include "imap.h"
#include "iundo.h"
#include "itextstream.h"
#include "ui/iuserinterface.h"

#include "selectionlib.h"
#include "scene/SelectionIndex.h"
#include "scenelib.h"
#include "eclass.h"
#include "gamelib.h"
#include "wxutil/menu/IconTextMenuItem.h"
#include "wxutil/dataview/TreeModel.h"
#include "wxutil/dataview/TreeViewItemStyle.h"
#include "selection/EntitySelection.h"
#include "TargetKey.h"

#include <optional>
#include <map>
#include <regex>
#include <string>

#include <wx/panel.h>
#include <wx/sizer.h>
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

namespace ui
{

namespace
{
    constexpr const char* const PROPERTY_NODES_XPATH = "/entityInspector//property";

    const std::string RKEY_ROOT = "user/ui/entityInspector/";
    const std::string RKEY_PANE_STATE = RKEY_ROOT + "pane";
    const std::string RKEY_SHOW_HELP_AREA = RKEY_ROOT + "showHelpArea";
    const std::string RKEY_SHOW_INHERITED_PROPERTIES = RKEY_ROOT + "showInheritedProperties";
}

EntityInspector::EntityInspector(wxWindow* parent) :
    DockablePanel(parent),
    _editorFrame(nullptr),
    _showInheritedCheckbox(nullptr),
    _showHelpColumnCheckbox(nullptr),
    _primitiveNumLabel(nullptr),
    _keyValueTreeView(nullptr),
    _keyEntry(nullptr),
    _valEntry(nullptr),
    _setButton(nullptr),
    _selectionNeedsUpdate(true),
    _inheritedPropertiesNeedUpdate(true),
    _helpTextNeedsUpdate(true)
{
    construct();
}

void EntityInspector::construct()
{
    _emptyIcon = wxutil::Icon(wxutil::GetLocalBitmap("empty.png"));

    SetName("EntityInspector");
    SetSizer(new wxBoxSizer(wxVERTICAL));

    // Construct HBox with the display checkboxes
    wxBoxSizer* optionsHBox = new wxBoxSizer(wxHORIZONTAL);

    _showInheritedCheckbox = new wxCheckBox(this, wxID_ANY, _("Show inherited properties"));
    _showInheritedCheckbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& ev) {
        handleShowInheritedChanged();
    });

    _showHelpColumnCheckbox = new wxCheckBox(this, wxID_ANY, _("Show help"));
    _showHelpColumnCheckbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& ev) {
        updateHelpTextPanel();
    });

    _primitiveNumLabel = new wxStaticText(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize);
    _primitiveNumLabel->SetFont(_primitiveNumLabel->GetFont().Bold());

    optionsHBox->Add(_primitiveNumLabel, 1, wxEXPAND | wxALL, 5);
    optionsHBox->AddStretchSpacer();
    optionsHBox->Add(_showInheritedCheckbox, 1, wxEXPAND);
    optionsHBox->Add(_showHelpColumnCheckbox, 0, wxEXPAND);

    // Pane with treeview and editor panel
    _paned = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);
    _paned->SetMinimumPaneSize(80);

    _paned->SplitHorizontally(createTreeViewPane(_paned), createPropertyEditorPane(_paned));
    _panedPosition.connect(_paned);

    _helpText = new wxTextCtrl(this, wxID_ANY, "",
        wxDefaultPosition, wxDefaultSize, wxTE_LEFT | wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
    _helpText->SetMinClientSize(wxSize(-1, 60));

    GetSizer()->Add(optionsHBox, 0, wxEXPAND | wxALL, 3);
    GetSizer()->Add(_paned, 1, wxEXPAND);
    GetSizer()->Add(_helpText, 0, wxEXPAND);

    _helpText->Hide();

    _spawnargs.reset(new selection::CollectiveSpawnargs);

    // Connect the signals
    _keyValueAddedHandler = _spawnargs->signal_KeyAdded().connect(
        sigc::mem_fun(this, &EntityInspector::onKeyAdded)
    );
    _keyValueRemovedHandler = _spawnargs->signal_KeyRemoved().connect(
        sigc::mem_fun(this, &EntityInspector::onKeyRemoved)
    );
    _keyValueSetChangedHandler = _spawnargs->signal_KeyValueSetChanged().connect(
        sigc::mem_fun(this, &EntityInspector::onKeyValueSetChanged)
    );

    _entitySelection = std::make_unique<selection::EntitySelection>(*_spawnargs);

    // Connect to registry key and handle the loaded value
    registry::bindWidget(_showHelpColumnCheckbox, RKEY_SHOW_HELP_AREA);
    registry::bindWidget(_showInheritedCheckbox, RKEY_SHOW_INHERITED_PROPERTIES);

    handleShowInheritedChanged();

    // Reload the information from the registry
    restoreSettings();

    // Create the context menu
    createContextMenu();

    // initialise the properties
    loadPropertyMap();

    // greebo: Now that the dialog is shown, tell the Entity Inspector to reload
    // the position info from the Registry once again.
    restoreSettings();

    refresh();
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
    if (mode == IMap::EditMode::Normal)
    {
        _mergeActions.clear();
        _conflictActions.clear();
        _keyValueIterMap.clear();
        _kvStore->Clear();
    }

    refresh();
}

void EntityInspector::onKeyChange(const std::string& key, const std::string& value, bool isMultiValue)
{
    wxDataViewItem item;
    bool added = false;

    // Check if we already have an iter for this key (i.e. this is a
    // modification).
    auto i = _keyValueIterMap.find(key);

    if (i != _keyValueIterMap.end())
    {
        item = i->second;
    }
    else
    {
        // Append a new row to the list store and add it to the iter map
        item = _kvStore->AddItem().getItem();
        _keyValueIterMap.emplace(key, item);

        added = true;
    }

    // Set the values for the row
    wxutil::TreeModel::Row row(item, *_kvStore);

    wxDataViewItemAttr style;

    // Get a base style for any merge actions on this key
    applyMergeActionStyle(key, style);

    // Insert the key and the value, the icon will be updated later
    row[_modelCols.name] = wxVariant(wxDataViewIconText(key, _emptyIcon));
    row[_modelCols.value] = value;
    row[_modelCols.isMultiValue] = isMultiValue;

    setOldAndNewValueColumns(row, key, style);

    // Apply background style to all other columns
    row[_modelCols.name].setAttr(style);
    row[_modelCols.booleanValue].setAttr(style);

    // Before applying the style to the value, check if the value is ambiguous
    if (isMultiValue)
    {
        wxutil::TreeViewItemStyle::ApplyKeyValueAmbiguousStyle(style);
    }

    row[_modelCols.value].setAttr(style);
    row[_modelCols.isInherited] = false;

    updateKeyType(row);

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
    if (!isMultiValue && curKey == key && selectedKey == key)
    {
        _valEntry->SetValue(value);
    }

    // Also update the property editor if the changed key is highlighted
    if (_currentPropertyEditor && key == selectedKey)
    {
        _currentPropertyEditor->updateFromEntities();
    }
}

void EntityInspector::updateKeyType(wxutil::TreeModel::Row& row)
{
    auto namePlusIcon = static_cast<wxDataViewIconText>(row[_modelCols.name]);
    auto key = namePlusIcon.GetText().ToStdString();
    auto value = row[_modelCols.value].getString();

    // Look up type for this key. First check the property parm map,
    // then the entity class itself. If nothing is found, leave blank.
    // Get the type for this key if it exists, and the options
    auto keyType = getPropertyTypeForKey(key);

    wxutil::Icon icon(keyType.empty() ? _emptyIcon :
        wxutil::Icon(EntityInspectorModule::Instance().getPropertyEditorFactory().getBitmapFor(keyType)));

    // Assign the icon to the column
    row[_modelCols.name] = wxVariant(wxDataViewIconText(key, icon));

    if (keyType == "bool")
    {
        // Render a checkbox for boolean values (store an actual bool)
        row[_modelCols.booleanValue] = value == "1";

        // Column is enabled by default after assignment
    }
    else
    {
        // Store false to render the checkbox as unchecked
        row[_modelCols.booleanValue] = false;
        row[_modelCols.booleanValue].setEnabled(false);
    }
}

void EntityInspector::setOldAndNewValueColumns(wxutil::TreeModel::Row& row, const std::string& key, const wxDataViewItemAttr& style)
{
    auto action = _mergeActions.find(key);

    if (action != _mergeActions.end())
    {
        if (action->second->getType() == scene::merge::ActionType::AddKeyValue)
        {
            row[_modelCols.oldValue] = std::string(); // no old value to show
            row[_modelCols.oldValue].setAttr(style);
        }
        else
        {
            wxDataViewItemAttr oldAttr = style;
            wxutil::TreeViewItemStyle::SetStrikethrough(oldAttr, true);
            row[_modelCols.oldValue] = action->second->getUnchangedValue();
            row[_modelCols.oldValue].setAttr(oldAttr);
        }

        row[_modelCols.newValue] = action->second->getValue();

        wxDataViewItemAttr newAttr = style;
        newAttr.SetBold(true);

        row[_modelCols.newValue].setAttr(newAttr);
    }
    else
    {
        row[_modelCols.oldValue] = std::string();
        row[_modelCols.newValue] = std::string();
    }
}

void EntityInspector::applyMergeActionStyle(const std::string& key, wxDataViewItemAttr& style)
{
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
        default:
            break;
        }
    }

    // Conflicting actions get a special render style
    auto conflict = _conflictActions.find(key);

    if (conflict != _conflictActions.end() && conflict->second->getResolution() == scene::merge::ResolutionType::Unresolved)
    {
        wxutil::TreeViewItemStyle::ApplyKeyValueConflictStyle(style);
    }
}

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

EntityInspector::~EntityInspector()
{
    if (panelIsActive())
    {
        disconnectListeners();
    }

    _keyValueAddedHandler.disconnect();
    _keyValueRemovedHandler.disconnect();
    _keyValueSetChangedHandler.disconnect();
    _entitySelection.reset();
    _spawnargs.reset();

    _mergeActions.clear();
    _conflictActions.clear();

    _panedPosition.saveToPath(RKEY_PANE_STATE);

    // Remove the current property editor to prevent destructors
    // from firing too late in the shutdown process
    removePropertyEditor();
}

void EntityInspector::onPanelActivated()
{
    connectListeners();
    refresh();

    _panedPosition.connect(_paned);
    _panedPosition.loadFromPath(RKEY_PANE_STATE);
}

void EntityInspector::onPanelDeactivated()
{
    // Save current position and disconnect the tracker to not receive
    // faulty sizes during reconstruction of the parent window
    _panedPosition.saveToPath(RKEY_PANE_STATE);
    _panedPosition.disconnect();

    disconnectListeners();
}

void EntityInspector::connectListeners()
{
    // Register self to the SelectionSystem to get notified upon selection changes.
    GlobalSelectionSystem().addObserver(this);

    _defsReloadedHandler = GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::EntityDef).connect(
        sigc::mem_fun(this, &EntityInspector::onDefsReloaded)
    );

    _mapEditModeChangedHandler = GlobalMapModule().signal_editModeChanged().connect(
        sigc::mem_fun(this, &EntityInspector::onMapEditModeChanged)
    );
}

void EntityInspector::disconnectListeners()
{
    GlobalSelectionSystem().removeObserver(this);

    _mapEditModeChangedHandler.disconnect();
    _defsReloadedHandler.disconnect();
}

void EntityInspector::onKeyUpdatedCommon(const std::string& key)
{
    if (key == "classname")
    {
        _inheritedPropertiesNeedUpdate = true;
    }

    _helpTextNeedsUpdate = true;
}

void EntityInspector::onKeyAdded(const std::string& key, const std::string& value)
{
    onKeyUpdatedCommon(key);
    onKeyChange(key, value);
}

void EntityInspector::onKeyRemoved(const std::string& key)
{
    onKeyUpdatedCommon(key);

    // Look up iter in the TreeIter map, and delete it from the list store
    auto i = _keyValueIterMap.find(key);

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

void EntityInspector::onKeyValueSetChanged(const std::string& key, const std::string& uniqueValue)
{
    onKeyUpdatedCommon(key);
    onKeyChange(key, uniqueValue.empty() ? _("[differing values]") : uniqueValue, uniqueValue.empty());
}

void EntityInspector::onDefsReloaded()
{
    GlobalUserInterface().dispatch([this]() { refresh(); });
}

void EntityInspector::refresh()
{
    _selectionNeedsUpdate = true;
    _helpTextNeedsUpdate = true;
    _inheritedPropertiesNeedUpdate = true;
    requestIdleCallback();
}

wxWindow* EntityInspector::createPropertyEditorPane(wxWindow* parent)
{
    _editorFrame = new wxPanel(parent, wxID_ANY);
    _editorFrame->SetSizer(new wxBoxSizer(wxVERTICAL));
    _editorFrame->SetMinClientSize(wxSize(-1, 50));
    return _editorFrame;
}

wxWindow* EntityInspector::createTreeViewPane(wxWindow* parent)
{
    wxPanel* treeViewPanel = new wxPanel(parent, wxID_ANY);
    treeViewPanel->SetSizer(new wxBoxSizer(wxVERTICAL));
    treeViewPanel->SetMinClientSize(wxSize(-1, 150));

    _kvStore = new wxutil::TreeModel(_modelCols, true); // this is a list model

    _keyValueTreeView = wxutil::TreeView::CreateWithModel(treeViewPanel, _kvStore.get(), wxDV_MULTIPLE);
    _keyValueTreeView->EnableAutoColumnWidthFix(true);

    // Search in both name and value columns
    _keyValueTreeView->AddSearchColumn(_modelCols.name);
    _keyValueTreeView->AddSearchColumn(_modelCols.value);

    // Add the checkbox for boolean properties
    _viewCols.boolean = _keyValueTreeView->AppendToggleColumn(
        "", _modelCols.booleanValue.getColumnIndex(), wxDATAVIEW_CELL_ACTIVATABLE,
        wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT
    );

    // Create the Property column (has an icon)
    _keyValueTreeView->AppendIconTextColumn(_("Property"),
        _modelCols.name.getColumnIndex(), wxDATAVIEW_CELL_INERT,
        wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);

    // Create the value column
    _viewCols.value = _keyValueTreeView->AppendTextColumn(_("Value"),
        _modelCols.value.getColumnIndex(), wxDATAVIEW_CELL_INERT,
        wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);

    _viewCols.oldValue = _keyValueTreeView->AppendTextColumn(_("Old Value"),
        _modelCols.oldValue.getColumnIndex(), wxDATAVIEW_CELL_INERT,
        wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);
    _viewCols.newValue = _keyValueTreeView->AppendTextColumn(_("New Value"),
        _modelCols.newValue.getColumnIndex(), wxDATAVIEW_CELL_INERT,
        wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);

    // Used to update the help text
    _keyValueTreeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED,
                            &EntityInspector::_onTreeViewSelectionChanged, this);
    _keyValueTreeView->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &EntityInspector::_onContextMenu,
                            this);

    // When the toggle column is clicked to check/uncheck the box, the model's column value
    // is directly changed by the wxWidgets event handlers. On model value change, this event is fired afterwards
    _keyValueTreeView->Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED,
                            &EntityInspector::_onDataViewItemChanged, this);

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

    auto iconAndName = static_cast<wxDataViewIconText>(row[_modelCols.name]);
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
    auto entityCanBeUpdated = canUpdateEntity();

    auto isMergeMode = GlobalMapModule().getEditMode() == IMap::EditMode::Merge;
    _viewCols.oldValue->SetHidden(!isMergeMode);
    _viewCols.newValue->SetHidden(!isMergeMode);

    // Set the value column back to the default AUTO setting
    _viewCols.value->SetWidth(wxCOL_WIDTH_AUTOSIZE);

    if (!_entitySelection->empty())
    {
        _editorFrame->Enable(entityCanBeUpdated);
        _showHelpColumnCheckbox->Enable(true);
        _viewCols.boolean->GetRenderer()->SetMode(entityCanBeUpdated ? wxDATAVIEW_CELL_ACTIVATABLE
                                                                     : wxDATAVIEW_CELL_INERT);

        if (!entityCanBeUpdated)
        {
            removePropertyEditor();
        }
    }
    else  // no selected entity
    {
        // Remove the displayed PropertyEditor
        removePropertyEditor();
        // Disable the dialog and clear the TreeView
        _editorFrame->Enable(false);
        _showInheritedCheckbox->Enable(false);
        _showHelpColumnCheckbox->Enable(false);
    }

    updateEntryBoxSensitivity();
}

void EntityInspector::updateEntryBoxSensitivity()
{
    auto entityCanBeUpdated = canUpdateEntity() && !_entitySelection->empty();
    auto selectedKey = getSelectedKey();

    auto classNameSelected = getSelectedKey() == "classname";

    _keyEntry->Enable(entityCanBeUpdated && !classNameSelected);
    _valEntry->Enable(entityCanBeUpdated && !classNameSelected);
    _setButton->Enable(entityCanBeUpdated && !classNameSelected);
}

void EntityInspector::updatePrimitiveNumber()
{
    auto numSelectedEntities = _entitySelection->size();

    if (GlobalMapModule().getEditMode() == IMap::EditMode::Merge)
    {
        if (numSelectedEntities > 1)
        {
            _primitiveNumLabel->SetLabelText(_("No multi-selection in merge mode"));
            return;
        }
        else if (numSelectedEntities == 1)
        {
            _primitiveNumLabel->SetLabelText(_("Merge Preview"));
            return;
        }
    }

    // Do we have exactly one primitive selected?
    if (numSelectedEntities > 1)
    {
        _primitiveNumLabel->SetLabelText(fmt::format("[{0} Entities]", numSelectedEntities));
        return;
    }

    // Check if the node is a primitive
    if (GlobalSelectionSystem().countSelected() == 1)
    {
        try
        {
            auto selectedNode = GlobalSelectionSystem().ultimateSelected();
            auto indices = scene::getNodeIndices(selectedNode);

            if (Node_isEntity(selectedNode))
            {
                _primitiveNumLabel->SetLabelText(fmt::format(_("Entity {0}"), indices.first));
            }
            else
            {
                _primitiveNumLabel->SetLabelText(fmt::format(_("Entity {0}, Primitive {1}"), indices.first, indices.second));
            }

            return;
        }
        catch (const std::out_of_range& ex)
        {
            rWarning() << ex.what() << std::endl;
        }
    }

    _primitiveNumLabel->SetLabelText("");
}

void EntityInspector::onIdle()
{
    if (_selectionNeedsUpdate)
    {
        _selectionNeedsUpdate = false;

        // Fire the selection update. This will invoke onKeyAdded/onKeyChanged etc.
        // on ourselves for every spawnarg that should be listed or removed
        _entitySelection->update();

        // After selection rescan, trigger an update of the key types
        // of all listed key/value pairs. Not all information is fully available
        // during the above update
        updateListedKeyTypes();

        if (_entitySelection->empty())
        {
            // Reset the sorting when the last entity is released
            _keyValueTreeView->ResetSortingOnAllColumns();
        }

        _mergeActions.clear();
        _conflictActions.clear();

        if (GlobalMapModule().getEditMode() == IMap::EditMode::Merge)
        {
            auto selectionCount = GlobalSelectionSystem().countSelected();

            // Disable the tree view if we're in merge mode and multiple entities are selected
            _keyValueTreeView->Enable(selectionCount == 1);

            // In merge mode, we handle all updates ourselves,
            // no entities are directly selected, only merge nodes are candidates

            // We do a full refresh on every selection change in merge mode
            _kvStore->Clear();
            _keyValueIterMap.clear();
            _keyValueTreeView->ResetSortingOnAllColumns();

            if (GlobalSelectionSystem().countSelected() == 1)
            {
                handleMergeActions(GlobalSelectionSystem().ultimateSelected());
            }
        }
        else
        {
            _keyValueTreeView->Enable(!_entitySelection->empty());
        }

        updatePrimitiveNumber();
    }

    if (_inheritedPropertiesNeedUpdate)
    {
        _inheritedPropertiesNeedUpdate = false;

        // We can display inherited key values if the classname is unique
        bool canShowInheritedKeys = !_spawnargs->getSharedKeyValue("classname").empty();

        _showInheritedCheckbox->Enable(canShowInheritedKeys);

        // We might be switching from one single selected entity to another
        // Avoid the old set of inherited key values from sticking around
        removeClassProperties();

        if (canShowInheritedKeys && _showInheritedCheckbox->IsChecked())
        {
            addClassProperties();
        }
    }

    if (_helpTextNeedsUpdate)
    {
        _helpTextNeedsUpdate = false;
        updateHelpTextPanel();
    }

    updateGUIElements();
}

// Selection changed callback
void EntityInspector::selectionChanged(const scene::INodePtr& node, bool isComponent)
{
    if (isComponent) return; // ignore component changes

    refresh();
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

    auto nodes = game::current::getNodes(PROPERTY_NODES_XPATH);

    for (const auto& node : nodes)
    {
        _propertyTypes.emplace(node.getAttributeValue("match"), node.getAttributeValue("type"));
    }
}

// Popup menu callbacks (see wxutil::PopupMenu)

void EntityInspector::_onAddKey()
{
    assert(_entitySelection->size() == 1);

    auto selectedEntity = Node_getEntity(_entitySelection->getSingleSelectedEntity());

    // Obtain the entity class to provide to the AddPropertyDialog
    auto ec = selectedEntity->getEntityClass();

    // Choose a property, and add to entity with a default value
    auto properties = AddPropertyDialog::chooseProperty(selectedEntity);

    for (std::size_t i = 0; i < properties.size(); ++i)
    {
        const std::string& key = properties[i];

        // Add all keys, skipping existing ones to not overwrite any values on the entity
        if (selectedEntity->getKeyValue(key).empty() || selectedEntity->isInherited(key))
        {
            // Add the keyvalue on the entity (triggering the refresh)
            selectedEntity->setKeyValue(key, "-");
        }
    }
}

bool EntityInspector::_testAddKey()
{
    return _entitySelection->size() == 1 && canUpdateEntity();
}

void EntityInspector::_onDeleteKey()
{
    wxDataViewItemArray selectedItems;
    _keyValueTreeView->GetSelections(selectedItems);

    if (selectedItems.Count() == 0) return;

    assert(!_entitySelection->empty());

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

        auto iconAndName = static_cast<wxDataViewIconText>(row[_modelCols.name]);
        auto key = iconAndName.GetText().ToStdString();

        applyKeyValueToSelection(key, "");
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

        wxDataViewIconText iconAndName = static_cast<wxDataViewIconText>(row[_modelCols.name]);

        std::string key = iconAndName.GetText().ToStdString();
        std::string value = row[_modelCols.value];

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

    assert(!_entitySelection->empty());

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

        auto iconAndName = static_cast<wxDataViewIconText>(row[_modelCols.name]);
        auto key = iconAndName.GetText().ToStdString();
        auto value = row[_modelCols.value];

        _clipboard.emplace_back(key, value);

        // Clear the key after copying
        applyKeyValueToSelection(key, "");
    }
}

bool EntityInspector::isItemDeletable(const wxutil::TreeModel::Row& row)
{
    auto iconAndName = static_cast<wxDataViewIconText>(row[_modelCols.name]);
    auto key = iconAndName.GetText().ToStdString();

    // We don't delete any inherited key values
    if (row[_modelCols.isInherited].getBool()) return false;

    // Don't delete any classnames or names
    if (key == "classname" || key == "name") return false;

    return true;
}

bool EntityInspector::_testNonEmptyAndDeletableSelection()
{
    if (_entitySelection->empty() || !canUpdateEntity()) return false;

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

        auto key = row[_modelCols.name].getString().ToStdString();

        auto conflict = _conflictActions.find(key);

        if (conflict != _conflictActions.end())
        {
            conflict->second->setResolution(scene::merge::ResolutionType::ApplySourceChange);
        }
    }

    // We perform a full refresh of the view
    refresh();
}

void EntityInspector::_onRejectMergeAction()
{
    wxDataViewItemArray selectedItems;
    _keyValueTreeView->GetSelections(selectedItems);

    for (const wxDataViewItem& item : selectedItems)
    {
        wxutil::TreeModel::Row row(item, *_kvStore);

        auto key = row[_modelCols.name].getString().ToStdString();

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
    refresh();
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
    auto key = row[_modelCols.name].getString().ToStdString();
    return _mergeActions.count(key) > 0 && _conflictActions.count(key) > 0;
}

bool EntityInspector::isItemAffecedByMergeOperation(const wxutil::TreeModel::Row& row)
{
    auto key = row[_modelCols.name].getString().ToStdString();
    return _mergeActions.count(key) > 0 || _conflictActions.count(key) > 0;
}

void EntityInspector::_onContextMenu(wxDataViewEvent& ev)
{
    _contextMenu->show(_keyValueTreeView);
}

void EntityInspector::_onDataViewItemChanged(wxDataViewEvent& ev)
{
    if (ev.GetDataViewColumn() != nullptr &&
        static_cast<int>(ev.GetDataViewColumn()->GetModelColumn()) == _modelCols.booleanValue.getColumnIndex())
    {
        // Model value in the boolean column has changed, this means
        // the user has clicked the checkbox, send the value to the entity/entities
        wxutil::TreeModel::Row row(ev.GetItem(), *_kvStore);

        wxDataViewIconText iconAndName = static_cast<wxDataViewIconText>(row[_modelCols.name]);

        std::string key = iconAndName.GetText().ToStdString();
        bool updatedValue = row[_modelCols.booleanValue].getBool();

        UndoableCommand cmd("entitySetProperty");
        applyKeyValueToSelection(key, updatedValue ? "1" : "0");

        // Check if the property was an inherited one.
        // The applyKeyValue function produced a non-inherited entry
        // which should be visible once we're done
        // Note: selecting the non-inherited property instead of this one
        // is not as easy as it may appear, since the user is yet to release
        // the mouse button (we're in the middle of the click event here)
        // and the MouseUp handler will select this row again
        if (row[_modelCols.isInherited].getBool())
        {
            _kvStore->ForeachNode([&](wxutil::TreeModel::Row& row)
            {
                wxDataViewIconText nameVal = static_cast<wxDataViewIconText>(row[_modelCols.name]);

                if (nameVal.GetText() == key && !row[_modelCols.isInherited].getBool())
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
        addClassProperties();
    else
        removeClassProperties();
}

void EntityInspector::updateHelpTextPanel()
{
    bool helpIsVisible = _showHelpColumnCheckbox->IsChecked();

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
        else
        {
            // No single key selected, clear the help contents and we're done
            setHelpText("");
        }
    }

    // Set the visibility of the help text panel
    if (_helpText->IsShown() != helpIsVisible)
    {
        _helpText->Show(helpIsVisible);

        // After showing a packed control we need to call the layout() method
        Layout();
    }
}

void EntityInspector::setHelpText(const std::string& newText)
{
    _helpText->SetValue(newText);
    _helpText->Enable(!newText.empty());
}

void EntityInspector::updateHelpText(const wxutil::TreeModel::Row& row)
{
    assert(row.getItem().IsOk());

    // Get the highlighted key
    auto selectedKey = getSelectedKey();

    // Check the entityclass (which will return blank if not found)
    auto eclass = _entitySelection->getSingleSharedEntityClass();

    if (!eclass)
    {
        setHelpText("");
        return; // non-unique eclass, cannot show any reliable help
    }

    if (selectedKey == "classname")
    {
        // #5621: Show the editor_usage string when the classname is selected
        setHelpText(eclass::getUsage(eclass));
        return;
    }

    // Find the (inherited) attribute description
    setHelpText(eclass->getAttributeDescription(selectedKey));
}

// Update the PropertyEditor pane, displaying the PropertyEditor if necessary
// and making sure it refers to the currently-selected Entity.
void EntityInspector::_onTreeViewSelectionChanged(wxDataViewEvent& ev)
{
    ev.Skip();

    // Abort if called without a valid entity selection (may happen during
    // various cleanup operations).
    if (_entitySelection->empty()) return;

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
        std::string value = getListSelection(_modelCols.value);

        // Update key and value entry boxes, but only if there is a key value. If
        // there is no selection we do not clear the boxes, to allow keyval copying
        // between entities.
        if (!key.empty())
        {
            _keyEntry->SetValue(key);

            // Leave the entry box empty, don't store the "[differing values]" placeholder in the entry box
            _valEntry->SetValue(row[_modelCols.isMultiValue].getBool() ? "" : value);
        }

        // Update property editor, unless we're in merge mode
        if (canUpdateEntity())
        {
            // Get the type for this key if it exists, and the options
            auto type = getPropertyTypeForKey(key);

            // Set up the target key for the property editor instance
            auto targetKey = TargetKey::CreateFromString(key);

            removePropertyEditor();

            // Construct and add a new PropertyEditor
            _currentPropertyEditor = EntityInspectorModule::Instance().getPropertyEditorFactory()
                .create(_editorFrame, type, *_entitySelection, targetKey);

            if (_currentPropertyEditor)
            {
                // Get notified when the property editor applied a key value
                _propertyEditorAppliedKeyValue = _currentPropertyEditor->signal_keyValueApplied()
                    .connect(sigc::mem_fun(*this, &EntityInspector::onPropertyEditorAppliedKeyValue));

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
        removePropertyEditor();
    }

    updateEntryBoxSensitivity();
}

void EntityInspector::removePropertyEditor()
{
    _propertyEditorAppliedKeyValue.disconnect();
    _currentPropertyEditor.reset();
}

void EntityInspector::onPropertyEditorAppliedKeyValue(const std::string& key, const std::string& value)
{
    // If the property editor applied a key that is currently displayed in the entry boxes,
    // sync the value to the one that has been applied (#5700)
    if (_keyEntry->GetValue() == key && _valEntry->GetValue() != value)
    {
        _valEntry->SetValue(value);
    }
}

std::string EntityInspector::getPropertyTypeFromGame(const std::string& key)
{
    // Attempt to find the key in the property map
    for (const auto& [expression, type] : _propertyTypes)
    {
        if (expression.empty()) continue; // safety check

        // Try to match the entity key against the regex
        std::smatch matches;

        if (std::regex_match(key, matches, std::regex(expression)))
        {
            // We have a match
            return type;
        }
    }

    return {};
}

std::string EntityInspector::getPropertyTypeForKey(const std::string& key)
{
    // Check the local mappings first
    auto type = getPropertyTypeFromGame(key);

    if (!type.empty())
    {
        return type;
    }

    // Try to get the type from the entity class (editor_* key values)
    auto sharedEntityClass = _entitySelection->getSingleSharedEntityClass();

    if (sharedEntityClass)
    {
        type = sharedEntityClass->getAttributeType(key);
    }

    if (!type.empty())
    {
        return type;
    }

    // Finally, look for key types registered on the named attachment
    return getPropertyTypeForAttachmentKey(key);
}

std::string EntityInspector::getPropertyTypeForAttachmentKey(const std::string& key)
{
    // Check for keys using the "set X on Y" pattern to set keyvalues on attachments
    std::regex pattern(TargetKey::SetKeyPattern, std::regex::icase);

    std::smatch match;
    if (!std::regex_match(key, match, pattern)) return {};

    auto attachmentKey = match[1].str();
    auto attachmentName = match[2].str();

    // Find a property type in our local mappings
    auto locallyMappedType = getPropertyTypeForKey(attachmentKey);

    if (!locallyMappedType.empty())
    {
        return locallyMappedType;
    }

    // Check the editor_* definition for this key on the attachment eclass
    // Check if there is a single attachment eclass on all selected entities
    std::optional<std::string> attachmentClass;

    _entitySelection->foreachEntity([&](const IEntityNodePtr& entity)
    {
        entity->getEntity().forEachAttachment([&](const Entity::Attachment& attachment)
        {
            if (attachment.name != attachmentName) return;

            if (!attachmentClass.has_value())
            {
                attachmentClass = attachment.eclass;
                return;
            }

            // Check if the attachment is unique
            if (attachment.eclass != attachmentClass.value())
            {
                attachmentClass.value().clear();
            }
        });
    });

    // Nothing found or not uniquely identified
    if (attachmentClass.has_value() && !attachmentClass.value().empty())
    {
        // Check the key of this attachment instead
        if (auto eclass = GlobalEntityClassManager().findClass(attachmentClass.value()); eclass)
        {
            const auto& keyType = eclass->getAttributeType(attachmentKey);

            if (!keyType.empty())
            {
                return keyType;
            }
        }
    }

    return {};
}

void EntityInspector::updateListedKeyTypes()
{
    _kvStore->ForeachNode([&](wxutil::TreeModel::Row& row)
    {
        updateKeyType(row);
        row.SendItemChanged();
    });
}

void EntityInspector::addClassAttribute(const EntityClassAttribute& a)
{
    // Only add properties with values, we don't want the optional
    // "editor_var xxx" properties here.
    if (a.getValue().empty()) return;

    auto row = _kvStore->AddItem();

    row[_modelCols.name] = wxVariant(wxDataViewIconText(a.getName(), _emptyIcon));
    row[_modelCols.value] = a.getValue();

    // Load the correct icon for this key
    updateKeyType(row);

    // Inherited values have an inactive checkbox, so assign a false value and disable
    if (a.getType() == "bool")
    {
        row[_modelCols.booleanValue] = a.getValue() == "1";
    }
    else
    {
        row[_modelCols.booleanValue] = false;
        row[_modelCols.booleanValue].setEnabled(false);
    }

    // Set style attributes
    auto style = wxutil::TreeViewItemStyle::Inherited();
    row[_modelCols.name].setAttr(style);
    row[_modelCols.value].setAttr(style);

    row[_modelCols.oldValue] = std::string();
    row[_modelCols.newValue] = std::string();

    row[_modelCols.isInherited] = true;

    row.SendItemAdded();
}

// Append inherited (entityclass) properties
void EntityInspector::addClassProperties()
{
    // Visit the entityclass for the current entities
    if (auto eclass = _entitySelection->getSingleSharedEntityClass(); eclass) {
        eclass->forEachAttribute(
            [&](const EntityClassAttribute& a, bool) { addClassAttribute(a); }
        );
    }
}

// Remove the inherited properties
void EntityInspector::removeClassProperties()
{
    _kvStore->RemoveItems([&] (const wxutil::TreeModel::Row& row)->bool
    {
        // If this is an inherited row, remove it
        return row[_modelCols.isInherited].getBool();
    });
}

void EntityInspector::handleKeyValueMergeAction(const scene::merge::IEntityKeyValueMergeAction::Ptr& mergeAction)
{
    const auto& key = mergeAction->getKey();

    // Remember this action in the map, it will be used in onKeyChange()
    _mergeActions[key] = mergeAction;

    // Keys added by a merge operation won't be handled in onKeyChange(), so do this here
    if (mergeAction->getType() == scene::merge::ActionType::AddKeyValue ||
        (mergeAction->getType() == scene::merge::ActionType::ChangeKeyValue && !_spawnargs->containsKey(key)))
    {
        onKeyChange(key, mergeAction->getValue());
    }

    // Remove keys are special too
    if (mergeAction->getType() == scene::merge::ActionType::RemoveKeyValue && !_spawnargs->containsKey(key))
    {
        onKeyChange(key, mergeAction->getUnchangedValue());
    }
}

void EntityInspector::handleMergeActions(const scene::INodePtr& selectedNode)
{
    if (!selectedNode || selectedNode->getNodeType() != scene::INode::Type::MergeAction)
    {
        return;
    }

    auto mergeNode = std::dynamic_pointer_cast<scene::IMergeActionNode>(selectedNode);

    if (!mergeNode) return;

    // Collect all the merge actions and conflict info first
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

    // Now load the regular key values from the entity into the view
    auto affectedEntity = Node_getEntity(mergeNode->getAffectedNode());

    if (!affectedEntity) return; // not an entity we're looking at

    affectedEntity->forEachKeyValue([&](const std::string& key, const std::string& value)
    {
        onKeyChange(key, value, false);
    });
}

} // namespace ui
