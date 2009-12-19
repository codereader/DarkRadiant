#include "EntityInspector.h"
#include "PropertyEditorFactory.h"
#include "AddPropertyDialog.h"

#include "ientity.h"
#include "ieclass.h"
#include "iregistry.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "imainframe.h"

#include "modulesystem/StaticModule.h"
#include "selectionlib.h"
#include "scenelib.h"
#include "gtkutil/dialog.h"
#include "gtkutil/Paned.h"
#include "gtkutil/StockIconMenuItem.h"
#include "gtkutil/SeparatorMenuItem.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/ScrolledFrame.h"
#include "xmlutil/Document.h"
#include "signal/signal.h"
#include "map/Map.h"
#include "selection/algorithm/Entity.h"

#include <map>
#include <string>

#include <gtk/gtk.h>

#include <boost/bind.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/regex.hpp>

namespace ui {

/* CONSTANTS */

namespace {

    const int TREEVIEW_MIN_WIDTH = 220;
    const int TREEVIEW_MIN_HEIGHT = 60;

    const char* PROPERTY_NODES_XPATH = "game/entityInspector//property";

	const std::string RKEY_ROOT = "user/ui/entityInspector/";
	const std::string RKEY_PANE_STATE = RKEY_ROOT + "pane";

	const std::string HELP_ICON_NAME = "helpicon.png";

	// TreeView column numbers
    enum {
        PROPERTY_NAME_COLUMN,
        PROPERTY_VALUE_COLUMN,
        TEXT_COLOUR_COLUMN,
        PROPERTY_ICON_COLUMN,
        INHERITED_FLAG_COLUMN,
		HELP_ICON_COLUMN,
		HAS_HELP_FLAG_COLUMN,
        N_COLUMNS
    };

}

EntityInspector::EntityInspector() : 
	_selectedEntity(NULL),
	_kvStore(NULL),
	_keyValueTreeView(NULL),
	_helpColumn(NULL)
{}

void EntityInspector::construct()
{
	_kvStore = gtk_list_store_new(N_COLUMNS,
                              G_TYPE_STRING,		// property
                              G_TYPE_STRING,		// value
                              G_TYPE_STRING,		// text colour
                              GDK_TYPE_PIXBUF,	    // value icon
                              G_TYPE_STRING,		// inherited flag
                              GDK_TYPE_PIXBUF,	    // help icon
                              G_TYPE_BOOLEAN);	    // has help
  
	_keyValueTreeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_kvStore));

	_paned = gtkutil::PanedPtr(new gtkutil::Paned(gtkutil::Paned::Vertical)); // vertical pane
	_contextMenu = gtkutil::PopupMenuPtr(new gtkutil::PopupMenu(_keyValueTreeView));

    _widget = gtk_vbox_new(FALSE, 0);

	// Pack in GUI components

	GtkWidget* topHBox = gtk_hbox_new(FALSE, 6);

    // Show inherited properties checkbutton
    _showInheritedCheckbox = gtk_check_button_new_with_label(
        "Show inherited properties"
    );
	g_signal_connect(
        G_OBJECT(_showInheritedCheckbox), "toggled",
        G_CALLBACK(_onToggleShowInherited), this
    );
	gtk_box_pack_start(
        GTK_BOX(topHBox), _showInheritedCheckbox, FALSE, FALSE, 0
    );

	_showHelpColumnCheckbox = gtk_check_button_new_with_label("Show help icons");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_showHelpColumnCheckbox), FALSE);
	g_signal_connect(G_OBJECT(_showHelpColumnCheckbox), "toggled", G_CALLBACK(_onToggleShowHelpIcons), this);

	gtk_box_pack_start(GTK_BOX(topHBox), _showHelpColumnCheckbox, FALSE, FALSE, 0);

    // Add checkbutton hbox to main widget
	gtk_box_pack_start(GTK_BOX(_widget), topHBox, FALSE, FALSE, 0);

	// Pack everything into the paned container
	_paned->setFirstChild(createTreeViewPane());
	_paned->setSecondChild(createPropertyEditorPane());

	gtk_box_pack_start(GTK_BOX(_widget), _paned->getWidget(), TRUE, TRUE, 0);

	_panedPosition.connect(_paned->getWidget());

	// Reload the information from the registry
	restoreSettings();

    // Create the context menu
    createContextMenu();

    // Stimulate initial redraw to get the correct status
    updateGUIElements();

    // Register self to the SelectionSystem to get notified upon selection
    // changes.
    GlobalSelectionSystem().addObserver(this);

	// Observe the Undo system for undo/redo operations, to refresh the 
	// keyvalues when this happens
	GlobalUndoSystem().addObserver(this);

	// initialise the properties
	loadPropertyMap();
}

