#include "GlobalXYWnd.h"
#include "FloatingOrthoView.h"

#include "i18n.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "ipreferencesystem.h"

#include "registry/registry.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/FramedWidget.h"

#include "modulesystem/StaticModule.h"
#include "selection/algorithm/General.h"
#include "camera/GlobalCamera.h"
#include <boost/bind.hpp>

namespace
{
	const std::string RKEY_XYVIEW_ROOT = "user/ui/xyview";

	const std::string RKEY_CHASE_MOUSE = RKEY_XYVIEW_ROOT + "/chaseMouse";
	const std::string RKEY_CAMERA_XY_UPDATE = RKEY_XYVIEW_ROOT + "/camXYUpdate";
	const std::string RKEY_SHOW_CROSSHAIRS = RKEY_XYVIEW_ROOT + "/showCrossHairs";
	const std::string RKEY_SHOW_GRID = RKEY_XYVIEW_ROOT + "/showGrid";
	const std::string RKEY_SHOW_SIZE_INFO = RKEY_XYVIEW_ROOT + "/showSizeInfo";
	const std::string RKEY_SHOW_ENTITY_ANGLES = RKEY_XYVIEW_ROOT + "/showEntityAngles";
	const std::string RKEY_SHOW_ENTITY_NAMES = RKEY_XYVIEW_ROOT + "/showEntityNames";
	const std::string RKEY_SHOW_BLOCKS = RKEY_XYVIEW_ROOT + "/showBlocks";
	const std::string RKEY_SHOW_COORDINATES = RKEY_XYVIEW_ROOT + "/showCoordinates";
	const std::string RKEY_SHOW_OUTLINE = RKEY_XYVIEW_ROOT + "/showOutline";
	const std::string RKEY_SHOW_AXES = RKEY_XYVIEW_ROOT + "/showAxes";
	const std::string RKEY_SHOW_WORKZONE = RKEY_XYVIEW_ROOT + "/showWorkzone";
	const std::string RKEY_DEFAULT_BLOCKSIZE = "user/ui/xyview/defaultBlockSize";
	const std::string RKEY_TRANSLATE_CONSTRAINED = "user/ui/xyview/translateConstrained";
	const std::string RKEY_HIGHER_ENTITY_PRIORITY = "user/ui/xyview/higherEntitySelectionPriority";
}

// Constructor
XYWndManager::XYWndManager()
{}

/* greebo: This method restores all xy views from the information stored in the registry.
 *
 * Note: The window creation code looks very unelegant (in fact it is), but this is required
 * to restore the exact position of the windows (at least on my WinXP/GTK2+ system).
 *
 * The position of the TransientWindow has to be set IMMEDIATELY after creation, before
 * any widgets are added to this container. When trying to apply the position restore
 * on the fully "fabricated" xyview widget, the position tends to be some 20 pixels below
 * the original position. I have no full explanation for this and it is nasty, but the code
 * below seems to work.
 */
void XYWndManager::restoreState()
{
	xml::NodeList views = GlobalRegistry().findXPath(RKEY_XYVIEW_ROOT + "//views");

	if (!views.empty())
	{
		// Find all <view> tags under the first found <views> tag
		xml::NodeList viewList = views[0].getNamedChildren("view");

		for (xml::NodeList::const_iterator i = viewList.begin();
			 i != viewList.end();
			 ++i)
		{
			// Assemble the XPath for the viewstate
			std::string path = RKEY_XYVIEW_ROOT +
				"/views/view[@name='" + i->getAttributeValue("name") + "']";

			// Create the view and restore the size
			XYWndPtr newWnd = createFloatingOrthoView(XY);
			newWnd->readStateFromPath(path);

			const std::string typeStr = i->getAttributeValue("type");

			if (typeStr == "YZ") {
				newWnd->setViewType(YZ);
			}
			else if (typeStr == "XZ") {
				newWnd->setViewType(XZ);
			}
			else {
				newWnd->setViewType(XY);
			}
		}
	}
	else {
		// Create at least one XYView, if no view info is found
		rMessage() << "XYWndManager: No xywindow information found in XMLRegistry, creating default view.\n";

		// Create a default OrthoView
		createFloatingOrthoView(XY);
	}
}

void XYWndManager::saveState()
{
	// Delete all the current window states from the registry
	GlobalRegistry().deleteXPath(RKEY_XYVIEW_ROOT + "//views");

	// Create a new node
	std::string rootNodePath(RKEY_XYVIEW_ROOT + "/views");

	for (XYWndMap::iterator i = _xyWnds.begin(); i != _xyWnds.end(); ++i)
	{
		// Save each XYView state to the registry
		i->second->saveStateToPath(rootNodePath);
	}
}

