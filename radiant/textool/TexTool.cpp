#include "TexTool.h"

#include "ieventmanager.h"
#include "iregistry.h"
#include "igl.h"
#include "iundo.h"
#include "iuimanager.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "patch/PatchNode.h"
#include "texturelib.h"
#include "selectionlib.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/FramedWidget.h"
#include "gtkutil/GLWidgetSentry.h"
#include "mainframe.h"
#include "brush/Face.h"
#include "brush/BrushNode.h"
#include "brush/Winding.h"
#include "camera/GlobalCamera.h"

#include "textool/Selectable.h"
#include "textool/Transformable.h"
#include "textool/item/PatchItem.h"
#include "textool/item/BrushItem.h"
#include "textool/item/FaceItem.h"

#include "selection/algorithm/Primitives.h"
#include "selection/algorithm/Shader.h"

namespace ui {
	
	namespace {
		const std::string WINDOW_TITLE = "Texture Tool";
		
		const std::string RKEY_ROOT = "user/ui/textures/texTool/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
		const std::string RKEY_GRID_STATE = RKEY_ROOT + "gridActive";
		
		const float DEFAULT_ZOOM_FACTOR = 1.5f;
		const float ZOOM_MODIFIER = 1.25f;
		const float MOVE_FACTOR = 2.0f;
		
		const float GRID_MAX = 1.0f;
		const float GRID_DEFAULT = 0.0625f;
		const float GRID_MIN = 0.00390625f;
	}

TexTool::TexTool() 
: gtkutil::PersistentTransientWindow(WINDOW_TITLE, MainFrame_getWindow(), true),
  _glWidget(true),
  _selectionInfo(GlobalSelectionSystem().getSelectionInfo()),
  _zoomFactor(DEFAULT_ZOOM_FACTOR),
  _dragRectangle(false),
  _manipulatorMode(false),
  _viewOriginMove(false),
  _grid(GRID_DEFAULT),
  _gridActive(GlobalRegistry().get(RKEY_GRID_STATE) == "1")
{
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	g_signal_connect(G_OBJECT(getWindow()), "focus-in-event", G_CALLBACK(triggerRedraw), this);
	g_signal_connect(G_OBJECT(getWindow()), "key_press_event", G_CALLBACK(onKeyPress), this);
	
	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connect(GTK_OBJECT(getWindow()));
	
	populateWindow();
	
	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(getWindow()));
	_windowPosition.applyPosition();
	
	// Register self to the SelSystem to get notified upon selection changes.
	GlobalSelectionSystem().addObserver(this);
	
	GlobalRegistry().addKeyObserver(this, RKEY_GRID_STATE);
}

TexToolPtr& TexTool::InstancePtr() {
	static TexToolPtr _instancePtr;
	
	if (_instancePtr == NULL) {
		// Not yet instantiated, do it now
		_instancePtr = TexToolPtr(new TexTool);
		
		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().addEventListener(_instancePtr);
	}
	
	return _instancePtr;
}

void TexTool::keyChanged(const std::string& key, const std::string& val) 
{
	_gridActive = (val == "1");
	draw();
}