void EntityInspector::restoreSettings() 
{
	// Find the information stored in the registry
	if (GlobalRegistry().keyExists(RKEY_PANE_STATE)) {
		_panedPosition.loadFromPath(RKEY_PANE_STATE);
	}
	else {
		// No saved information, apply standard value
		_panedPosition.setPosition(400);
	}

	_panedPosition.applyPosition();
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
    GtkTreeIter keyValueIter;

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
        gtk_list_store_append(_kvStore, &keyValueIter);
        _keyValueIterMap.insert(TreeIterMap::value_type(key, keyValueIter));
    }

    // Look up type for this key. First check the property parm map,
    // then the entity class itself. If nothing is found, leave blank.
	// Get the type for this key if it exists, and the options
	PropertyParms parms = getPropertyParmsForKey(key);

	// Check the entityclass (which will return blank if not found)
	IEntityClassConstPtr eclass = _selectedEntity->getEntityClass();
	const EntityClassAttribute& attr = eclass->getAttribute(key);

    if (parms.type.empty()) 
    {
        parms.type = attr.type;
    }

    bool hasDescription = !attr.description.empty();

    // Set the values for the row
    gtk_list_store_set(
        _kvStore, 
        &keyValueIter,
        PROPERTY_NAME_COLUMN, key.c_str(),
        PROPERTY_VALUE_COLUMN, value.c_str(),
        TEXT_COLOUR_COLUMN, "black",
        PROPERTY_ICON_COLUMN, PropertyEditorFactory::getPixbufFor(parms.type),
        INHERITED_FLAG_COLUMN, "", // not inherited
        HELP_ICON_COLUMN, hasDescription 
                          ? GlobalUIManager().getLocalPixbuf(HELP_ICON_NAME) 
                          : NULL,
        HAS_HELP_FLAG_COLUMN, hasDescription ? TRUE : FALSE,
        -1
    );

	// Check if we should update the key/value entry boxes
	std::string curKey = gtk_entry_get_text(GTK_ENTRY(_keyEntry));

	std::string selectedKey = getListSelection(PROPERTY_NAME_COLUMN);

	// If the key in the entry box matches the key which got changed,
	// update the value accordingly, otherwise leave it alone. This is to fix
	// the entry boxes not being updated when a PropertyEditor is changing the value.
	// Therefore only do this if the selectedKey is matching too.
	if (curKey == key && selectedKey == key)
	{
		gtk_entry_set_text(GTK_ENTRY(_valEntry), value.c_str());
	}
}

void EntityInspector::onKeyErase(const std::string& key,
                                 EntityKeyValue& value)
{
    // Look up iter in the TreeIter map, and delete it from the list store
    TreeIterMap::iterator i = _keyValueIterMap.find(key);
    if (i != _keyValueIterMap.end())
    {
        GtkTreeIter treeIter = i->second;

        // Erase row from tree store
        gtk_list_store_remove(_kvStore, &treeIter);

        // Erase iter from iter map
        _keyValueIterMap.erase(i);
    }
    else
    {
        std::cerr << "EntityInspector: warning: removed key '" << key 
                  << "' not found in map." << std::endl;
    }
}