// Free the allocated XYViews from the heap
void XYWndManager::destroyViews()
{
	// Discard the whole list
	for (XYWndMap::iterator i = _xyWnds.begin(); i != _xyWnds.end(); /* in-loop incr.*/)
	{
		// Extract the pointer to prevent the destructor from firing
		XYWndPtr candidate = i->second;

		// Now remove the item from the map, increase the iterator
		_xyWnds.erase(i++);

		// greebo: Release the shared_ptr, this fires the destructor chain
		// which eventually reaches notifyXYWndDestroy(). This is safe at this
		// point, because the id is not found in the map anymore, thus
		// double-deletions are prevented.
		candidate = XYWndPtr();
	}

	_activeXY = XYWndPtr();
}

void XYWndManager::registerCommands() {
	GlobalCommandSystem().addCommand("NewOrthoView", boost::bind(&XYWndManager::createXYFloatingOrthoView, this, _1));
	GlobalCommandSystem().addCommand("NextView", boost::bind(&XYWndManager::toggleActiveView, this, _1));
	GlobalCommandSystem().addCommand("ZoomIn", boost::bind(&XYWndManager::zoomIn, this, _1));
	GlobalCommandSystem().addCommand("ZoomOut", boost::bind(&XYWndManager::zoomOut, this, _1));
	GlobalCommandSystem().addCommand("ViewTop", boost::bind(&XYWndManager::setActiveViewXY, this, _1));
	GlobalCommandSystem().addCommand("ViewSide", boost::bind(&XYWndManager::setActiveViewXZ, this, _1));
	GlobalCommandSystem().addCommand("ViewFront", boost::bind(&XYWndManager::setActiveViewYZ, this, _1));
	GlobalCommandSystem().addCommand("CenterXYViews", boost::bind(&XYWndManager::splitViewFocus, this, _1));
	GlobalCommandSystem().addCommand("CenterXYView", boost::bind(&XYWndManager::focusActiveView, this, _1));
	GlobalCommandSystem().addCommand("Zoom100", boost::bind(&XYWndManager::zoom100, this, _1));

	GlobalEventManager().addCommand("NewOrthoView", "NewOrthoView");
	GlobalEventManager().addCommand("NextView", "NextView");
	GlobalEventManager().addCommand("ZoomIn", "ZoomIn");
	GlobalEventManager().addCommand("ZoomOut", "ZoomOut");
	GlobalEventManager().addCommand("ViewTop", "ViewTop");
	GlobalEventManager().addCommand("ViewSide", "ViewSide");
	GlobalEventManager().addCommand("ViewFront", "ViewFront");
	GlobalEventManager().addCommand("CenterXYViews", "CenterXYViews");
	GlobalEventManager().addCommand("CenterXYView", "CenterXYView");
	GlobalEventManager().addCommand("Zoom100", "Zoom100");

	GlobalEventManager().addRegistryToggle("ToggleCrosshairs", RKEY_SHOW_CROSSHAIRS);
	GlobalEventManager().addRegistryToggle("ToggleGrid", RKEY_SHOW_GRID);
	GlobalEventManager().addRegistryToggle("ShowAngles", RKEY_SHOW_ENTITY_ANGLES);
	GlobalEventManager().addRegistryToggle("ShowNames", RKEY_SHOW_ENTITY_NAMES);
	GlobalEventManager().addRegistryToggle("ShowBlocks", RKEY_SHOW_BLOCKS);
	GlobalEventManager().addRegistryToggle("ShowCoordinates", RKEY_SHOW_COORDINATES);
	GlobalEventManager().addRegistryToggle("ShowWindowOutline", RKEY_SHOW_OUTLINE);
	GlobalEventManager().addRegistryToggle("ShowAxes", RKEY_SHOW_AXES);
	GlobalEventManager().addRegistryToggle("ShowWorkzone", RKEY_SHOW_WORKZONE);
	GlobalEventManager().addRegistryToggle("ToggleShowSizeInfo", RKEY_SHOW_SIZE_INFO);
}

