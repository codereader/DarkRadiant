#pragma once

#include "PropertyEditor.h"

#include "ui/ientityinspector.h"
#include "imap.h"
#include "imapmerge.h"
#include "iselection.h"
#include "string/string.h"
#include "wxutil/menu/PopupMenu.h"
#include "wxutil/PanedPosition.h"
#include "wxutil/dataview/TreeView.h"
#include "wxutil/event/SingleIdleCallback.h"
#include "wxutil/Icon.h"

#include <wx/event.h>
#include <sigc++/connection.h>

#include <map>

#include "wxutil/DockablePanel.h"

/* FORWARD DECLS */

class Entity;
class Selectable;
class EntityClassAttribute;
class wxCheckBox;
class wxStaticText;
class wxTextCtrl;
class wxBitmapButton;
class wxDataViewColumn;

namespace selection
{
    class CollectiveSpawnargs;
    class EntitySelection;
}

namespace ui
{

/**
 * The EntityInspector class represents the GTK dialog for editing properties
 * on the selected game entity. The class is implemented as a singleton and
 * contains a method to return the current instance.
 */
class EntityInspector final :
    public wxutil::DockablePanel,
    public selection::SelectionSystem::Observer,
    public wxutil::SingleIdleCallback
{
public:
    struct TreeColumns :
        public wxutil::TreeModel::ColumnRecord
    {
        TreeColumns() :
            name(add(wxutil::TreeModel::Column::IconText)),
            value(add(wxutil::TreeModel::Column::String)),
            isInherited(add(wxutil::TreeModel::Column::Boolean)),
            isMultiValue(add(wxutil::TreeModel::Column::Boolean)),
            booleanValue(add(wxutil::TreeModel::Column::Boolean)),
            oldValue(add(wxutil::TreeModel::Column::String)),
            newValue(add(wxutil::TreeModel::Column::String))
        {}

        wxutil::TreeModel::Column name;
        wxutil::TreeModel::Column value;
        wxutil::TreeModel::Column isInherited;
        wxutil::TreeModel::Column isMultiValue; // non-unique value (when multiple entities are selected)
        wxutil::TreeModel::Column booleanValue;
        wxutil::TreeModel::Column oldValue; // when displaying merge changes
        wxutil::TreeModel::Column newValue; // when displaying merge changes
    };

private:
    // Tracking helpers to organise the selected entities and their key values
    std::unique_ptr<selection::CollectiveSpawnargs> _spawnargs;
    std::unique_ptr<selection::EntitySelection> _entitySelection;

	// Frame to contain the Property Editor
	wxPanel* _editorFrame;

    // The checkbox for showing the eclass properties
    wxCheckBox* _showInheritedCheckbox = nullptr;
    wxCheckBox* _showHelpColumnCheckbox = nullptr;

    // A label showing the primitive number
    wxStaticText* _primitiveNumLabel = nullptr;

    // View and model for the keyvalue list
    wxutil::TreeView* _keyValueTreeView;
    TreeColumns _modelCols;
    wxutil::TreeModel::Ptr _kvStore;
    struct {
        wxDataViewColumn* boolean = nullptr;
        wxDataViewColumn* value = nullptr;
        wxDataViewColumn* oldValue = nullptr;
        wxDataViewColumn* newValue = nullptr;
    } _viewCols;

    wxutil::Icon _emptyIcon;

    // Cache of wxDataViewItems pointing to keyvalue rows,
    // so we can quickly find existing keys to change their values
    typedef std::map<std::string, wxDataViewItem, string::ILess> TreeIterMap;
    TreeIterMap _keyValueIterMap;

    // Key and value edit boxes. These remain available even for multiple entity
    // selections.
    wxTextCtrl* _keyEntry = nullptr;
    wxTextCtrl* _valEntry = nullptr;
    wxBitmapButton* _setButton = nullptr;

    wxTextCtrl* _helpText;

    // The pane dividing the treeview and the property editors
    wxSplitterWindow* _paned;

    // An object tracking the divider position of the paned view
    wxutil::PanedPosition _panedPosition;

    // Context menu
    wxutil::PopupMenuPtr _contextMenu;

    // Currently displayed PropertyEditor
    IPropertyEditor::Ptr _currentPropertyEditor;
    sigc::connection _propertyEditorAppliedKeyValue;

    // The clipboard for spawnargs
    typedef std::pair<std::string, std::string> KeyValuePair;
    typedef std::vector<KeyValuePair> ClipBoard;
    ClipBoard _clipboard;

    // Map of property names to type, mapped like this: regex => type
    std::map<std::string, std::string> _propertyTypes;

    sigc::connection _defsReloadedHandler;
    sigc::connection _mapEditModeChangedHandler;

    sigc::connection _keyValueAddedHandler;
    sigc::connection _keyValueRemovedHandler;
    sigc::connection _keyValueSetChangedHandler;

    // Maps the key names to a possible merge action that should be displayed
    std::map<std::string, scene::merge::IEntityKeyValueMergeAction::Ptr> _mergeActions;
    std::map<std::string, scene::merge::IConflictResolutionAction::Ptr> _conflictActions;

    bool _selectionNeedsUpdate = true;
    bool _inheritedPropertiesNeedUpdate = true;
    bool _helpTextNeedsUpdate = true;

private:
    bool canUpdateEntity();

    // Utility functions to construct the Gtk components
    void construct();

    wxWindow* createPropertyEditorPane(wxWindow* parent); // bottom widget pane
    wxWindow* createTreeViewPane(wxWindow* parent); // tree view for selecting attributes
    void createContextMenu();

    // Utility function to retrieve the string selection from the given column in the
    // list store
    // getSelectedKey() returns an empty string if nothing is selected
    std::string getSelectedKey();
    std::string getListSelection(const wxutil::TreeModel::Column& col);
    bool getListSelectionBool(const wxutil::TreeModel::Column& col);

    // wxutil::PopupMenu callbacks
    void _onAddKey();
    void _onDeleteKey();
    void _onCopyKey();
    void _onCutKey();
    void _onPasteKey();
    void _onAcceptMergeAction();
    void _onRejectMergeAction();

    bool _testAddKey();
    bool _testDeleteKey();
    bool _testCopyKey();
    bool _testCutKey();
    bool _testPasteKey();
    bool _testAcceptMergeAction();
    bool _testRejectMergeAction();

    // Shared by cut and delete keys
    bool _testNonEmptyAndDeletableSelection();
    bool isItemDeletable(const wxutil::TreeModel::Row& row);
    bool isItemAffecedByMergeConflict(const wxutil::TreeModel::Row& row);
    bool isItemAffecedByMergeOperation(const wxutil::TreeModel::Row& row);

    // callbacks
    void _onEntryActivate(wxCommandEvent& ev);
    void _onSetProperty(wxCommandEvent& ev);
    void _onTreeViewSelectionChanged(wxDataViewEvent& ev);
    void _onContextMenu(wxDataViewEvent& ev);
    void _onDataViewItemChanged(wxDataViewEvent& ev);

    void onKeyAdded(const std::string& key, const std::string& value);
    void onKeyRemoved(const std::string& key);
    void onKeyValueSetChanged(const std::string& key, const std::string& uniqueValue);

    // Routines shared by onKeyAdded, onKeyRemoved and onKeyValueSetChanged
    void onKeyUpdatedCommon(const std::string& key);

    void handleShowInheritedChanged();
    void updateHelpTextPanel();
    void updateHelpText(const wxutil::TreeModel::Row& row);
    void setHelpText(const std::string& newText);
    void updatePrimitiveNumber();
    static std::string  cleanInputString( const std::string& );

    // Add and remove inherited properties from the entity class
    void addClassAttribute(const EntityClassAttribute& a);
    void addClassProperties();
    void removeClassProperties();

    void applyMergeActionStyle(const std::string& key, wxDataViewItemAttr& style);
    void setOldAndNewValueColumns(wxutil::TreeModel::Row& row, const std::string& key, const wxDataViewItemAttr& style);

    void handleMergeActions(const scene::INodePtr& selectedNode);
    void handleKeyValueMergeAction(const scene::merge::IEntityKeyValueMergeAction::Ptr& mergeAction);

    // Set the keyval on all selected entities from the key and value textboxes
    void setPropertyFromEntries();

    void removePropertyEditor();
    void onPropertyEditorAppliedKeyValue(const std::string& key, const std::string& value);

    // Applies the given key/value pair to the selection (works with multiple
    // selected entities)
    void applyKeyValueToSelection(const std::string& key,
                                  const std::string& value);

    // Initialise the property lookup tables
    void loadPropertyMap();

    // Returns property type and option for the given entity key
    std::string getPropertyTypeFromGame(const std::string& key);
    std::string getPropertyTypeForKey(const std::string& key);
    std::string getPropertyTypeForAttachmentKey(const std::string& key);
    void updateListedKeyTypes();
    void updateKeyType(wxutil::TreeModel::Row& row);

    // Update tree view contents and property editor
    void updateGUIElements();

    void updateEntryBoxSensitivity();

    // Release the current entity and rescan the selection
    void refresh();

    void onMapEditModeChanged(IMap::EditMode mode);
    void onDefsReloaded();

	// greebo: Tells the inspector to reload the window settings from the registry.
	void restoreSettings();

    void connectListeners();
    void disconnectListeners();

protected:
    // Called when the app is idle
    void onIdle() override;

    void onPanelActivated() override;
    void onPanelDeactivated() override;

public:
    EntityInspector(wxWindow* parent);
    ~EntityInspector() override;

    /** greebo: Gets called by the RadiantSelectionSystem upon selection change.
     */
    void selectionChanged(const scene::INodePtr& node, bool isComponent) override;

    void onKeyChange(const std::string& key, const std::string& value, bool isMultiValue = false);
};

} // namespace ui