// Create the context menu
void EntityInspector::createContextMenu() 
{
	_contextMenu->addItem(
		gtkutil::StockIconMenuItem(GTK_STOCK_ADD, "Add property..."),
		boost::bind(&EntityInspector::_onAddKey, this)
	);
	_contextMenu->addItem(
		gtkutil::StockIconMenuItem(GTK_STOCK_DELETE, "Delete property"),
		boost::bind(&EntityInspector::_onDeleteKey, this),
		boost::bind(&EntityInspector::_testDeleteKey, this)
	);

	_contextMenu->addItem(gtkutil::SeparatorMenuItem(), gtkutil::PopupMenu::Callback());

	_contextMenu->addItem(
		gtkutil::StockIconMenuItem(GTK_STOCK_COPY, "Copy Spawnarg"),
		boost::bind(&EntityInspector::_onCopyKey, this),
		boost::bind(&EntityInspector::_testCopyKey, this)
	);
	_contextMenu->addItem(
		gtkutil::StockIconMenuItem(GTK_STOCK_CUT, "Cut Spawnarg"),
		boost::bind(&EntityInspector::_onCutKey, this),
		boost::bind(&EntityInspector::_testCutKey, this)
	);
	_contextMenu->addItem(
		gtkutil::StockIconMenuItem(GTK_STOCK_PASTE, "Paste Spawnarg"),
		boost::bind(&EntityInspector::_onPasteKey, this),
		boost::bind(&EntityInspector::_testPasteKey, this)
	);
}

void EntityInspector::onRadiantShutdown() {
	// Remove all previously stored pane information
	_panedPosition.saveToPath(RKEY_PANE_STATE);
}

void EntityInspector::postUndo()
{
	// Clear the previous entity (detaches this class as observer)
	changeSelectedEntity(NULL);

	// Now rescan the selection and update the stores
	updateGUIElements();
}

void EntityInspector::postRedo()
{
	// Clear the previous entity (detaches this class as observer)
	changeSelectedEntity(NULL);

	// Now rescan the selection and update the stores
	updateGUIElements();
}

const std::string& EntityInspector::getName() const
{
	static std::string _name(MODULE_ENTITYINSPECTOR);
	return _name;
}

const StringSet& EntityInspector::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_UIMANAGER);
		_dependencies.insert(MODULE_SELECTIONSYSTEM);
		_dependencies.insert(MODULE_UNDOSYSTEM);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_EVENTMANAGER);
	}

	return _dependencies;
}

void EntityInspector::initialiseModule(const ApplicationContext& ctx)
{
	construct();

	GlobalRadiant().addEventListener(shared_from_this());

	GlobalCommandSystem().addCommand("ToggleEntityInspector", toggle);
	GlobalEventManager().addCommand("ToggleEntityInspector", "ToggleEntityInspector");
}

void EntityInspector::registerPropertyEditor(const std::string& key, const IPropertyEditorPtr& editor)
{
	PropertyEditorFactory::registerPropertyEditor(key, editor);
}

// Return the Gtk widget for the EntityInspector dialog.

GtkWidget* EntityInspector::_getWidget() const
{
    return _widget;
}

// Create the dialog pane
GtkWidget* EntityInspector::createPropertyEditorPane() 
{
	GtkWidget* hbx = gtk_hbox_new(FALSE, 0);
    _editorFrame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(_editorFrame), GTK_SHADOW_NONE);
    gtk_box_pack_start(GTK_BOX(hbx), _editorFrame, TRUE, TRUE, 0);
    return hbx;
}

// Create the TreeView pane