void TexTool::populateWindow() {
	
	// Load the texture toolbar from the registry
    IToolbarManager& tbCreator = GlobalUIManager().getToolbarManager();
    GtkToolbar* textoolbar = tbCreator.getToolbar("textool");
    GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(textoolbar), GTK_CAN_FOCUS);
    
	// Create the GL widget
	GtkWidget* glWidget = _glWidget; // cast to GtkWidget*
	GtkWidget* frame = gtkutil::FramedWidget(glWidget);
	
	// Connect the events
	gtk_widget_set_events(glWidget, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);
	g_signal_connect(G_OBJECT(glWidget), "expose-event", G_CALLBACK(onExpose), this);
	g_signal_connect(G_OBJECT(glWidget), "focus-in-event", G_CALLBACK(triggerRedraw), this);
	g_signal_connect(G_OBJECT(glWidget), "button-press-event", G_CALLBACK(onMouseDown), this);
	g_signal_connect(G_OBJECT(glWidget), "button-release-event", G_CALLBACK(onMouseUp), this);
	g_signal_connect(G_OBJECT(glWidget), "motion-notify-event", G_CALLBACK(onMouseMotion), this);
	g_signal_connect(G_OBJECT(glWidget), "key_press_event", G_CALLBACK(onKeyPress), this);
	g_signal_connect(G_OBJECT(glWidget), "scroll_event", G_CALLBACK(onMouseScroll), this);
	
	// Make the GL widget accept the global shortcuts
	GlobalEventManager().connect(GTK_OBJECT(glWidget));
	
	// Create a top-level vbox, pack it and add it to the window 
	GtkWidget* vbox = gtk_vbox_new(false, 0);
	
	if (textoolbar != NULL) {
    	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(textoolbar), false, false, 0);
    }
	
	gtk_box_pack_start(GTK_BOX(vbox), frame, true, true, 0);
	
	gtk_container_add(GTK_CONTAINER(getWindow()), vbox);
}

void TexTool::toggleWindow() {
	if (isVisible())
		hide();
	else
		show();
}

// Pre-hide callback
void TexTool::_preHide() {
	// Save the window position, to make sure
	_windowPosition.readPosition();
}

// Pre-show callback
void TexTool::_preShow() {
	// Trigger an update of the current selection
	rescanSelection();
	// Restore the position
	_windowPosition.applyPosition();
}

void TexTool::gridUp() {
	if (_grid*2 <= GRID_MAX && _gridActive) {
		_grid *= 2;
		draw();
	}
}

void TexTool::gridDown() {
	if (_grid/2 >= GRID_MIN && _gridActive) {
		_grid /= 2;
		draw();
	}
}

void TexTool::onRadiantShutdown() {
	// Release the shader
	_shader = IShaderPtr();

	// De-register this as selectionsystem observer
	GlobalSelectionSystem().removeObserver(this);
	
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
	
	GlobalEventManager().disconnect(GTK_OBJECT(static_cast<GtkWidget*>(_glWidget)));
	GlobalEventManager().disconnect(GTK_OBJECT(getWindow()));

	// Destroy the window
	destroy();
}

TexTool& TexTool::Instance() {
	return *InstancePtr();
}

void TexTool::update() {
	std::string selectedShader = selection::algorithm::getShaderFromSelection();
	_shader = GlobalShaderSystem().getShaderForName(selectedShader);
}

void TexTool::rescanSelection() {
	update();
	
	// Clear the list to remove all the previously allocated items
	_items.clear();
	
	// Does the selection use one single shader?
	if (std::string(_shader->getName()) != "") {
		if (_selectionInfo.patchCount > 0) {
			// One single named shader, get the selection list
			PatchPtrVector patchList = selection::algorithm::getSelectedPatches();
			
			for (std::size_t i = 0; i < patchList.size(); i++) {
				// Allocate a new PatchItem on the heap (shared_ptr)
				textool::TexToolItemPtr patchItem(
					new textool::PatchItem(patchList[i]->getPatch())
				);
				
				// Add it to the list
				_items.push_back(patchItem);
			}
		}
		
		if (_selectionInfo.brushCount > 0) {
			BrushPtrVector brushList = selection::algorithm::getSelectedBrushes();
			
			for (std::size_t i = 0; i < brushList.size(); i++) {
				// Allocate a new BrushItem on the heap (shared_ptr)
				textool::TexToolItemPtr brushItem(
					new textool::BrushItem(brushList[i]->getBrush())
				);
				
				// Add it to the list
				_items.push_back(brushItem);
			}
		}
		
		// Get the single selected faces
		FacePtrVector faceList = selection::algorithm::getSelectedFaces();
		
		for (std::size_t i = 0; i < faceList.size(); i++) {
			// Allocate a new FaceItem on the heap (shared_ptr)
			textool::TexToolItemPtr faceItem(
				new textool::FaceItem(*faceList[i])
			);
			
			// Add it to the list
			_items.push_back(faceItem);
		}
	}
	
	recalculateVisibleTexSpace();
}

