#include "GlobalXYWnd.h"
#include "FloatingOrthoView.h"

#include "ieventmanager.h"
#include "ipreferencesystem.h"

#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/FramedWidget.h"
#include "stringio.h"

#include "selection/algorithm/General.h"
#include "mainframe.h"

// Constructor
XYWndManager::XYWndManager() :
	_globalParentWindow(NULL)
{
	// Connect self to the according registry keys
	GlobalRegistry().addKeyObserver(this, RKEY_CHASE_MOUSE);
	GlobalRegistry().addKeyObserver(this, RKEY_CAMERA_XY_UPDATE);
	GlobalRegistry().addKeyObserver(this, RKEY_SHOW_CROSSHAIRS);
	GlobalRegistry().addKeyObserver(this, RKEY_SHOW_GRID);
	GlobalRegistry().addKeyObserver(this, RKEY_SHOW_SIZE_INFO);
	GlobalRegistry().addKeyObserver(this, RKEY_SHOW_ENTITY_ANGLES);
	GlobalRegistry().addKeyObserver(this, RKEY_SHOW_ENTITY_NAMES);
	GlobalRegistry().addKeyObserver(this, RKEY_SHOW_BLOCKS);
	GlobalRegistry().addKeyObserver(this, RKEY_SHOW_COORDINATES);
	GlobalRegistry().addKeyObserver(this, RKEY_SHOW_OUTLINE);
	GlobalRegistry().addKeyObserver(this, RKEY_SHOW_AXES);
	GlobalRegistry().addKeyObserver(this, RKEY_SHOW_WORKZONE);
	GlobalRegistry().addKeyObserver(this, RKEY_DEFAULT_BLOCKSIZE);
	
	// Trigger loading the values of the observed registry keys
	keyChanged("", "");
	
	// Construct the preference settings widgets
	constructPreferences();
	
	// Add the commands to the EventManager
	registerCommands();
}

void XYWndManager::construct() {
	XYWnd::captureStates();
}

// Release resources
void XYWndManager::destroy() {
	// Release all owned XYWndPtrs
	destroyViews();

	XYWnd::releaseStates();
}

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
void XYWndManager::restoreState() {
	xml::NodeList views = GlobalRegistry().findXPath(RKEY_XYVIEW_ROOT + "//views");
	
	if (views.size() > 0) {
		// Find all <view> tags under the first found <views> tag
		xml::NodeList viewList = views[0].getNamedChildren("view");
	
		for (xml::NodeList::const_iterator i = viewList.begin(); 
			 i != viewList.end();
			 ++i) 
		{
			// Create the view and restore the size
			XYWndPtr newWnd = createFloatingOrthoView(XY);
			newWnd->readStateFromNode(*i);
			
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
		globalOutputStream() << "XYWndManager: No xywindow information found in XMLRegistry, creating default view.\n";
		
		// Create a default OrthoView
		createFloatingOrthoView(XY);
	}
}

void XYWndManager::saveState() {

	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_XYVIEW_ROOT + "//views");
	
	// Create a new node
	xml::Node rootNode(GlobalRegistry().createKey(RKEY_XYVIEW_ROOT + "/views"));
	
	for (XYWndMap::iterator i = _xyWnds.begin(); i
		 != _xyWnds.end(); 
		 ++i) 
	{
		XYWndPtr xyView = i->second;
		
		xyView->saveStateToNode(rootNode);
		// gtk_widget_hide(GTK_WIDGET(xyView->getParent())); TODO: What?
	}
}

