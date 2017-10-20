#pragma once

#include "PropertyEditor.h"

#include "ientityinspector.h"
#include "iradiant.h"
#include "icommandsystem.h"
#include "iselection.h"
#include "ientity.h"
#include "string/string.h"
#include "wxutil/menu/PopupMenu.h"
#include "wxutil/PanedPosition.h"
#include "wxutil/TreeView.h"
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

namespace ui
{

class EntityInspector;
typedef std::shared_ptr<EntityInspector> EntityInspectorPtr;

/**
 * The EntityInspector class represents the GTK dialog for editing properties
 * on the selected game entity. The class is implemented as a singleton and
 * contains a method to return the current instance.
 */
class EntityInspector :
	public IEntityInspector,
 	public SelectionSystem::Observer,
    public wxutil::SingleIdleCallback,
    public Entity::Observer,
	public std::enable_shared_from_this<EntityInspector>
{
public:
	struct TreeColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		TreeColumns() :
			name(add(wxutil::TreeModel::Column::IconText)),
			value(add(wxutil::TreeModel::Column::String)),
			isInherited(add(wxutil::TreeModel::Column::Boolean)),
			helpIcon(add(wxutil::TreeModel::Column::Icon)),
			hasHelpText(add(wxutil::TreeModel::Column::Boolean)),
			booleanValue(add(wxutil::TreeModel::Column::Boolean))
		{}

		wxutil::TreeModel::Column name;
		wxutil::TreeModel::Column value;
		wxutil::TreeModel::Column isInherited;
		wxutil::TreeModel::Column helpIcon;
		wxutil::TreeModel::Column hasHelpText;
		wxutil::TreeModel::Column booleanValue;
	};

private:
	struct StringCompareFunctorNoCase :
		public std::binary_function<std::string, std::string, bool>
	{
		bool operator()(const std::string& s1, const std::string& s2) const
		{
			// return boost::algorithm::ilexicographical_compare(s1, s2); // slow!
			return string_compare_nocase(s1.c_str(), s2.c_str()) < 0;
		}
	};

	// Currently selected entity, this pointer is only non-NULL if the
	// current entity selection includes exactly 1 entity.
	scene::INodeWeakPtr _selectedEntity;

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

	wxIcon _emptyIcon;

	wxDataViewColumn* _helpColumn;

    // Cache of wxDataViewItems pointing to keyvalue rows,
	// so we can quickly find existing keys to change their values
	typedef std::map<std::string, wxDataViewItem, StringCompareFunctorNoCase> TreeIterMap;
    TreeIterMap _keyValueIterMap;

	// Key and value edit boxes. These remain available even for multiple entity
    // selections.
	wxTextCtrl* _keyEntry;
	wxTextCtrl* _valEntry;

	wxTextCtrl* _helpText;

	// The pane dividing the treeview and the property editors
	wxSplitterWindow* _paned;

	// An object tracking the divider position of the paned view
	wxutil::PanedPosition _panedPosition;

	// Context menu
	wxutil::PopupMenuPtr _contextMenu;

	// Currently displayed PropertyEditor
	IPropertyEditorPtr _currentPropertyEditor;

	// The clipboard for spawnargs
	typedef std::pair<std::string, std::string> KeyValuePair;
	typedef std::vector<KeyValuePair> ClipBoard;
	ClipBoard _clipBoard;

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

	sigc::connection _undoHandler;
	sigc::connection _redoHandler;

private:

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

	bool _testAddKey();
	bool _testDeleteKey();
	bool _testCopyKey();
	bool _testCutKey();
	bool _testPasteKey();

    // callbacks
	void _onEntryActivate(wxCommandEvent& ev);
	void _onSetProperty(wxCommandEvent& ev);
	void _onToggleShowInherited(wxCommandEvent& ev);
	void _onToggleShowHelpIcons(wxCommandEvent& ev);
	void _onTreeViewSelectionChanged(wxDataViewEvent& ev);
	void _onContextMenu(wxDataViewEvent& ev);
	void _onDataViewItemChanged(wxDataViewEvent& ev);

	void updateHelpText(const wxutil::TreeModel::Row& row);
    static std::string  cleanInputString( const std::string& );

    // Add and remove inherited properties from the entity class
    void addClassAttribute(const EntityClassAttribute& a);
    void addClassProperties();
    void removeClassProperties();

    // Update our selected entity pointer from the selection system
    void getEntityFromSelectionSystem();

    // Change the selected entity pointer, setting up the observer
    void changeSelectedEntity(const scene::INodePtr& newEntity);

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

    // Update tree view contents and property editor
    void updateGUIElements();

	// Gets called after an undo operation
	void onUndoRedoOperation();

protected:
    // Called when the app is idle
    void onIdle();

public:
	// Constructor
    EntityInspector();

	// Get the main widget for packing
	wxPanel* getWidget();

	/** greebo: Gets called by the RadiantSelectionSystem upon selection change.
	 */
	void selectionChanged(const scene::INodePtr& node, bool isComponent);

	void registerPropertyEditor(const std::string& key, const IPropertyEditorPtr& editor);
	IPropertyEditorPtr getRegisteredPropertyEditor(const std::string& key);
	void unregisterPropertyEditor(const std::string& key);

	void onRadiantStartup();
	void onRadiantShutdown();

	/* Entity::Observer implementation */
    void onKeyInsert(const std::string& key, EntityKeyValue& value);
    void onKeyChange(const std::string& key, const std::string& value);
    void onKeyErase(const std::string& key, EntityKeyValue& value);

	// greebo: Tells the inspector to reload the window settings from the registry.
	void restoreSettings();

	/**
	 * greebo: Static command target for toggling the Entity Inspector in the GroupDialog.
	 */
	static void toggle(const cmd::ArgumentList& args);

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
};

} // namespace ui