void TexTool::selectionChanged(const scene::INodePtr& node, bool isComponent) {
	rescanSelection();
	draw();
}

void TexTool::flipSelected(int axis) {
	if (countSelected() > 0) {
		beginOperation();
		
		for (std::size_t i = 0; i < _items.size(); i++) {
			_items[i]->flipSelected(axis);
		}
		
		draw();
		endOperation("TexToolMergeItems");
	}
}

void TexTool::mergeSelectedItems() {
	if (countSelected() > 0) {
		AABB selExtents;
		
		for (std::size_t i = 0; i < _items.size(); i++) {
			selExtents.includeAABB(_items[i]->getSelectedExtents());
		}
		
		if (selExtents.isValid()) {
			beginOperation();
		
			Vector2 centroid(
				selExtents.origin[0],
				selExtents.origin[1]
			);
			
			for (std::size_t i = 0; i < _items.size(); i++) {
				_items[i]->moveSelectedTo(centroid);
			}
			
			draw();
			
			endOperation("TexToolMergeItems");
		}
	}
}

void TexTool::snapToGrid() {
	if (_gridActive) {
		beginOperation();
		
		for (std::size_t i = 0; i < _items.size(); i++) {
			_items[i]->snapSelectedToGrid(_grid);
		}
		
		endOperation("TexToolSnapToGrid");
		
		draw();
	}
}

int TexTool::countSelected() {
	// The storage variable for use in the visitor class
	int selCount = 0;
	
	// Create the visitor class and let it walk
	textool::SelectedCounter counter(selCount);
	foreachItem(counter);
	
	return selCount;
}

bool TexTool::setAllSelected(bool selected) {
	
	if (countSelected() == 0 && !selected) {
		// Nothing selected and de-selection requested, 
		// return FALSE to propagate the command
		return false;
	}
	else {
		// Clear the selection using a visitor class
		textool::SetSelectedWalker visitor(selected);
		foreachItem(visitor);
	
		// Redraw to visualise the changes
		draw();
		
		// Return success
		return true;
	}
}

void TexTool::recalculateVisibleTexSpace() {
	// Get the selection extents
	AABB& selAABB = getExtents();
	
	// Reset the viewport zoom
	_zoomFactor = DEFAULT_ZOOM_FACTOR;
	
	// Relocate and resize the texSpace AABB
	_texSpaceAABB = AABB(selAABB.origin, selAABB.extents);
	
	// Normalise the plane to be square
	_texSpaceAABB.extents[0] = std::max(_texSpaceAABB.extents[0], _texSpaceAABB.extents[1]);
	_texSpaceAABB.extents[1] = std::max(_texSpaceAABB.extents[0], _texSpaceAABB.extents[1]);
}

void TexTool::draw() {
	// Redraw
	gtk_widget_queue_draw(_glWidget);
}

AABB& TexTool::getExtents() {
	_selAABB = AABB();
	
	for (std::size_t i = 0; i < _items.size(); i++) {
		// Expand the selection AABB by the extents of the item
		_selAABB.includeAABB(_items[i]->getExtents());
	}
	
	return _selAABB;
}

AABB& TexTool::getVisibleTexSpace() {
	return _texSpaceAABB;
}

Vector2 TexTool::getTextureCoords(const double& x, const double& y) {
	Vector2 result;
	
	if (_selAABB.isValid()) {
		Vector3 aabbTL = _texSpaceAABB.origin - _texSpaceAABB.extents * _zoomFactor;
		Vector3 aabbBR = _texSpaceAABB.origin + _texSpaceAABB.extents * _zoomFactor;
		
		Vector2 topLeft(aabbTL[0], aabbTL[1]);
		Vector2 bottomRight(aabbBR[0], aabbBR[1]);
		
		// Determine the texcoords by the according proportionality factors
		result[0] = topLeft[0] + x * (bottomRight[0]-topLeft[0]) / _windowDims[0];
		result[1] = topLeft[1] + y * (bottomRight[1]-topLeft[1]) / _windowDims[1];
	}
	
	return result;
}