void XYWndManager::constructPreferences()
{
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage(_("Settings/Orthoview"));

	page->appendCheckBox("", _("View chases Mouse Cursor during Drags"), RKEY_CHASE_MOUSE);
	page->appendCheckBox("", _("Update Views on Camera Movement"), RKEY_CAMERA_XY_UPDATE);
	page->appendCheckBox("", _("Show Crosshairs"), RKEY_SHOW_CROSSHAIRS);
	page->appendCheckBox("", _("Show Grid"), RKEY_SHOW_GRID);
	page->appendCheckBox("", _("Show Size Info"), RKEY_SHOW_SIZE_INFO);
	page->appendCheckBox("", _("Show Entity Angle Arrow"), RKEY_SHOW_ENTITY_ANGLES);
	page->appendCheckBox("", _("Show Entity Names"), RKEY_SHOW_ENTITY_NAMES);
	page->appendCheckBox("", _("Show Blocks"), RKEY_SHOW_BLOCKS);
	page->appendCheckBox("", _("Show Coordinates"), RKEY_SHOW_COORDINATES);
	page->appendCheckBox("", _("Show Axes"), RKEY_SHOW_AXES);
	page->appendCheckBox("", _("Show Window Outline"), RKEY_SHOW_OUTLINE);
	page->appendCheckBox("", _("Show Workzone"), RKEY_SHOW_WORKZONE);
	page->appendCheckBox("", _("Translate Manipulator always constrained to Axis"), RKEY_TRANSLATE_CONSTRAINED);
	page->appendCheckBox("", _("Higher Selection Priority for Entities"), RKEY_HIGHER_ENTITY_PRIORITY);
}

// Load/Reload the values from the registry
void XYWndManager::refreshFromRegistry()
{
	_chaseMouse = registry::getValue<bool>(RKEY_CHASE_MOUSE);
	_camXYUpdate = registry::getValue<bool>(RKEY_CAMERA_XY_UPDATE);
	_showCrossHairs = registry::getValue<bool>(RKEY_SHOW_CROSSHAIRS);
	_showGrid = registry::getValue<bool>(RKEY_SHOW_GRID);
	_showSizeInfo = registry::getValue<bool>(RKEY_SHOW_SIZE_INFO);
	_showBlocks = registry::getValue<bool>(RKEY_SHOW_BLOCKS);
	_showCoordinates = registry::getValue<bool>(RKEY_SHOW_COORDINATES);
	_showOutline = registry::getValue<bool>(RKEY_SHOW_OUTLINE);
	_showAxes = registry::getValue<bool>(RKEY_SHOW_AXES);
	_showWorkzone = registry::getValue<bool>(RKEY_SHOW_WORKZONE);
	_defaultBlockSize = registry::getValue<int>(RKEY_DEFAULT_BLOCKSIZE);
	updateAllViews();
}

bool XYWndManager::higherEntitySelectionPriority() const 
{
	return registry::getValue<bool>(RKEY_HIGHER_ENTITY_PRIORITY);
}

bool XYWndManager::chaseMouse() const {
	return _chaseMouse;
}

bool XYWndManager::camXYUpdate() const {
	return _camXYUpdate;
}

bool XYWndManager::showCrossHairs() const {
	return _showCrossHairs;
}

bool XYWndManager::showBlocks() const {
	return _showBlocks;
}

unsigned int XYWndManager::defaultBlockSize() const {
	return _defaultBlockSize;
}

bool XYWndManager::showCoordinates() const {
	return _showCoordinates;
}

bool XYWndManager::showOutline() const  {
	return _showOutline;
}

bool XYWndManager::showAxes() const {
	return _showAxes;
}

bool XYWndManager::showWorkzone() const {
	return _showWorkzone;
}

bool XYWndManager::showGrid() const {
	return _showGrid;
}

bool XYWndManager::showSizeInfo() const {
	return _showSizeInfo;
}

void XYWndManager::updateAllViews() {
	for (XYWndMap::iterator i = _xyWnds.begin();
		 i != _xyWnds.end();
		 ++i)
	{
		i->second->queueDraw();
	}
}

void XYWndManager::zoomIn(const cmd::ArgumentList& args) {
	if (_activeXY != NULL) {
		_activeXY->zoomIn();
	}
}

void XYWndManager::zoomOut(const cmd::ArgumentList& args) {
	if (_activeXY != NULL) {
		_activeXY->zoomOut();
	}
}

XYWndPtr XYWndManager::getActiveXY() const {
	return _activeXY;
}

void XYWndManager::setOrigin(const Vector3& origin) {
	// Cycle through the list of views and set the origin
	for (XYWndMap::iterator i = _xyWnds.begin();
		 i != _xyWnds.end();
		 ++i)
	{
		i->second->setOrigin(origin);
	}
}

