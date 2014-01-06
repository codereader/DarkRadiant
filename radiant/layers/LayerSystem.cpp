#include "LayerSystem.h"

#include "iorthocontextmenu.h"
#include "i18n.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "itextstream.h"
#include "imainframe.h"
#include "icommandsystem.h"
#include "scene/Node.h"
#include "scenelib.h"
#include "modulesystem/StaticModule.h"

#include "AddToLayerWalker.h"
#include "MoveToLayerWalker.h"
#include "RemoveFromLayerWalker.h"
#include "SetLayerSelectedWalker.h"

#include "gtkutil/dialog/MessageBox.h"
#include "gtkutil/IconTextMenuItem.h"
#include "gtkutil/EntryAbortedException.h"
#include "gtkutil/menu/CommandMenuItem.h"

#include "ui/layers/LayerControlDialog.h"
#include "ui/layers/LayerOrthoContextMenuItem.h"

#include <boost/bind.hpp>

namespace scene
{

namespace
{
	const char* const DEFAULT_LAYER_NAME = N_("Default");

	const char* const LAYER_ICON = "layers.png";
	const char* const CREATE_LAYER_TEXT = N_("Create Layer...");

	const char* const ADD_TO_LAYER_TEXT = N_("Add to Layer...");
	const char* const MOVE_TO_LAYER_TEXT = N_("Move to Layer...");
	const char* const REMOVE_FROM_LAYER_TEXT = N_("Remove from Layer...");

