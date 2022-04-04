#pragma once

#include "PropertyEditor.h"

#include "ui/ientityinspector.h"
#include "iradiant.h"
#include "imap.h"
#include "icommandsystem.h"
#include "imapmerge.h"
#include "iselection.h"
#include "ientity.h"
#include "string/string.h"
#include "wxutil/menu/PopupMenu.h"
#include "wxutil/PanedPosition.h"
#include "wxutil/dataview/TreeView.h"
#include "wxutil/event/SingleIdleCallback.h"

#include <wx/event.h>
#include <wx/icon.h>
#include <sigc++/connection.h>

#include <map>

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

class PropertyEditorFactory;

/**
 * The EntityInspector class represents the GTK dialog for editing properties
 * on the selected game entity. The class is implemented as a singleton and
 * contains a method to return the current instance.
 */
class EntityInspector final :
	public IEntityInspector,
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
    std::unique_ptr<PropertyEditorFactory> _propertyEditorFactory;

    // Tracking helpers to organise the selected entities and their key values
    std::unique_ptr<selection::CollectiveSpawnargs> _spawnargs;
    std::unique_ptr<selection::EntitySelection> _entitySelection;

	// Main EntityInspector widget
	wxPanel* _mainWidget;

	// Frame to contain the Property Editor
	wxPanel* _editorFrame;

	// The checkbox for showing the eclass properties
	wxCheckBox* _showInheritedCheckbox;
	wxCheckBox* _showHelpColumnCheckbox;

	// A label showing the primitive number
	wxStaticText* _primitiveNumLabel;

    // View and model for the keyvalue list
	wxutil::TreeView* _keyValueTreeView;
	TreeColumns _columns;
	wxutil::TreeModel::Ptr _kvStore;
    wxDataViewColumn* _booleanColumn;
    wxDataViewColumn* _valueColumn;
    wxDataViewColumn* _oldValueColumn;
    wxDataViewColumn* _newValueColumn;

	wxIcon _emptyIcon;

    // Cache of wxDataViewItems pointing to keyvalue rows,
	// so we can quickly find existing keys to change their values
	typedef std::map<std::string, wxDataViewItem, string::ILess> TreeIterMap;
    TreeIterMap _keyValueIterMap;

	// Key and value edit boxes. These remain available even for multiple entity
    // selections.
	wxTextCtrl* _keyEntry;
	wxTextCtrl* _valEntry;
    wxBitmapButton* _setButton;

	wxTextCtrl* _helpText;

	// The pane dividing the treeview and the property editors
	wxSplitterWindow* _paned;

	// An object tracking the divider position of the paned view
	wxutil::PanedPosition _panedPosition;

	// Context menu
	wxutil::PopupMenuPtr _contextMenu;

	// Currently displayed PropertyEditor
	IPropertyEditor::Ptr _currentPropertyEditor;

	// The clipboard for spawnargs
	typedef std::pair<std::string, std::string> KeyValuePair;
	typedef std::vector<KeyValuePair> ClipBoard;
	ClipBoard _clipboard;

	// Data structure to store the type (vector3, text etc) and the options
	// string for a single property.
	struct PropertyParms
	{
		std::string type;
		std::string options;
	};

	// Map of property names to PropertyParms, mapped like this: regex => parms
	typedef std::map<std::string, PropertyParms> PropertyParmMap;
	PropertyParmMap _propertyTypes;

	sigc::connection _defsReloadedHandler;
	sigc::connection _mapEditModeChangedHandler;

	sigc::connection _keyValueAddedHandler;
	sigc::connection _keyValueRemovedHandler;
	sigc::connection _keyValueSetChangedHandler;

    // Maps the key names to a possible merge action that should be displayed
    std::map<std::string, scene::merge::IEntityKeyValueMergeAction::Ptr> _mergeActions;
    std::map<std::string, scene::merge::IConflictResolutionAction::Ptr> _conflictActions;

    bool _selectionNeedsUpdate;
    bool _inheritedPropertiesNeedUpdate;
    bool _helpTextNeedsUpdate;

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

    // Applies the given key/value pair to the selection (works with multiple
    // selected entities)
    void applyKeyValueToSelection(const std::string& key,
                                  const std::string& value);

	// Initialise the property lookup tables
	void loadPropertyMap();

	// Returns property type and option for the given entity key
	PropertyParms getPropertyParmsForKey(const std::string& key);
	std::string getPropertyTypeForKey(const std::string& key);
    void updateListedKeyTypes();
    void updateKeyType(wxutil::TreeModel::Row& row);

    // Update tree view contents and property editor
    void updateGUIElements();

    void updateEntryBoxSensitivity();

    // Release the current entity and rescan the selection
    void refresh();

    void onMapEditModeChanged(IMap::EditMode mode);
	void onDefsReloaded();

protected:
    // Called when the app is idle
    void onIdle() override;

public:
	// Constructor
    EntityInspector();

	// Get the main widget for packing
	wxPanel* getWidget() override;

	/** greebo: Gets called by the RadiantSelectionSystem upon selection change.
	 */
	void selectionChanged(const scene::INodePtr& node, bool isComponent) override;

    void registerPropertyEditor(const std::string& key,
                                const IPropertyEditor::CreationFunc& creator) override;
    void unregisterPropertyEditor(const std::string& key) override;
    void registerPropertyEditorDialog(const std::string& key,
                                      const IPropertyEditorDialog::CreationFunc& create) override;
    IPropertyEditorDialog::Ptr createDialog(const std::string& key) override;
    void unregisterPropertyEditorDialog(const std::string& key) override;

	void onMainFrameConstructed();
	void onMainFrameShuttingDown();

    void onKeyChange(const std::string& key, const std::string& value, bool isMultiValue = false);

	// greebo: Tells the inspector to reload the window settings from the registry.
	void restoreSettings() override;

	/**
	 * greebo: Static command target for toggling the Entity Inspector in the GroupDialog.
	 */
	static void toggle(const cmd::ArgumentList& args);

    // RegisterableModule implementation
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;
};

} // namespace ui