// Free the allocated XYViews from the heap
void XYWndManager::destroyViews() {
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
	GlobalEventManager().addCommand(
		"NewOrthoView", 
		MemberCaller<XYWndManager, &XYWndManager::createXYFloatingOrthoView>(
			*this
		)
	);
	GlobalEventManager().addCommand("NextView", MemberCaller<XYWndManager, &XYWndManager::toggleActiveView>(*this));
	GlobalEventManager().addCommand("ZoomIn", MemberCaller<XYWndManager, &XYWndManager::zoomIn>(*this));
	GlobalEventManager().addCommand("ZoomOut", MemberCaller<XYWndManager, &XYWndManager::zoomOut>(*this));
	GlobalEventManager().addCommand("ViewTop", MemberCaller<XYWndManager, &XYWndManager::setActiveViewXY>(*this));
	GlobalEventManager().addCommand("ViewSide", MemberCaller<XYWndManager, &XYWndManager::setActiveViewXZ>(*this));
	GlobalEventManager().addCommand("ViewFront", MemberCaller<XYWndManager, &XYWndManager::setActiveViewYZ>(*this));
	GlobalEventManager().addCommand("CenterXYViews", MemberCaller<XYWndManager, &XYWndManager::splitViewFocus>(*this));
	GlobalEventManager().addCommand("CenterXYView", MemberCaller<XYWndManager, &XYWndManager::focusActiveView>(*this));
	GlobalEventManager().addCommand("Zoom100", MemberCaller<XYWndManager, &XYWndManager::zoom100>(*this));
	
	GlobalEventManager().addRegistryToggle("ToggleCrosshairs", RKEY_SHOW_CROSSHAIRS);
	GlobalEventManager().addRegistryToggle("ToggleGrid", RKEY_SHOW_GRID);
	GlobalEventManager().addRegistryToggle("ShowAngles", RKEY_SHOW_ENTITY_ANGLES);
	GlobalEventManager().addRegistryToggle("ShowNames", RKEY_SHOW_ENTITY_NAMES);
	GlobalEventManager().addRegistryToggle("ShowBlocks", RKEY_SHOW_BLOCKS);
	GlobalEventManager().addRegistryToggle("ShowCoordinates", RKEY_SHOW_COORDINATES);
	GlobalEventManager().addRegistryToggle("ShowWindowOutline", RKEY_SHOW_OUTLINE);
	GlobalEventManager().addRegistryToggle("ShowAxes", RKEY_SHOW_AXES);
	GlobalEventManager().addRegistryToggle("ShowWorkzone", RKEY_SHOW_WORKZONE);
}

void XYWndManager::constructPreferences() {
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage("Settings/Orthoview");
	
	page->appendCheckBox("", "View chases Mouse Cursor during Drags", RKEY_CHASE_MOUSE);
	page->appendCheckBox("", "Update Views on Camera Movement", RKEY_CAMERA_XY_UPDATE);
	page->appendCheckBox("", "Show Crosshairs", RKEY_SHOW_CROSSHAIRS);
	page->appendCheckBox("", "Show Grid", RKEY_SHOW_GRID);
	page->appendCheckBox("", "Show Size Info", RKEY_SHOW_SIZE_INFO);
	page->appendCheckBox("", "Show Entity Angle Arrow", RKEY_SHOW_ENTITY_ANGLES);
	page->appendCheckBox("", "Show Entity Names", RKEY_SHOW_ENTITY_NAMES);
	page->appendCheckBox("", "Show Blocks", RKEY_SHOW_BLOCKS);
	page->appendCheckBox("", "Show Coordinates", RKEY_SHOW_COORDINATES);
	page->appendCheckBox("", "Show Axes", RKEY_SHOW_AXES);
	page->appendCheckBox("", "Show Window Outline", RKEY_SHOW_OUTLINE);
	page->appendCheckBox("", "Show Workzone", RKEY_SHOW_WORKZONE);
	page->appendCheckBox("", "Translate Manipulator always constrained to Axis", RKEY_TRANSLATE_CONSTRAINED);
	page->appendCheckBox("", "Higher Selection Priority for Entities", RKEY_HIGHER_ENTITY_PRIORITY);
}

// Load/Reload the values from the registry
void XYWndManager::keyChanged(const std::string& key, const std::string& val) 
{
	_chaseMouse = (GlobalRegistry().get(RKEY_CHASE_MOUSE) == "1");
	_camXYUpdate = (GlobalRegistry().get(RKEY_CAMERA_XY_UPDATE) == "1");
	_showCrossHairs = (GlobalRegistry().get(RKEY_SHOW_CROSSHAIRS) == "1");
	_showGrid = (GlobalRegistry().get(RKEY_SHOW_GRID) == "1");
	_showSizeInfo = (GlobalRegistry().get(RKEY_SHOW_SIZE_INFO) == "1");
	_showBlocks = (GlobalRegistry().get(RKEY_SHOW_BLOCKS) == "1");
	_showCoordinates = (GlobalRegistry().get(RKEY_SHOW_COORDINATES) == "1");
	_showOutline = (GlobalRegistry().get(RKEY_SHOW_OUTLINE) == "1");
	_showAxes = (GlobalRegistry().get(RKEY_SHOW_AXES) == "1");
	_showWorkzone = (GlobalRegistry().get(RKEY_SHOW_WORKZONE) == "1");
	_defaultBlockSize = (GlobalRegistry().getInt(RKEY_DEFAULT_BLOCKSIZE));
	updateAllViews();
}