GtkWidget* EntityInspector::createTreeViewPane()
{
    GtkWidget* vbx = gtk_vbox_new(FALSE, 3);

    // Create the Property column
    GtkTreeViewColumn* nameCol = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(nameCol, "Property");
	gtk_tree_view_column_set_sizing(nameCol, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_spacing(nameCol, 3);

	GtkCellRenderer* pixRenderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(nameCol, pixRenderer, FALSE);
    gtk_tree_view_column_set_attributes(nameCol, pixRenderer, "pixbuf", PROPERTY_ICON_COLUMN, NULL);

    GtkCellRenderer* textRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(nameCol, textRenderer, FALSE);
    gtk_tree_view_column_set_attributes(nameCol, textRenderer,
                                        "text", PROPERTY_NAME_COLUMN,
                                        "foreground", TEXT_COLOUR_COLUMN,
                                        NULL);

	gtk_tree_view_column_set_sort_column_id(nameCol, PROPERTY_NAME_COLUMN);
    gtk_tree_view_append_column(GTK_TREE_VIEW(_keyValueTreeView), nameCol);

	// Create the value column
    GtkTreeViewColumn* valCol = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(valCol, "Value");
	gtk_tree_view_column_set_sizing(valCol, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    GtkCellRenderer* valRenderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(valCol, valRenderer, TRUE);
    gtk_tree_view_column_set_attributes(valCol, valRenderer,
    									"text", PROPERTY_VALUE_COLUMN,
    									"foreground", TEXT_COLOUR_COLUMN,
    									NULL);

	gtk_tree_view_column_set_sort_column_id(valCol, PROPERTY_VALUE_COLUMN);
    gtk_tree_view_append_column(GTK_TREE_VIEW(_keyValueTreeView), valCol);

	// Help column
	_helpColumn = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(_helpColumn, "?");
	gtk_tree_view_column_set_spacing(_helpColumn, 3);
	gtk_tree_view_column_set_visible(_helpColumn, FALSE);

	GdkPixbuf* helpIcon = GlobalUIManager().getLocalPixbuf(HELP_ICON_NAME);
	if (helpIcon != NULL) {
		gtk_tree_view_column_set_fixed_width(_helpColumn, gdk_pixbuf_get_width(helpIcon));
	}

	// Add the help icon
	GtkCellRenderer* pixRend = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(_helpColumn, pixRend, FALSE);
	gtk_tree_view_column_set_attributes(_helpColumn, pixRend,
										"pixbuf", HELP_ICON_COLUMN,
										NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(_keyValueTreeView), _helpColumn);

	// Connect the tooltip query signal to our custom routine
	g_object_set(G_OBJECT(_keyValueTreeView), "has-tooltip", TRUE, NULL);
	g_signal_connect(G_OBJECT(_keyValueTreeView), "query-tooltip", G_CALLBACK(_onQueryTooltip), this);

    // Set up the signals
    GtkTreeSelection* selection = gtk_tree_view_get_selection(
        GTK_TREE_VIEW(_keyValueTreeView)
    );
    g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(callbackTreeSelectionChanged), this);

    // Embed the TreeView in a scrolled viewport
    GtkWidget* scrollWin = gtkutil::ScrolledFrame(_keyValueTreeView);
    gtk_widget_set_size_request(
        _keyValueTreeView, TREEVIEW_MIN_WIDTH, TREEVIEW_MIN_HEIGHT
    );
    gtk_box_pack_start(GTK_BOX(vbx), scrollWin, TRUE, TRUE, 0);

	// Pack in the key and value edit boxes
	_keyEntry = gtk_entry_new();
	_valEntry = gtk_entry_new();

	GtkWidget* setButton = gtk_button_new();
	gtk_container_add(
		GTK_CONTAINER(setButton),
		gtk_image_new_from_stock(GTK_STOCK_APPLY, GTK_ICON_SIZE_MENU)
	);
	GtkWidget* setButtonBox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(setButtonBox), _valEntry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(setButtonBox), setButton, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbx), _keyEntry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), setButtonBox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbx), gtk_hseparator_new(), FALSE, FALSE, 0);

    // Signals for entry boxes
    g_signal_connect(
    	G_OBJECT(setButton), "clicked", G_CALLBACK(_onSetProperty), this);
    g_signal_connect(
    	G_OBJECT(_keyEntry), "activate", G_CALLBACK(_onEntryActivate), this);
    g_signal_connect(
    	G_OBJECT(_valEntry), "activate", G_CALLBACK(_onEntryActivate), this);

    return vbx;
}

// Retrieve the selected string from the given property in the list store

std::string EntityInspector::getListSelection(int col) 
{
	// Prepare to get the selection
    GtkTreeSelection* selection = gtk_tree_view_get_selection(
            GTK_TREE_VIEW(_keyValueTreeView)
    );
    GtkTreeIter tmpIter;

	// Return the selected string if available, else a blank string
    if (gtk_tree_selection_get_selected(selection, NULL, &tmpIter)) 
    {
        return gtkutil::TreeModel::getString(
            GTK_TREE_MODEL(_kvStore), &tmpIter, col
        );
    }
    else 
    {
    	return "";
    }
}