	const int DEFAULT_LAYER = 0;
}

LayerSystem::LayerSystem() :
	_activeLayer(DEFAULT_LAYER)
{}

int LayerSystem::createLayer(const std::string& name, int layerID)
{
	// Check if the ID already exists
	if (_layers.find(layerID) != _layers.end())
	{
		// already exists => quit
		return -1;
	}

	// Insert the new layer
	std::pair<LayerMap::iterator, bool> result = _layers.insert(
		LayerMap::value_type(layerID, name)
	);

	if (result.second == false) {
		rError() << "LayerSystem: Could not create layer!" << std::endl;
		return -1;
	}

	// Update the visibility cache, so get the highest ID
	int highestID = getHighestLayerID();

	// Make sure the vector has allocated enough memory
	_layerVisibility.resize(highestID+1);

	// Set the newly created layer to "visible"
	_layerVisibility[result.first->first] = true;

	// Return the ID of the inserted layer
	return result.first->first;
}

int LayerSystem::createLayer(const std::string& name)
{
	// Check if the layer already exists
	int existingID = getLayerID(name);

	if (existingID != -1) {
		rError() << "Could not create layer, name already exists: "
			<< name << std::endl;
		return -1;
	}

	// Layer doesn't exist yet, get the lowest free Id
	int newID = getLowestUnusedLayerID();

	// pass the call to the overload and return
	return createLayer(name, newID);
}

void LayerSystem::deleteLayer(const std::string& name)
{
	// Check if the layer already exists
	int layerID = getLayerID(name);

	if (layerID == -1)
	{
		rError() << "Could not delete layer, name doesn't exist: "
			<< name << std::endl;
		return;
	}

	// Remove all nodes from this layer first, but don't de-select them yet
	RemoveFromLayerWalker walker(layerID);
	GlobalSceneGraph().root()->traverse(walker);

	// Remove the layer
	_layers.erase(layerID);

	// Reset the visibility flag to TRUE
	_layerVisibility[layerID] = true;

	if (layerID == _activeLayer)
	{
		// We have removed the active layer, fall back to default
		_activeLayer = DEFAULT_LAYER;
	}

	// Fire the visibility changed event to
	// update the scenegraph and redraw the views
	onLayerVisibilityChanged();
}

void LayerSystem::foreachLayer(Visitor& visitor)
{
	for (LayerMap::iterator i = _layers.begin(); i != _layers.end(); ++i)
	{
		visitor.visit(i->first, i->second);
	}
}

void LayerSystem::reset()
{
	_activeLayer = DEFAULT_LAYER;

	_layers.clear();
	_layers.insert(LayerMap::value_type(DEFAULT_LAYER, _(DEFAULT_LAYER_NAME)));

	_layerVisibility.resize(1);
	_layerVisibility[DEFAULT_LAYER] = true;

	// Update the LayerControlDialog
	ui::LayerControlDialog::Instance().refresh();
}

bool LayerSystem::renameLayer(int layerID, const std::string& newLayerName)
{
	// Check sanity
	if (newLayerName.empty() || newLayerName == _(DEFAULT_LAYER_NAME)) {
		return false; // empty name or default name used
	}

	LayerMap::iterator i = _layers.find(layerID);

	if (i == _layers.end()) {
		return false; // not found
	}

	// Rename that layer
	i->second = newLayerName;

	return true;
}

int LayerSystem::getFirstVisibleLayer() const
{
	// Iterate over all IDs and check the visibility status, return the first visible
	for (LayerMap::const_iterator i = _layers.begin(); i != _layers.end(); ++i)
	{
		if (_layerVisibility[i->first])
		{
			return i->first;
		}
	}

	// No layer visible, return DEFAULT_LAYER to prevent callers from doing unreasonable things.
	return DEFAULT_LAYER;
}

int LayerSystem::getActiveLayer() const
{
	return _activeLayer;
}

void LayerSystem::setActiveLayer(int layerID)
{
	LayerMap::iterator i = _layers.find(layerID);

	if (i == _layers.end())
	{
		return; // do nothing
	}

	// ID is valid, assign active layer
	_activeLayer = layerID;
}

bool LayerSystem::layerIsVisible(const std::string& layerName) 
{
	// Check if the layer already exists
	int layerID = getLayerID(layerName);

	if (layerID == -1) {
		rError() << "Could not query layer visibility, name doesn't exist: "
			<< layerName << std::endl;
		return false;
	}

	return _layerVisibility[layerID];
}

bool LayerSystem::layerIsVisible(int layerID) {
	// Sanity check
	if (layerID < 0 || layerID >= static_cast<int>(_layerVisibility.size())) {
		rMessage() << "LayerSystem: Querying invalid layer ID: " << layerID << std::endl;
		return false;
	}

	return _layerVisibility[layerID];
}

void LayerSystem::setLayerVisibility(int layerID, bool visible)
{
	// Sanity check
	if (layerID < 0 || layerID >= static_cast<int>(_layerVisibility.size()))
	{
		rMessage() <<
			"LayerSystem: Setting visibility of invalid layer ID: " <<
			layerID << std::endl;
		return;
	}

	// Set the visibility
	_layerVisibility[layerID] = visible;

	if (!visible && layerID == _activeLayer)
	{
		// We just hid the active layer, fall back to another one
		_activeLayer = getFirstVisibleLayer();
	}

	// Fire the visibility changed event
	onLayerVisibilityChanged();
}

void LayerSystem::setLayerVisibility(const std::string& layerName, bool visible) {
	// Check if the layer already exists
	int layerID = getLayerID(layerName);

	if (layerID == -1) {
		rError() << "Could not set layer visibility, name doesn't exist: "
			<< layerName.c_str() << std::endl;
		return;
	}

	// Pass the call to the overloaded method to do the work
	setLayerVisibility(layerID, visible);
}

void LayerSystem::updateSceneGraphVisibility() {
	UpdateNodeVisibilityWalker walker;
	GlobalSceneGraph().root()->traverseChildren(walker);
}

void LayerSystem::onLayerVisibilityChanged() {
	// Update all nodes
	updateSceneGraphVisibility();

	// Redraw
	SceneChangeNotify();

	// Update the LayerControlDialog
	ui::LayerControlDialog::Instance().update();
}

void LayerSystem::addSelectionToLayer(int layerID) {
	// Check if the layer ID exists
	if (_layers.find(layerID) == _layers.end()) {
		return;
	}

	// Instantiate a Selectionwalker and traverse the selection
	AddToLayerWalker walker(layerID);
	GlobalSelectionSystem().foreachSelected(walker);

	updateSceneGraphVisibility();
}

void LayerSystem::addSelectionToLayer(const std::string& layerName) {
	// Check if the layer already exists
	int layerID = getLayerID(layerName);

	if (layerID == -1) {
		rError() << "Cannot add to layer, name doesn't exist: "
			<< layerName << std::endl;
		return;
	}

	// Pass the call to the overload
	addSelectionToLayer(layerID);
}

void LayerSystem::moveSelectionToLayer(const std::string& layerName) {
	// Check if the layer already exists
	int layerID = getLayerID(layerName);

	if (layerID == -1) {
		rError() << "Cannot move to layer, name doesn't exist: "
			<< layerName << std::endl;
		return;
	}

	// Pass the call to the overload
	moveSelectionToLayer(layerID);
}

void LayerSystem::moveSelectionToLayer(int layerID) {
	// Check if the layer ID exists
	if (_layers.find(layerID) == _layers.end()) {
		return;
	}

	// Instantiate a Selectionwalker and traverse the selection
	MoveToLayerWalker walker(layerID);
	GlobalSelectionSystem().foreachSelected(walker);

	updateSceneGraphVisibility();
}

void LayerSystem::removeSelectionFromLayer(const std::string& layerName) {
	// Check if the layer already exists
	int layerID = getLayerID(layerName);

	if (layerID == -1) {
		rError() << "Cannot remove from layer, name doesn't exist: "
			<< layerName << std::endl;
		return;
	}

	// Pass the call to the overload
	removeSelectionFromLayer(layerID);
}

void LayerSystem::removeSelectionFromLayer(int layerID) {
	// Check if the layer ID exists
	if (_layers.find(layerID) == _layers.end()) {
		return;
	}

	// Instantiate a Selectionwalker and traverse the selection
	RemoveFromLayerWalker walker(layerID);
	GlobalSelectionSystem().foreachSelected(walker);

	updateSceneGraphVisibility();
}

bool LayerSystem::updateNodeVisibility(const scene::INodePtr& node) {
	// Get the list of layers the node is associated with
	// greebo: TODO: Check if returning the LayerList by value is taxing.
	LayerList layers = node->getLayers();

	// We start with the assumption that a node is hidden
	node->enable(Node::eLayered);

	// Cycle through the Node's layers, and show the node as soon as
	// a visible layer is found.
	for (LayerList::const_iterator i = layers.begin(); i != layers.end(); i++) {
		if (_layerVisibility[*i]) {
			// The layer is visible, set the visibility to true and quit
			node->disable(Node::eLayered);
			return true;
		}
	}

	// Node is hidden, return FALSE
	return false;
}

void LayerSystem::setSelected(int layerID, bool selected) {
	SetLayerSelectedWalker walker(layerID, selected);
	GlobalSceneGraph().root()->traverseChildren(walker);
}

int LayerSystem::getLayerID(const std::string& name) const {
	for (LayerMap::const_iterator i = _layers.begin(); i != _layers.end(); i++) {
		if (i->second == name) {
			// Name found, return the ID
			return i->first;
		}
	}

	return -1;
}

std::string LayerSystem::getLayerName(int layerID) const {
	LayerMap::const_iterator found = _layers.find(layerID);

	if (found != _layers.end()) {
		return found->second;
	}

	// not found
	return "";
}

bool LayerSystem::layerExists(int layerID) const
{
	return _layers.find(layerID) != _layers.end();
}

int LayerSystem::getHighestLayerID() const {
	if (_layers.size() == 0) {
		// Empty layer map, just return DEFAULT_LAYER
		return DEFAULT_LAYER;
	}

	// A map is sorted, so return the ID of the element from the end of the map
	return _layers.rbegin()->first;
}

int LayerSystem::getLowestUnusedLayerID() {
	for (int i = 0; i < INT_MAX; i++) {
		if (_layers.find(i) == _layers.end()) {
			// Found a free ID
			return i;
		}
	}

	return -1;
}

// RegisterableModule implementation
const std::string& LayerSystem::getName() const {
	static std::string _name(MODULE_LAYERSYSTEM);
	return _name;
}

const StringSet& LayerSystem::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_UIMANAGER);
		_dependencies.insert(MODULE_ORTHOCONTEXTMENU);
	}

	return _dependencies;
}

