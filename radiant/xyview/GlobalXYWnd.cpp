#include "GlobalXYWnd.h"

#include "ieventmanager.h"

#include "gtkutil/TransientWindow.h"
#include "gtkutil/FramedWidget.h"
#include "stringio.h"

#include "select.h"
#include "mainframe.h"

// Constructor
XYWndManager::XYWndManager() :
	_activeXY(NULL),
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
	keyChanged();
	
	// greebo: Register this class in the preference system so that the constructPreferencePage() gets called.
	GlobalPreferenceSystem().addConstructor(this);
	
	// Add the commands to the EventManager
	registerCommands();
}

// Destructor
XYWndManager::~XYWndManager() {
	// The method destroy() is called from mainframe.cpp
}

void XYWndManager::construct() {
	XYWnd::captureStates();
}

// Release the shader states
void XYWndManager::destroy() {
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
	
		if (viewList.size() > 0) {
			for (unsigned int i = 0; i < viewList.size(); i++) {
				GtkWidget* window = gtkutil::TransientWindow("OrthoView", _globalParentWindow);
				
				// Create the view and restore the size
				XYWnd* newWnd = createXY();
				newWnd->readStateFromNode(viewList[i], GTK_WINDOW(window));
				
				// Connect the destroyed signal to the callback of this class 
				g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(onDeleteOrthoView), newWnd);
				
				newWnd->setParent(GTK_WINDOW(window));
				
				const std::string typeStr = viewList[i].getAttributeValue("type");
		
				if (typeStr == "YZ") {
					newWnd->setViewType(YZ);
				}
				else if (typeStr == "XZ") {
					newWnd->setViewType(XZ);
				}
				else {
					newWnd->setViewType(XY);
				}
				
				GtkWidget* framedXYView = gtkutil::FramedWidget(newWnd->getWidget());
				
				gtk_container_add(GTK_CONTAINER(window), framedXYView);
				gtk_widget_show_all(GTK_WIDGET(window));
			}
		}
	}
	else {
		// Create at least one XYView, if no view info is found 
		globalOutputStream() << "XYWndManager: No xywindow information found in XMLRegistry, creating default view.\n";
		
		// Create a default OrthoView
		/*XYWnd* newWnd = */createOrthoView(XY);
	}
}

void XYWndManager::saveState() {
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_XYVIEW_ROOT + "//views");
	
	// Create a new node
	xml::Node rootNode(GlobalRegistry().createKey(RKEY_XYVIEW_ROOT + "/views"));
	
	for (XYWndList::iterator i = _XYViews.begin(); i != _XYViews.end(); i++) {
		XYWnd* xyView = *i;
		
		xyView->saveStateToNode(rootNode);
	}
}

// Free the allocated XYViews from the heap
void XYWndManager::destroyViews() {
	
	for (XYWndList::iterator i = _XYViews.begin(); i != _XYViews.end(); i++) {
		// Free the view from the heap
		XYWnd* xyView = *i;
		
		delete xyView;
	}
	// Discard the whole list
	_XYViews.clear();
}