void TexTool::drawUVCoords() {
	// Cycle through the items and tell them to render themselves
	for (std::size_t i = 0; i < _items.size(); i++) {
		_items[i]->render();
	}
}

textool::TexToolItemVec 
	TexTool::getSelectables(const textool::Rectangle& rectangle)
{
	textool::TexToolItemVec selectables;
	
	// Cycle through all the toplevel items and test them for selectability
	for (std::size_t i = 0; i < _items.size(); i++) {
		if (_items[i]->testSelect(rectangle)) {
			selectables.push_back(_items[i]);
		}
	}
	
	// Cycle through all the items and ask them to deliver the list of child selectables
	// residing within the test rectangle
	for (std::size_t i = 0; i < _items.size(); i++) {
		// Get the list from each item
		textool::TexToolItemVec found = 
			_items[i]->getSelectableChilds(rectangle);
		
		// and append the vector to the existing vector
		selectables.insert(selectables.end(), found.begin(), found.end());
	}
	
	return selectables;
}

textool::TexToolItemVec TexTool::getSelectables(const Vector2& coords) {
	// Construct a test rectangle with 2% of the width/height
	// of the visible texture space
	textool::Rectangle testRectangle;
	
	Vector3 extents = getVisibleTexSpace().extents * _zoomFactor;
	
	testRectangle.topLeft[0] = coords[0] - extents[0]*0.02; 
	testRectangle.topLeft[1] = coords[1] - extents[1]*0.02;
	testRectangle.bottomRight[0] = coords[0] + extents[0]*0.02; 
	testRectangle.bottomRight[1] = coords[1] + extents[1]*0.02;
	
	// Pass the call on to the getSelectables(<RECTANGLE>) method
	return getSelectables(testRectangle);
}

void TexTool::beginOperation() {
	// Start an undo recording session
	GlobalUndoSystem().start();
	
	// Tell all the items to save their memento
	for (std::size_t i = 0; i < _items.size(); i++) {
		_items[i]->beginTransformation();
	}
}

void TexTool::endOperation(const std::string& commandName) {
	for (std::size_t i = 0; i < _items.size(); i++) {
		_items[i]->endTransformation();
	}
	GlobalUndoSystem().finish(commandName);
}

void TexTool::doMouseUp(const Vector2& coords, GdkEventButton* event) {
	
	ui::XYViewEvent xyViewEvent = 
		GlobalEventManager().MouseEvents().getXYViewEvent(event);
		
	// End the origin move, if it was active before
	if (xyViewEvent == ui::xyMoveView) {
		_viewOriginMove = false;
	}
	
	// Retrieve the according ObserverEvent for the GdkEventButton
	ui::ObserverEvent observerEvent = 
		GlobalEventManager().MouseEvents().getObserverEvent(event);
	
	// If we are in manipulation mode, end the move
	if (observerEvent == ui::obsManipulate && _manipulatorMode) {
		_manipulatorMode = false;
		
		// Finish the undo recording, store the accumulated undomementos
		endOperation("TexToolDrag");
	}
	
	// If we are in selection mode, end the selection
	if ((observerEvent == ui::obsSelect || observerEvent == ui::obsToggle) 
		 && _dragRectangle) 
	{
		_dragRectangle = false;
		
		// Make sure the corners are in the correct order
		_selectionRectangle.sortCorners();
		
		// The minimim rectangle diameter for a rectangle test (3 % of visible texspace) 
		float minDist = _texSpaceAABB.extents[0] * _zoomFactor * 0.03;
		
		textool::TexToolItemVec selectables;
		
		if ((coords - _selectionRectangle.topLeft).getLength() < minDist) {
			// Perform a point selection test
			selectables = getSelectables(_selectionRectangle.topLeft);
		}
		else {
			// Perform the regular selectiontest
			selectables = getSelectables(_selectionRectangle);
		}
		
		// Toggle the selection
		for (std::size_t i = 0; i < selectables.size(); i++) {
			selectables[i]->toggle();
		}
	}
	
	draw();
}