// Redraw the GUI elements
void EntityInspector::updateGUIElements()
{
    // Update from selection system
    getEntityFromSelectionSystem();

    if (_selectedEntity != NULL)
    {
        gtk_widget_set_sensitive(_editorFrame, TRUE);
        gtk_widget_set_sensitive(_keyValueTreeView, TRUE);
        gtk_widget_set_sensitive(_showInheritedCheckbox, TRUE);
        gtk_widget_set_sensitive(_showHelpColumnCheckbox, TRUE);
    }
    else  // no selected entity
    {
        // Remove the displayed PropertyEditor
        if (_currentPropertyEditor) {
            _currentPropertyEditor = PropertyEditorPtr();
        }

        // Disable the dialog and clear the TreeView
        gtk_widget_set_sensitive(_editorFrame, FALSE);
        gtk_widget_set_sensitive(_keyValueTreeView, FALSE);
        gtk_widget_set_sensitive(_showInheritedCheckbox, FALSE);
        gtk_widget_set_sensitive(_showHelpColumnCheckbox, FALSE);
    }
}

// Selection changed callback
void EntityInspector::selectionChanged(const scene::INodePtr& node, bool isComponent) 
{
	updateGUIElements();
}

namespace
{

    // SelectionSystem visitor to set a keyvalue on each entity, checking for
    // func_static-style name=model requirements
    class EntityKeySetter
    : public SelectionSystem::Visitor
    {
        // Key and value to set on all entities
        std::string _key;
        std::string _value;

    public:

        // Construct with key and value to set
        EntityKeySetter(const std::string& k, const std::string& v)
        : _key(k), _value(v)
        { }

        // Required visit function
        void visit(const scene::INodePtr& node) const
        {
            Entity* entity = Node_getEntity(node);
            if (entity)
            {
                // Check if we have a func_static-style entity
                std::string name = entity->getKeyValue("name");
                std::string model = entity->getKeyValue("model");
                bool isFuncType = (!name.empty() && name == model);

                // Set the actual value
                entity->setKeyValue(_key, _value);

                // Check for name key changes of func_statics
                if (isFuncType && _key == "name")
                {
                    // Adapt the model key along with the name
                    entity->setKeyValue("model", _value);
				}
            }
			else if (Node_isPrimitive(node)) {
				// We have a primitve node selected, check its parent
				scene::INodePtr parent(node->getParent());

				if (parent == NULL) return;

				Entity* parentEnt = Node_getEntity(parent);

				if (parentEnt != NULL) {
					// We have child primitive of an entity selected, the change
					// should go right into that parent entity
					parentEnt->setKeyValue(_key, _value);
				}
			}
        }
    };

}


std::string EntityInspector::cleanInputString(const std::string &input)
{
    std::string ret = input;

    boost::algorithm::replace_all( ret, "\n", "" );
    boost::algorithm::replace_all( ret, "\r", "" );
    return ret;
}


// Set entity property from entry boxes
void EntityInspector::setPropertyFromEntries()
{
	// Get the key from the entry box
    std::string key = cleanInputString(gtk_entry_get_text(GTK_ENTRY(_keyEntry)));
    std::string val = cleanInputString(gtk_entry_get_text(GTK_ENTRY(_valEntry)));

    // Update the entry boxes
    gtk_entry_set_text( GTK_ENTRY( _keyEntry ), key.c_str() );
    gtk_entry_set_text( GTK_ENTRY( _valEntry ), val.c_str() );

	// Pass the call to the specialised routine
	applyKeyValueToSelection(key, val);
}

void EntityInspector::applyKeyValueToSelection(const std::string& key, const std::string& val) {
	// greebo: Instantiate a scoped object to make this operation undoable
	UndoableCommand command("entitySetProperty");

	if (key.empty()) {
		return;
	}

	if (key == "name") {
		// Check the global namespace if this change is ok
		IMapRootNodePtr mapRoot = GlobalMapModule().getRoot();
		if (mapRoot != NULL) {
			INamespacePtr nspace = mapRoot->getNamespace();

			if (nspace != NULL && nspace->nameExists(val))
            {
				// name exists, cancel the change
				gtkutil::errorDialog("The name " + val + " already exists in this map!",
					GlobalMainFrame().getTopLevelWindow());
				return;
			}
		}
	}

	// Detect classname changes
    if (key == "classname") {
		// Classname changes are handled in a special way
		selection::algorithm::setEntityClassname(val);
	}
	else {
		// Regular key change, use EntityKeySetter to set value on all selected entities
		EntityKeySetter setter(key, val);
		GlobalSelectionSystem().foreachSelected(setter);
	}
}