void XYWndManager::registerCommands() {
	GlobalEventManager().addCommand("NewOrthoView", MemberCaller<XYWndManager, &XYWndManager::createNewOrthoView>(*this));
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

void XYWndManager::constructPreferencePage(PreferenceGroup& group) {
	PreferencesPage* page(group.createPage("Orthographic", "Orthographic View Settings"));
	
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
}

// Load/Reload the values from the registry
void XYWndManager::keyChanged() {
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
	for (XYWndList::iterator i = _XYViews.begin(); i != _XYViews.end(); i++) {
		XYWnd* xyview = *i;
		
		// Pass the call
		xyview->queueDraw();
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

XYWnd* XYWndManager::getActiveXY() const {
	return _activeXY;
}

void XYWndManager::setOrigin(const Vector3& origin) {
	// Cycle through the list of views and set the origin 
	for (XYWndList::iterator i = _XYViews.begin(); i != _XYViews.end(); i++) {
		XYWnd* xyView = *i;
		
		if (xyView != NULL) {
			// Pass the call
			xyView->setOrigin(origin);
		}
	}
}

void XYWndManager::setScale(float scale) {
	// Cycle through the list of views and set the origin 
	for (XYWndList::iterator i = _XYViews.begin(); i != _XYViews.end(); i++) {
		XYWnd* xyView = *i;
		
		if (xyView != NULL) {
			// Pass the call
			xyView->setScale(scale);
		}
	}
}

void XYWndManager::positionAllViews(const Vector3& origin) {
	// Cycle through the list of views and set the origin 
	for (XYWndList::iterator i = _XYViews.begin(); i != _XYViews.end(); i++) {
		XYWnd* xyView = *i;
		
		if (xyView != NULL) {
			// Pass the call
			xyView->positionView(origin);
		}
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

XYWnd* XYWndManager::getView(EViewType viewType) {
	// Cycle through the list of views and get the one matching the type 
	for (XYWndList::iterator i = _XYViews.begin(); i != _XYViews.end(); i++) {
		XYWnd* xyView = *i;
		
		if (xyView != NULL) {
			// If the view matches, return the pointer
			if (xyView->getViewType() == viewType) {
				return xyView;
			}
		}
	}

	return NULL;
}

void XYWndManager::setActiveXY(XYWnd* wnd) {
	// Notify the currently active XYView that is has been deactivated
	if (_activeXY != NULL) {
		_activeXY->setActive(false);
	}
	
	// Update the pointer
	_activeXY = wnd;
	
	// Notify the new active XYView about its activation
	if (_activeXY != NULL) {
		_activeXY->setActive(true);
	}
}

XYWnd* XYWndManager::createXY() {
	// Allocate a new window
	XYWnd* newWnd = new XYWnd();
	
	// Add it to the internal list and return the pointer
	_XYViews.push_back(newWnd);
	
	// Tag the new view as active, if there is no active view yet
	if (_activeXY == NULL) {
		_activeXY = newWnd;
	}
	
	return newWnd;
}

void XYWndManager::setGlobalParentWindow(GtkWindow* globalParentWindow) {
	_globalParentWindow = globalParentWindow;
}

void XYWndManager::destroyOrthoView(XYWnd* xyWnd) {
	if (xyWnd != NULL) {		
		
		// Remove the pointer from the list
		for (XYWndList::iterator i = _XYViews.begin(); i != _XYViews.end(); i++) {
			XYWnd* listItem = (*i);
		
			// If the view is found, remove it from the list
			if (listItem == xyWnd) {
				// Retrieve the parent from the view (for later destruction)
				GtkWindow* parent = xyWnd->getParent();
				GtkWidget* glWidget = xyWnd->getWidget();
				
				if (_activeXY == xyWnd) {
					_activeXY = NULL;
				}
				
				// Destroy the window
				delete xyWnd;
				
				// Remove it from the list
				_XYViews.erase(i);
				
				// Destroy the parent window (and the contained frame) as well
				if (parent != NULL) {
					gtk_widget_destroy(GTK_WIDGET(glWidget));
					gtk_widget_destroy(GTK_WIDGET(parent));
				}
				break;
			}
		}
	}
}

gboolean XYWndManager::onDeleteOrthoView(GtkWidget *widget, GdkEvent *event, gpointer data) {
	// Get the pointer to the deleted XY view from data
	XYWnd* deletedView = reinterpret_cast<XYWnd*>(data);
	
	GlobalXYWnd().destroyOrthoView(deletedView);
	
	return false;
}

XYWnd* XYWndManager::createOrthoView(EViewType viewType) {
	
	// Create a new XY view
	XYWnd* newWnd = createXY();
	
	// Add the new XYView GL widget to a framed window
	GtkWidget* window = gtkutil::TransientWindow(
							XYWnd::getViewTypeTitle(viewType), 
							_globalParentWindow);
	gtk_container_add(GTK_CONTAINER(window), newWnd->getWidget());
	
	// Connect the destroyed signal to the callback of this class 
	g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(onDeleteOrthoView), newWnd);
	
	newWnd->setParent(GTK_WINDOW(window));
	newWnd->connectWindowPosition();
	
	// Set the viewtype (and with it the window title)
	newWnd->setViewType(viewType);
	
	return newWnd;
}

// Shortcut method for connecting to a GlobalEventManager command
void XYWndManager::createNewOrthoView() {
	createOrthoView(XY);
}

/* greebo: This function determines the point currently being "looked" at, it is used for toggling the ortho views
 * If something is selected the center of the selection is taken as new origin, otherwise the camera
 * position is considered to be the new origin of the toggled orthoview.
*/
Vector3 XYWndManager::getFocusPosition() {
	Vector3 position(0,0,0);
	
	if (GlobalSelectionSystem().countSelected() != 0) {
		Select_GetMid(position);
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