void TexTool::doMouseMove(const Vector2& coords, GdkEventMotion* event) {
	if (_dragRectangle) {
		_selectionRectangle.bottomRight = coords;
		draw();
	}
	else if (_manipulatorMode) {
		Vector2 delta = coords - _manipulateRectangle.topLeft;
		
		// Snap the operations to the grid
		Vector3 snapped(0,0,0);
		
		if (_gridActive) {
			snapped[0] = (fabs(delta[0]) > 0.0f) ? 
				floor(fabs(delta[0]) / _grid)*_grid * delta[0]/fabs(delta[0]) : 
				0.0f;
				
			snapped[1] = (fabs(delta[1]) > 0.0f) ? 
				floor(fabs(delta[1]) / _grid)*_grid * delta[1]/fabs(delta[1]) : 
				0.0f;
		}
		else {
			snapped[0] = delta[0];
			snapped[1] = delta[1]; 
		}
		
		if (snapped.getLength() > 0) {
			// Create the transformation matrix
			Matrix4 transform = Matrix4::getTranslation(snapped);
			
			// Transform the selected
			// The transformSelected() call is propagated down the entire tree
			// of available items (e.g. PatchItem > PatchVertexItems)
			for (std::size_t i = 0; i < _items.size(); i++) {
				_items[i]->transformSelected(transform);
			}
			
			// Move the starting point by the effective translation
			_manipulateRectangle.topLeft[0] += transform.tx();
			_manipulateRectangle.topLeft[1] += transform.ty();
			
			draw();
		
			// Update the camera to reflect the changes
			GlobalCamera().update();
		}
	}
}

void TexTool::doMouseDown(const Vector2& coords, GdkEventButton* event) {
	
	// Retrieve the according ObserverEvent for the GdkEventButton
	ui::ObserverEvent observerEvent = 
		GlobalEventManager().MouseEvents().getObserverEvent(event);
	
	_manipulatorMode = false;
	_dragRectangle = false;
	_viewOriginMove = false;
	
	if (observerEvent == ui::obsManipulate) {
		// Get the list of selectables of this point
		textool::TexToolItemVec selectables;
		selectables = getSelectables(coords);
		
		// Any selectables under the mouse pointer?
		if (selectables.size() > 0) {
			// Activate the manipulator mode
			_manipulatorMode = true;
			_manipulateRectangle.topLeft = coords; 
			_manipulateRectangle.bottomRight = coords;
			
			// Begin the undoable operation
			beginOperation();
		}
	}
	else if (observerEvent == ui::obsSelect || observerEvent == ui::obsToggle) {
		// Start a drag or click operation
		_dragRectangle = true;
		_selectionRectangle.topLeft = coords;
		_selectionRectangle.bottomRight = coords;
	}
}

void TexTool::selectRelatedItems() {
	for (std::size_t i = 0; i < _items.size(); i++) {
		_items[i]->selectRelated();
	}
	draw();
}

void TexTool::foreachItem(textool::ItemVisitor& visitor) {
	for (std::size_t i = 0; i < _items.size(); i++) {
		// Visit the class
		visitor.visit(_items[i]);
		
		// Now propagate the visitor down the hierarchy
		_items[i]->foreachItem(visitor);
	}
}