void EntityInspector::loadPropertyMap()
{
	_propertyTypes.clear();

	xml::NodeList pNodes = GlobalRegistry().findXPath(PROPERTY_NODES_XPATH);

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

/* Popup menu callbacks (see gtkutil::PopupMenu) */

void EntityInspector::_onAddKey()
{
	// Obtain the entity class to provide to the AddPropertyDialog
	IEntityClassConstPtr ec = _selectedEntity->getEntityClass();

	// Choose a property, and add to entity with a default value
	AddPropertyDialog::PropertyList properties = AddPropertyDialog::chooseProperty(_selectedEntity);

	for (std::size_t i = 0; i < properties.size(); ++i)
	{
		const std::string& key = properties[i];

		// Add all keys, skipping existing ones to not overwrite any values on the entity
		if (_selectedEntity->getKeyValue(key) == "" || _selectedEntity->isInherited(key))
		{
			// Add the keyvalue on the entity (triggering the refresh)
			_selectedEntity->setKeyValue(key, "-");
		}
    }
}

void EntityInspector::_onDeleteKey() {
	std::string property = getListSelection(PROPERTY_NAME_COLUMN);
	if (!property.empty())
	{
		UndoableCommand cmd("deleteProperty");

		_selectedEntity->setKeyValue(property, "");
	}
}

bool EntityInspector::_testDeleteKey() 
{
	// Make sure the Delete item is only available for explicit
	// (non-inherited) properties
	if (getListSelection(INHERITED_FLAG_COLUMN) != "1")
		return true;
	else
		return false;
}

void EntityInspector::_onCopyKey() {
	std::string key = getListSelection(PROPERTY_NAME_COLUMN);
    std::string value = getListSelection(PROPERTY_VALUE_COLUMN);

	if (!key.empty()) {
		_clipBoard.key = key;
		_clipBoard.value = value;
	}
}

bool EntityInspector::_testCopyKey() {
	return !getListSelection(PROPERTY_NAME_COLUMN).empty();
}

void EntityInspector::_onCutKey() {
	std::string key = getListSelection(PROPERTY_NAME_COLUMN);
    std::string value = getListSelection(PROPERTY_VALUE_COLUMN);

	if (!key.empty() && _selectedEntity != NULL) {
		_clipBoard.key = key;
		_clipBoard.value = value;

		UndoableCommand cmd("cutProperty");

		// Clear the key after copying
		_selectedEntity->setKeyValue(key, "");
	}
}

bool EntityInspector::_testCutKey() {
	// Make sure the Delete item is only available for explicit
	// (non-inherited) properties
	if (getListSelection(INHERITED_FLAG_COLUMN) != "1") {
		// return true only if selection is not empty
		return !getListSelection(PROPERTY_NAME_COLUMN).empty();
	}
	else {
		return false;
	}
}

void EntityInspector::_onPasteKey() {
	if (!_clipBoard.key.empty() && !_clipBoard.value.empty()) {
		// Pass the call
		applyKeyValueToSelection(_clipBoard.key, _clipBoard.value);
	}
}

bool EntityInspector::_testPasteKey() {
	if (GlobalSelectionSystem().getSelectionInfo().entityCount == 0) {
		// No entities selected
		return false;
	}

	// Return true if the clipboard contains data
	return !_clipBoard.key.empty() && !_clipBoard.value.empty();
}

/* GTK CALLBACKS */

// Called when the TreeView selects a different property
void EntityInspector::callbackTreeSelectionChanged(GtkWidget* widget, EntityInspector* self) {
    self->treeSelectionChanged();
}

void EntityInspector::_onSetProperty(GtkWidget* button, EntityInspector* self) {
	self->setPropertyFromEntries();
}

// ENTER key in entry boxes
void EntityInspector::_onEntryActivate(GtkWidget* w, EntityInspector* self) {
	// Set property and move back to key entry
	self->setPropertyFromEntries();
	gtk_widget_grab_focus(self->_keyEntry);
}

void EntityInspector::_onToggleShowInherited(GtkToggleButton* b,
                                             EntityInspector* self) 
{
	if (gtk_toggle_button_get_active(b)) 
    {
		self->addClassProperties();
	}
	else 
    {
		self->removeClassProperties();
	}
}

void EntityInspector::_onToggleShowHelpIcons(GtkToggleButton* b, EntityInspector* self) {
	// Set the visibility of the column accordingly
	gtk_tree_view_column_set_visible(self->_helpColumn, gtk_toggle_button_get_active(b));
}

gboolean EntityInspector::_onQueryTooltip(GtkWidget* widget,
	 gint x, gint y, gboolean keyboard_mode,
	 GtkTooltip* tooltip, EntityInspector* self)
{
	if (self->_selectedEntity == NULL) return FALSE; // no single entity selected

	GtkTreeView* tv = GTK_TREE_VIEW(widget);
	bool showToolTip = false;

	// greebo: Important: convert the widget coordinates to bin coordinates first
	gint binX, binY;
	gtk_tree_view_convert_widget_to_bin_window_coords(tv, x, y, &binX, &binY);

	gint cellx, celly;
	GtkTreeViewColumn* column = NULL;
	GtkTreePath* path = NULL;

	if (gtk_tree_view_get_path_at_pos(tv, binX, binY, &path, &column, &cellx, &celly)) {
		// Get the iter of the row pointed at
		GtkTreeIter iter;
		GtkTreeModel* model = GTK_TREE_MODEL(self->_kvStore);
		if (gtk_tree_model_get_iter(model, &iter, path)) {
			// Get the key pointed at
			bool hasHelp = gtkutil::TreeModel::getBoolean(model, &iter, HAS_HELP_FLAG_COLUMN);

			if (hasHelp) {
				std::string key = gtkutil::TreeModel::getString(model, &iter, PROPERTY_NAME_COLUMN);

				IEntityClassConstPtr eclass = self->_selectedEntity->getEntityClass();
				assert(eclass != NULL);

				// Find the attribute on the eclass, that's where the descriptions are defined
				const EntityClassAttribute& attr = eclass->getAttribute(key);

				if (!attr.description.empty()) {
					// Check the description of the focused item
					gtk_tree_view_set_tooltip_row(tv, tooltip, path);
					gtk_tooltip_set_markup(tooltip, attr.description.c_str());
					showToolTip = true;
				}
			}
		}
	}

	if (path != NULL) {
		gtk_tree_path_free(path);
	}

	return showToolTip ? TRUE : FALSE;
}

/* END GTK CALLBACKS */

// Update the PropertyEditor pane, displaying the PropertyEditor if necessary
// and making sure it refers to the currently-selected Entity.
void EntityInspector::treeSelectionChanged() {

	// Abort if called without a valid entity selection (may happen during
	// various cleanup operations).
	if (_selectedEntity == NULL)
		return;

    // Get the selected key and value in the tree view
    std::string key = getListSelection(PROPERTY_NAME_COLUMN);
    std::string value = getListSelection(PROPERTY_VALUE_COLUMN);

    // Get the type for this key if it exists, and the options
	PropertyParms parms = getPropertyParmsForKey(key);

    // If the type was not found, also try looking on the entity class
    if (parms.type.empty())
	{
    	IEntityClassConstPtr eclass = _selectedEntity->getEntityClass();
		parms.type = eclass->getAttribute(key).type;
    }

	// Remove the existing PropertyEditor widget, if there is one
	GtkWidget* existingWidget = gtk_bin_get_child(GTK_BIN(_editorFrame));
   	if (existingWidget != NULL)
	{
   		gtk_widget_destroy(existingWidget);
	}

    // Construct and add a new PropertyEditor
    _currentPropertyEditor = PropertyEditorFactory::create(parms.type,
                                                           _selectedEntity,
                                                           key,
                                                           parms.options);

	// If the creation was successful (because the PropertyEditor type exists),
	// add its widget to the editor pane
    if (_currentPropertyEditor) {
        gtk_container_add(GTK_CONTAINER(_editorFrame),
        				  _currentPropertyEditor->getWidget());
    }

    // Update key and value entry boxes, but only if there is a key value. If
    // there is no selection we do not clear the boxes, to allow keyval copying
    // between entities.
	if (!key.empty()) {
		gtk_entry_set_text(GTK_ENTRY(_keyEntry), key.c_str());
		gtk_entry_set_text(GTK_ENTRY(_valEntry), value.c_str());
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
		boost::regex expr(i->first);
		boost::smatch matches;
		
		if (!boost::regex_match(key, matches, expr)) continue;
		
		// We have a match
		returnValue.type = i->second.type;
		returnValue.options = i->second.options;
	}

	return returnValue;
}

// Append inherited (entityclass) properties
void EntityInspector::addClassProperties() 
{
	// Get the entityclass for the current entity
	std::string className = _selectedEntity->getKeyValue("classname");
	IEntityClassPtr eclass = GlobalEntityClassManager().findOrInsert(
        className, true
    );

	// Use a functor to walk the entityclass and add all of its attributes
	// to the tree
	class ClassPropertyVisitor
	: public EntityClassAttributeVisitor
	{
		// List store to populate
		GtkListStore* _store;

    public:

		// Constructor
		ClassPropertyVisitor(GtkListStore* store)
		: _store(store) {}

		// Required visitor function
		void visit(const EntityClassAttribute& a) 
        {
			// Only add properties with values, we don't want the optional
			// "editor_var xxx" properties here.
			if (!a.value.empty()) 
            {
				bool hasDescription = !a.description.empty();

				GtkTreeIter iter;
				gtk_list_store_append(_store, &iter);
				gtk_list_store_set(_store, &iter,
					PROPERTY_NAME_COLUMN, a.name.c_str(),
					PROPERTY_VALUE_COLUMN, a.value.c_str(),
					TEXT_COLOUR_COLUMN, "#707070",
					PROPERTY_ICON_COLUMN, NULL,
					INHERITED_FLAG_COLUMN, "1", // inherited
					HELP_ICON_COLUMN, hasDescription 
                                      ? GlobalUIManager().getLocalPixbuf(HELP_ICON_NAME)
                                      : NULL,
					HAS_HELP_FLAG_COLUMN, hasDescription ? TRUE : FALSE,
					-1);
			}
		}
	};

	// Visit the entity class
	ClassPropertyVisitor visitor(_kvStore);
	eclass->forEachClassAttribute(visitor);
}

// Remove the inherited properties
void EntityInspector::removeClassProperties()
{
    // Iterate over all rows in the list store, removing inherited keys
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(
            GTK_TREE_MODEL(_kvStore), &iter
    );
    while (valid)
    {
        // If this is an inherited row, remove it, otherwise move to the next
        // row
        std::string inherited = gtkutil::TreeModel::getString(
            GTK_TREE_MODEL(_kvStore), &iter, INHERITED_FLAG_COLUMN
        );
        if (inherited == "1")
        {
            valid = gtk_list_store_remove(_kvStore, &iter);
        }
        else
        {
            valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(_kvStore), &iter);
        }
    }
}