bool XYWndManager::higherEntitySelectionPriority() const {
	return GlobalRegistry().get(RKEY_HIGHER_ENTITY_PRIORITY) == "1";
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

void XYWndManager::zoomIn() {
	if (_activeXY != NULL) {
		_activeXY->zoomIn();
	}
}

void XYWndManager::zoomOut() {
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

void XYWndManager::positionView(const Vector3& origin) {
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

void XYWndManager::toggleActiveView() {
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
		
		positionView(getFocusPosition());
	}
}

void XYWndManager::setActiveViewXY() {
	setActiveViewType(XY);
	positionView(getFocusPosition());
}

void XYWndManager::setActiveViewXZ() {
	setActiveViewType(XZ);
	positionView(getFocusPosition());
}

void XYWndManager::setActiveViewYZ() {
	setActiveViewType(YZ);
	positionView(getFocusPosition());
}

void XYWndManager::splitViewFocus() {
	positionAllViews(getFocusPosition());
}

void XYWndManager::zoom100() {
	setScale(1);
}

void XYWndManager::focusActiveView() {
	positionView(getFocusPosition());
}

XYWndPtr XYWndManager::getView(EViewType viewType) {
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
	if (_activeXY != NULL) {
		_activeXY->setActive(false);
	}
	
	// Find the ID in the map and update the active pointer
	XYWndMap::const_iterator it = _xyWnds.find(index);
	if (it != _xyWnds.end())
		_activeXY = it->second;
	else
		throw std::logic_error(
			"Cannot set XYWnd with ID " + intToStr(index) + " as active, "
			+ " ID not found in map."
		);
	
	// Notify the new active XYView about its activation
	if (_activeXY != NULL) {
		_activeXY->setActive(true);
	}
}

void XYWndManager::setGlobalParentWindow(GtkWindow* globalParentWindow) {
	_globalParentWindow = globalParentWindow;
}

// Notification for a floating XYWnd destruction, so that it can be removed
// from the map
void XYWndManager::notifyXYWndDestroy(int index) {
	XYWndMap::iterator found = _xyWnds.find(index);

	if (found != _xyWnds.end()) {
		_xyWnds.erase(found);
	}
}

// Create a unique ID for the window map
int XYWndManager::getUniqueID() const {
	for (int i = 0; i < INT_MAX; ++i) {
		if (_xyWnds.count(i) == 0)
			return i;
	}
	throw std::runtime_error(
		"Cannot create unique ID for ortho view: no more IDs."
	);
}

// Create a standard (non-floating) ortho view
XYWndPtr XYWndManager::createEmbeddedOrthoView() {

	// Allocate a new window and add it to the map
	int id = getUniqueID();
	XYWndPtr newWnd = XYWndPtr(new XYWnd(id));
	_xyWnds.insert(XYWndMap::value_type(id, newWnd));
	
	// Tag the new view as active, if there is no active view yet
	if (_activeXY == NULL) {
		_activeXY = newWnd;
	}
	
	return newWnd;
}

// Create a new floating ortho view
XYWndPtr XYWndManager::createFloatingOrthoView(EViewType viewType) {
	
	// Create a new XY view
	int uniqueId = getUniqueID();
	boost::shared_ptr<FloatingOrthoView> newWnd(
		new FloatingOrthoView(
			uniqueId, 
			XYWnd::getViewTypeTitle(viewType), 
			_globalParentWindow
		)
	);
	_xyWnds.insert(XYWndMap::value_type(uniqueId, newWnd));
	
	// Tag the new view as active, if there is no active view yet
	if (_activeXY == NULL) {
		_activeXY = newWnd;
	}

	// Set the viewtype and show the window
	newWnd->setViewType(viewType);
	newWnd->show();
	
	return newWnd;
}

// Shortcut method for connecting to a GlobalEventManager command
void XYWndManager::createXYFloatingOrthoView() {
	createFloatingOrthoView(XY);
}

/* greebo: This function determines the point currently being "looked" at, it is used for toggling the ortho views
 * If something is selected the center of the selection is taken as new origin, otherwise the camera
 * position is considered to be the new origin of the toggled orthoview.
*/
Vector3 XYWndManager::getFocusPosition() {
	Vector3 position(0,0,0);
	
	if (GlobalSelectionSystem().countSelected() != 0) {
		position = selection::algorithm::getCurrentSelectionCenter();
	}
	else {
		position = g_pParentWnd->GetCamWnd()->getCameraOrigin();
	}
	
	return position;
}

// Accessor function returning a reference to the static instance
XYWndManager& GlobalXYWnd() {
	static XYWndManager _xyWndManager;
	return _xyWndManager;
}