void XYWndManager::setScale(float scale) {
	for (XYWndMap::iterator i = _xyWnds.begin();
		 i != _xyWnds.end();
		 ++i)
	{
		i->second->setScale(scale);
	}
}

void XYWndManager::positionAllViews(const Vector3& origin) {
	for (XYWndMap::iterator i = _xyWnds.begin();
		 i != _xyWnds.end();
		 ++i)
	{
		i->second->positionView(origin);
	}
}

void XYWndManager::positionActiveView(const Vector3& origin) {
	if (_activeXY != NULL) {
		return _activeXY->positionView(origin);
	}
}

EViewType XYWndManager::getActiveViewType() const {
	if (_activeXY != NULL) {
		return _activeXY->getViewType();
	}
	// Return at least anything
	return XY;
}

void XYWndManager::setActiveViewType(EViewType viewType) {
	if (_activeXY != NULL) {
		return _activeXY->setViewType(viewType);
	}
}

void XYWndManager::toggleActiveView(const cmd::ArgumentList& args) {
	if (_activeXY != NULL) {
		if (_activeXY->getViewType() == XY) {
			_activeXY->setViewType(XZ);
		}
		else if (_activeXY->getViewType() == XZ) {
			_activeXY->setViewType(YZ);
		}
		else {
			_activeXY->setViewType(XY);
		}

		positionActiveView(getFocusPosition());
	}
}

void XYWndManager::setActiveViewXY(const cmd::ArgumentList& args) {
	setActiveViewType(XY);
	positionActiveView(getFocusPosition());
}

void XYWndManager::setActiveViewXZ(const cmd::ArgumentList& args) {
	setActiveViewType(XZ);
	positionActiveView(getFocusPosition());
}

void XYWndManager::setActiveViewYZ(const cmd::ArgumentList& args) {
	setActiveViewType(YZ);
	positionActiveView(getFocusPosition());
}

void XYWndManager::splitViewFocus(const cmd::ArgumentList& args) {
	positionAllViews(getFocusPosition());
}

void XYWndManager::zoom100(const cmd::ArgumentList& args) {
	setScale(1);
}

void XYWndManager::focusActiveView(const cmd::ArgumentList& args) {
	positionActiveView(getFocusPosition());
}

XYWndPtr XYWndManager::getView(EViewType viewType)
{
	// Cycle through the list of views and get the one matching the type
	for (XYWndMap::iterator i = _xyWnds.begin();
		 i != _xyWnds.end();
		 ++i)
	{
		// If the view matches, return the pointer
		if (i->second->getViewType() == viewType) {
			return i->second;
		}
	}

	return XYWndPtr();
}

// Change the active XYWnd
void XYWndManager::setActiveXY(int index) {

	// Notify the currently active XYView that is has been deactivated
	if (_activeXY != NULL)
	{
		_activeXY->setActive(false);
	}

	// Find the ID in the map and update the active pointer
	XYWndMap::const_iterator it = _xyWnds.find(index);

	if (it != _xyWnds.end())
	{
		_activeXY = it->second;
	}
	else
	{
		throw std::logic_error(
			"Cannot set XYWnd with ID " + string::to_string(index) + " as active, "
			+ " ID not found in map."
		);
	}

	// Notify the new active XYView about its activation
	if (_activeXY != NULL)
	{
		_activeXY->setActive(true);
	}
}

void XYWndManager::setGlobalParentWindow(const Glib::RefPtr<Gtk::Window>& globalParentWindow)
{
	_globalParentWindow = globalParentWindow;
}

void XYWndManager::destroyXYWnd(int id)
{
	XYWndMap::iterator found = _xyWnds.find(id);

	if (found != _xyWnds.end())
	{
		// Remove the shared_ptr from the map
		_xyWnds.erase(found);
	}

	// Also check if the activeXY is holding a strong reference of the XYWnd
	// which prevents destruction - release the shared_ptr
	if (_activeXY != NULL && _activeXY->getId() == id)
	{
		_activeXY.reset();

		// Set the activeXY to the next possible XYWnd
		if (!_xyWnds.empty())
		{
			_activeXY = _xyWnds.begin()->second;
			_activeXY->setActive(true);
		}
	}
}

int XYWndManager::getUniqueID() const
{
	for (int i = 0; i < INT_MAX; ++i)
	{
		if (_xyWnds.count(i) == 0)
			return i;
	}

	throw std::runtime_error(
		"Cannot create unique ID for ortho view: no more IDs."
	);
}