void LayerSystem::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << "LayerSystem::initialiseModule called.\n";

	// Create the "master" layer with ID DEFAULT_LAYER
	createLayer(_(DEFAULT_LAYER_NAME));

	// Add command targets for the first 10 layer IDs here
	for (int i = 0; i < 10; i++) {
		_commandTargets.push_back(
			LayerCommandTargetPtr(new LayerCommandTarget(i))
		);
	}

	// Register the "create layer" command
	GlobalCommandSystem().addCommand("CreateNewLayer",
		boost::bind(&LayerSystem::createLayerCmd, this, _1), cmd::ARGTYPE_STRING|cmd::ARGTYPE_OPTIONAL);
	IEventPtr ev = GlobalEventManager().addCommand("CreateNewLayer", "CreateNewLayer");

	GlobalCommandSystem().addCommand("ToggleLayerControlDialog", ui::LayerControlDialog::toggle);
	GlobalEventManager().addCommand("ToggleLayerControlDialog", "ToggleLayerControlDialog");


	// Create a new menu item connected to the CreateNewLayer command
	gtkutil::CommandMenuItemPtr menuItem(new gtkutil::CommandMenuItem(
		Gtk::manage(new gtkutil::IconTextMenuItem(GlobalUIManager().getLocalPixbuf(LAYER_ICON), _(CREATE_LAYER_TEXT))),
		"CreateNewLayer")
	);

	GlobalOrthoContextMenu().addItem(menuItem, ui::IOrthoContextMenu::SECTION_LAYER);

	// Add the ortho context menu items
	ui::LayerOrthoContextMenuItemPtr addMenu(new ui::LayerOrthoContextMenuItem(
		_(ADD_TO_LAYER_TEXT), ui::LayerOrthoContextMenuItem::AddToLayer));

	ui::LayerOrthoContextMenuItemPtr moveMenu(new ui::LayerOrthoContextMenuItem(
		_(MOVE_TO_LAYER_TEXT), ui::LayerOrthoContextMenuItem::MoveToLayer));

	ui::LayerOrthoContextMenuItemPtr removeMenu(new ui::LayerOrthoContextMenuItem(
		_(REMOVE_FROM_LAYER_TEXT), ui::LayerOrthoContextMenuItem::RemoveFromLayer));

	GlobalOrthoContextMenu().addItem(addMenu, ui::IOrthoContextMenu::SECTION_LAYER);
	GlobalOrthoContextMenu().addItem(moveMenu, ui::IOrthoContextMenu::SECTION_LAYER);
	GlobalOrthoContextMenu().addItem(removeMenu, ui::IOrthoContextMenu::SECTION_LAYER);
}