void TexTool::drawGrid() {
	AABB& texSpaceAABB = getVisibleTexSpace();
	
	Vector3 topLeft = texSpaceAABB.origin - texSpaceAABB.extents * _zoomFactor;
	Vector3 bottomRight = texSpaceAABB.origin + texSpaceAABB.extents * _zoomFactor;
	
	if (topLeft[0] > bottomRight[0]) {
		std::swap(topLeft[0], bottomRight[0]);
	}
	
	if (topLeft[1] > bottomRight[1]) {
		std::swap(topLeft[1], bottomRight[1]);
	}
	
	float startX = floor(topLeft[0]) - 1;
	float endX = ceil(bottomRight[0]) + 1;
	float startY = floor(topLeft[1]) - 1;
	float endY = ceil(bottomRight[1]) + 1;
	
	glBegin(GL_LINES);
	
	// Draw the integer grid
	glColor4f(0.4f, 0.4f, 0.4f, 0.4f);
	
	for (int y = static_cast<int>(startY); y <= static_cast<int>(endY); y++) {
		glVertex2f(startX, y);
		glVertex2f(endX, y);
	}
	
	for (int x = static_cast<int>(startX); x <= static_cast<int>(endX); x++) {
		glVertex2f(x, startY);
		glVertex2f(x, endY);
	}
	
	if (_gridActive) {
		// Draw the manipulation grid
		glColor4f(0.2f, 0.2f, 0.2f, 0.4f);
		for (float y = startY; y <= endY; y += _grid) {
			glVertex2f(startX, y);
			glVertex2f(endX, y);
		}
		
		for (float x = startX; x <= endX; x += _grid) {
			glVertex2f(x, startY);
			glVertex2f(x, endY); 
		}
	}
	
	// Draw the axes through the origin
	glColor4f(1, 1, 1, 0.5f);
	glVertex2f(0, startY);
	glVertex2f(0, endY);
	
	glVertex2f(startX, 0);
	glVertex2f(endX, 0);
	
	glEnd();
	
	// Draw coordinate strings
	for (int y = static_cast<int>(startY); y <= static_cast<int>(endY); y++) {
		glRasterPos2f(topLeft[0] + 0.05f, y + 0.05f);
		std::string ycoordStr = intToStr(y) + ".0";  
		GlobalOpenGL().drawString(ycoordStr); 
	}
	
	for (int x = static_cast<int>(startX); x <= static_cast<int>(endX); x++) {
		glRasterPos2f(x + 0.05f, topLeft[1] + 0.03f * _zoomFactor);
		std::string xcoordStr = intToStr(x) + ".0";  
		GlobalOpenGL().drawString(xcoordStr); 
	}
}