// Create a standard (non-floating) ortho view
XYWndPtr XYWndManager::createEmbeddedOrthoView()
{
	// Allocate a new window and add it to the map
	int id = getUniqueID();

	XYWndPtr newWnd = XYWndPtr(new XYWnd(id));

	std::pair<XYWndMap::iterator, bool> result = _xyWnds.insert(
		XYWndMap::value_type(id, newWnd));

	// Ensure that the insertion is successful
	assert(result.second == true);

	// Tag the new view as active, if there is no active view yet
	if (_activeXY == NULL)
	{
		_activeXY = newWnd;
	}

	return newWnd;
}

// Create a new floating ortho view
XYWndPtr XYWndManager::createFloatingOrthoView(EViewType viewType)
{
	// Create a new XY view
	int uniqueId = getUniqueID();

	FloatingOrthoViewPtr newWnd(
		new FloatingOrthoView(
			uniqueId,
			XYWnd::getViewTypeTitle(viewType),
			_globalParentWindow
		)
	);

	std::pair<XYWndMap::iterator, bool> result = _xyWnds.insert(
		XYWndMap::value_type(uniqueId, newWnd));

	// Ensure that the insertion is successful
	assert(result.second == true);

	// Tag the new view as active, if there is no active view yet
	if (_activeXY == NULL)
	{
		_activeXY = newWnd;
	}

	// Set the viewtype and show the window
	newWnd->setViewType(viewType);
	newWnd->show();

	return newWnd;
}

// Shortcut method for connecting to a GlobalEventManager command
void XYWndManager::createXYFloatingOrthoView(const cmd::ArgumentList& args)
{
	createFloatingOrthoView(XY);
}

/* greebo: This function determines the point currently being "looked" at, it is used for toggling the ortho views
 * If something is selected the center of the selection is taken as new origin, otherwise the camera
 * position is considered to be the new origin of the toggled orthoview.
*/
Vector3 XYWndManager::getFocusPosition()
{
	Vector3 position(0,0,0);

	if (GlobalSelectionSystem().countSelected() != 0) {
		position = selection::algorithm::getCurrentSelectionCenter();
	}
	else {
		CamWndPtr cam = GlobalCamera().getActiveCamWnd();

		if (cam != NULL) {
			position = cam->getCameraOrigin();
		}
	}

	return position;
}

const std::string& XYWndManager::getName() const
{
	static std::string _name(MODULE_ORTHOVIEWMANAGER);
	return _name;
}

const StringSet& XYWndManager::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_RENDERSYSTEM);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_UIMANAGER);
	}

	return _dependencies;
}

void XYWndManager::observeKey(const std::string& key)
{
    GlobalRegistry().signalForKey(key).connect(
        sigc::mem_fun(this, &XYWndManager::refreshFromRegistry)
    );
}

void XYWndManager::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	// Connect self to the according registry keys
	observeKey(RKEY_CHASE_MOUSE);
	observeKey(RKEY_CAMERA_XY_UPDATE);
	observeKey(RKEY_SHOW_CROSSHAIRS);
	observeKey(RKEY_SHOW_GRID);
	observeKey(RKEY_SHOW_SIZE_INFO);
	observeKey(RKEY_SHOW_ENTITY_ANGLES);
	observeKey(RKEY_SHOW_ENTITY_NAMES);
	observeKey(RKEY_SHOW_BLOCKS);
	observeKey(RKEY_SHOW_COORDINATES);
	observeKey(RKEY_SHOW_OUTLINE);
	observeKey(RKEY_SHOW_AXES);
	observeKey(RKEY_SHOW_WORKZONE);
	observeKey(RKEY_DEFAULT_BLOCKSIZE);

	// Trigger loading the values of the observed registry keys
	refreshFromRegistry();

	// Construct the preference settings widgets
	constructPreferences();

	// Add the commands to the EventManager
	registerCommands();

	GlobalUIManager().getStatusBarManager().addTextElement(
		"XYZPos",
		"",  // no icon
		IStatusBarManager::POS_POSITION
	);

	XYWnd::captureStates();
}

void XYWndManager::shutdownModule()
{
	// Release all owned XYWndPtrs
	destroyViews();

	XYWnd::releaseStates();
}

// Define the static GlobalXYWnd module
module::StaticModule<XYWndManager> xyWndModule;

// Accessor function returning the reference
XYWndManager& GlobalXYWnd()
{
	return *xyWndModule.getModule();
}