void LayerSystem::createLayerCmd(const cmd::ArgumentList& args)
{
	std::string initialName = !args.empty() ? args[0].getString() : "";

	while (true)
	{
		// Query the name of the new layer from the user
		std::string layerName;

		if (!initialName.empty()) {
			// If we got a layer name passed through the arguments,
			// we use this one, but only the first time
			layerName = initialName;
			initialName.clear();
		}

		if (layerName.empty()) {
			try {
				layerName = gtkutil::Dialog::TextEntryDialog(
					_("Enter Name"),
					_("Enter Layer Name"),
					"",
					GlobalMainFrame().getTopLevelWindow()
				);
			}
			catch (gtkutil::EntryAbortedException&) {
				break;
			}
		}

		if (layerName.empty()) {
			// Wrong name, let the user try again
			gtkutil::MessageBox::ShowError(_("Cannot create layer with empty name."), GlobalMainFrame().getTopLevelWindow());
			continue;
		}

		// Attempt to create the layer, this will return -1 if the operation fails
		int layerID = createLayer(layerName);

		if (layerID != -1) {
			// Success, break the loop
			ui::LayerControlDialog::Instance().refresh();
			break;
		}
		else {
			// Wrong name, let the user try again
			gtkutil::MessageBox::ShowError(_("This name already exists."), GlobalMainFrame().getTopLevelWindow());
			continue;
		}
	}
}

// Define the static LayerSystem module
module::StaticModule<LayerSystem> layerSystemModule;

// Internal accessor method
LayerSystem& getLayerSystem() {
	return *layerSystemModule.getModule();
}

} // namespace scene