gboolean TexTool::onExpose(GtkWidget* widget, GdkEventExpose* event, TexTool* self) {
	// Update the information about the current selection 
	self->update();
	
	// Activate the GL widget
	gtkutil::GLWidgetSentry sentry(self->_glWidget);
	
	// Store the window dimensions for later calculations
	self->_windowDims = Vector2(event->area.width, event->area.height);
	
	// Initialise the viewport
	glViewport(0, 0, event->area.width, event->area.height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	// Clear the window with the specified background colour
	glClearColor(0.1f, 0.1f, 0.1f, 0); 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	
	// Do nothing, if the shader name is empty
	std::string shaderName = self->_shader->getName(); 
	if (shaderName == "") {
		return false;
	}
	
	AABB& selAABB = self->getExtents(); 
	
	// Is there a valid selection?
	if (!selAABB.isValid()) {
		return false;
	}
	
	AABB& texSpaceAABB = self->getVisibleTexSpace();
	
	// Get the upper left and lower right corner coordinates
	Vector3 orthoTopLeft = texSpaceAABB.origin - texSpaceAABB.extents * self->_zoomFactor;
	Vector3 orthoBottomRight = texSpaceAABB.origin + texSpaceAABB.extents * self->_zoomFactor;
	
	// Initialise the 2D projection matrix with: left, right, bottom, top, znear, zfar 
	glOrtho(orthoTopLeft[0], 	// left 
			orthoBottomRight[0], // right
			orthoBottomRight[1], // bottom 
			orthoTopLeft[1], 	// top 
			-1, 1);
	
	glColor3f(1, 1, 1);
	// Tell openGL to draw front and back of the polygons in textured mode
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
	// Acquire the texture number of the active texture
	TexturePtr tex = self->_shader->getTexture();
	glBindTexture(GL_TEXTURE_2D, tex->texture_number);
	
	// Draw the background texture
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	
	glTexCoord2f(orthoTopLeft[0], orthoTopLeft[1]);
	glVertex2f(orthoTopLeft[0], orthoTopLeft[1]);	// Upper left
	
	glTexCoord2f(orthoBottomRight[0], orthoTopLeft[1]);
	glVertex2f(orthoBottomRight[0], orthoTopLeft[1]);	// Upper right
	
	glTexCoord2f(orthoBottomRight[0], orthoBottomRight[1]);
	glVertex2f(orthoBottomRight[0], orthoBottomRight[1]);	// Lower right
	
	glTexCoord2f(orthoTopLeft[0], orthoBottomRight[1]);
	glVertex2f(orthoTopLeft[0], orthoBottomRight[1]);	// Lower left
	
	glEnd();
	glDisable(GL_TEXTURE_2D);
	
	// Draw the grid
	self->drawGrid();
	
	// Draw the u/v coordinates
	self->drawUVCoords();
	
	if (self->_dragRectangle) {
		// Create a working reference to save typing
		textool::Rectangle& rectangle = self->_selectionRectangle;
		
		// Define the blend function for transparency
		glEnable(GL_BLEND);
		glBlendColor(0,0,0, 0.2f);
		glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);
		
		glColor3f(0.8f, 0.8f, 1);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		// The transparent fill rectangle
		glBegin(GL_QUADS);
		glVertex2f(rectangle.topLeft[0], rectangle.topLeft[1]);
		glVertex2f(rectangle.bottomRight[0], rectangle.topLeft[1]); 
		glVertex2f(rectangle.bottomRight[0], rectangle.bottomRight[1]);
		glVertex2f(rectangle.topLeft[0], rectangle.bottomRight[1]);
		glEnd();
		// The solid borders
		glBlendColor(0,0,0, 0.8f);
		glBegin(GL_LINE_LOOP);
		glVertex2f(rectangle.topLeft[0], rectangle.topLeft[1]);
		glVertex2f(rectangle.bottomRight[0], rectangle.topLeft[1]); 
		glVertex2f(rectangle.bottomRight[0], rectangle.bottomRight[1]);
		glVertex2f(rectangle.topLeft[0], rectangle.bottomRight[1]);
		glEnd();
		glDisable(GL_BLEND);
	}
	
	return false;
}

gboolean TexTool::triggerRedraw(GtkWidget* widget, GdkEventFocus* event, TexTool* self) {
	// Trigger a redraw
	self->draw();
	return false;
}

gboolean TexTool::onMouseUp(GtkWidget* widget, GdkEventButton* event, TexTool* self) {
	// Calculate the texture coords from the x/y click coordinates
	Vector2 texCoords = self->getTextureCoords(event->x, event->y);
	
	// Pass the call to the member method
	self->doMouseUp(texCoords, event);
	
	// Check for view origin movements
	IMouseEvents& mouseEvents = GlobalEventManager().MouseEvents();

	if (mouseEvents.stateMatchesXYViewEvent(ui::xyMoveView, event)) {
		self->_viewOriginMove = false;
	}
	
	return false;
}

gboolean TexTool::onMouseDown(GtkWidget* widget, GdkEventButton* event, TexTool* self) {
	// Calculate the texture coords from the x/y click coordinates
	Vector2 texCoords = self->getTextureCoords(event->x, event->y);
	
	// Pass the call to the member method
	self->doMouseDown(texCoords, event);
	
	// Check for view origin movements
	IMouseEvents& mouseEvents = GlobalEventManager().MouseEvents();

	if (mouseEvents.stateMatchesXYViewEvent(ui::xyMoveView, event)) {
		self->_moveOriginRectangle.topLeft = Vector2(event->x, event->y);
		self->_viewOriginMove = true;
	}
	
	return false;
}