// Update the selected Entity pointer from the selection system
void EntityInspector::getEntityFromSelectionSystem() 
{
	// A single entity must be selected
	if (GlobalSelectionSystem().countSelected() != 1) 
    {
        changeSelectedEntity(NULL);
		return;
	}

	scene::INodePtr selectedNode = GlobalSelectionSystem().ultimateSelected();

    // The root node must not be selected (this can happen if Invert Selection is
    // activated with an empty scene, or by direct selection in the entity list).
	if (selectedNode->isRoot()) 
    {
        changeSelectedEntity(NULL);
		return;
	}

    // Try both the selected node (if an entity is selected) or the parent node
    // (if a brush is selected).
    Entity* newSelectedEntity = Node_getEntity(selectedNode);
    if (newSelectedEntity)
    {
        // Node was an entity, use this
        changeSelectedEntity(newSelectedEntity);
    }
    else
    {
        // Node was not an entity, try parent instead
        scene::INodePtr selectedNodeParent = selectedNode->getParent();
        changeSelectedEntity(Node_getEntity(selectedNodeParent));
    }
}

// Change selected entity pointer
void EntityInspector::changeSelectedEntity(Entity* newEntity)
{
    if (newEntity == _selectedEntity)
    {
        // No change, do nothing
        return;
    }
    else
    {
        // Detach only if we already have a selected entity
        if (_selectedEntity)
        {
            _selectedEntity->detachObserver(this);
            removeClassProperties();
        }

        _selectedEntity = newEntity;

        // Attach to new entity if it is non-NULL
        if (_selectedEntity)
        {
            _selectedEntity->attachObserver(this);

            // Add inherited properties if the checkbox is set
            gboolean showInherited = gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(_showInheritedCheckbox)
            );

            if (showInherited)
            {
                addClassProperties();
            }
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