gboolean TexTool::onMouseMotion(GtkWidget* widget, GdkEventMotion* event, TexTool* self) {
	// Calculate the texture coords from the x/y click coordinates
	Vector2 texCoords = self->getTextureCoords(event->x, event->y);
	
	// Pass the call to the member routine
	self->doMouseMove(texCoords, event);
	
	// Check for view origin movements
	IMouseEvents& mouseEvents = GlobalEventManager().MouseEvents();

	if (mouseEvents.stateMatchesXYViewEvent(ui::xyMoveView, event->state)) {
		
		// Calculate the movement delta relative to the old window x,y coords
		Vector2 delta = Vector2(event->x, event->y) - self->_moveOriginRectangle.topLeft;
		
		AABB& texSpaceAABB = self->getVisibleTexSpace();
		
		float speedFactor = self->_zoomFactor * MOVE_FACTOR;
		
		float factorX = texSpaceAABB.extents[0] / self->_windowDims[0] * speedFactor;
		float factorY = texSpaceAABB.extents[1] / self->_windowDims[1] * speedFactor;
		
		texSpaceAABB.origin[0] -= delta[0] * factorX;
		texSpaceAABB.origin[1] -= delta[1] * factorY; 
		
		// Store the new coordinates
		self->_moveOriginRectangle.topLeft = Vector2(event->x, event->y);
		
		// Redraw to visualise the changes
		self->draw();
	}

	return false;
}

// The GTK keypress callback
gboolean TexTool::onKeyPress(GtkWindow* window, GdkEventKey* event, TexTool* self) {
	
	// Check for ESC to deselect all items
	if (event->keyval == GDK_Escape) {
		// Don't propage the keypress if the ESC could be processed
		// setAllSelected returns TRUE in that case
		return self->setAllSelected(false);
	}
	
	return false;
}

gboolean TexTool::onMouseScroll(GtkWidget* widget, GdkEventScroll* event, TexTool* self) {
	
	if (event->direction == GDK_SCROLL_UP) {
		self->_zoomFactor /= ZOOM_MODIFIER;
		
		self->draw();
	}
	else if (event->direction == GDK_SCROLL_DOWN) {
		self->_zoomFactor *= ZOOM_MODIFIER;
		self->draw();
	}
	
	return false;
}

// Static command targets
void TexTool::toggle() {
	// Call the toggle() method of the static instance
	Instance().toggleWindow();
}

void TexTool::texToolGridUp() {
	Instance().gridUp();
}

void TexTool::texToolGridDown() {
	Instance().gridDown();
}

void TexTool::texToolSnapToGrid() {
	Instance().snapToGrid();
}

void TexTool::texToolMergeItems() {
	Instance().mergeSelectedItems();
}

void TexTool::texToolFlipS() {
	Instance().flipSelected(0);
}

void TexTool::texToolFlipT() {
	Instance().flipSelected(1);
}

void TexTool::selectRelated() {
	Instance().selectRelatedItems();
}

void TexTool::registerCommands() {
	GlobalEventManager().addCommand("TextureTool", FreeCaller<TexTool::toggle>());
	GlobalEventManager().addCommand("TexToolGridUp", FreeCaller<TexTool::texToolGridUp>());
	GlobalEventManager().addCommand("TexToolGridDown", FreeCaller<TexTool::texToolGridDown>());
	GlobalEventManager().addCommand("TexToolSnapToGrid", FreeCaller<TexTool::texToolSnapToGrid>());
	GlobalEventManager().addCommand("TexToolMergeItems", FreeCaller<TexTool::texToolMergeItems>());
	GlobalEventManager().addCommand("TexToolFlipS", FreeCaller<TexTool::texToolFlipS>());
	GlobalEventManager().addCommand("TexToolFlipT", FreeCaller<TexTool::texToolFlipT>());
	GlobalEventManager().addCommand("TexToolSelectRelated", FreeCaller<TexTool::selectRelated>());
	GlobalEventManager().addRegistryToggle("TexToolToggleGrid", RKEY_GRID_STATE);
}

} // namespace ui
