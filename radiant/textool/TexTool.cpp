#include "TexTool.h"

#include "ieventmanager.h"
#include "iregistry.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <GL/glew.h>

#include "texturelib.h"
#include "selectionlib.h"
#include "gtkutil/TransientWindow.h"
#include "gtkutil/FramedWidget.h"
#include "gtkutil/glwidget.h"
#include "gtkutil/GLWidgetSentry.h"
#include "mainframe.h"
#include "brush/Face.h"
#include "patch/Patch.h"
#include "winding.h"
#include "camera/GlobalCamera.h"

#include "textool/Selectable.h"
#include "textool/Transformable.h"
#include "textool/PatchItem.h"
#include "textool/BrushItem.h"
#include "textool/FaceItem.h"

#include "selection/algorithm/Primitives.h"
#include "selection/algorithm/Shader.h"

namespace ui {
	
	namespace {
		const std::string WINDOW_TITLE = "Texture Tool";
		
		const std::string RKEY_ROOT = "user/ui/textures/texTool/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
		
		const float DEFAULT_ZOOM_FACTOR = 1.5f;
		const float ZOOM_MODIFIER = 1.25f;
		const float MOVE_FACTOR = 2.0f;
	}

TexTool::TexTool() :
	_selectionInfo(GlobalSelectionSystem().getSelectionInfo()),
	_zoomFactor(DEFAULT_ZOOM_FACTOR),
	_dragRectangle(false),
	_manipulatorMode(false),
	_viewOriginMove(false),
	_undoCommand(NULL)
{
	// Be sure to pass FALSE to the TransientWindow to prevent it from self-destruction
	_window = gtkutil::TransientWindow(WINDOW_TITLE, MainFrame_getWindow(), false);
	
	gtk_window_set_type_hint(GTK_WINDOW(_window), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	g_signal_connect(G_OBJECT(_window), "delete-event", G_CALLBACK(onDelete), this);
	g_signal_connect(G_OBJECT(_window), "focus-in-event", G_CALLBACK(triggerRedraw), this);
	g_signal_connect(G_OBJECT(_window), "key_press_event", G_CALLBACK(onKeyPress), this);
	
	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connect(GTK_OBJECT(_window));
	
	populateWindow();
	
	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(_window));
	
	// Register self to the SelSystem to get notified upon selection changes.
	GlobalSelectionSystem().addObserver(this);
}

void TexTool::populateWindow() {
	// Create the GL widget
	_glWidget = glwidget_new(TRUE);
	GtkWidget* frame = gtkutil::FramedWidget(_glWidget);
		
	// Connect the events
	gtk_widget_set_events(_glWidget, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);
	g_signal_connect(G_OBJECT(_glWidget), "expose-event", G_CALLBACK(onExpose), this);
	g_signal_connect(G_OBJECT(_glWidget), "focus-in-event", G_CALLBACK(triggerRedraw), this);
	g_signal_connect(G_OBJECT(_glWidget), "button-press-event", G_CALLBACK(onMouseDown), this);
	g_signal_connect(G_OBJECT(_glWidget), "button-release-event", G_CALLBACK(onMouseUp), this);
	g_signal_connect(G_OBJECT(_glWidget), "motion-notify-event", G_CALLBACK(onMouseMotion), this);
	g_signal_connect(G_OBJECT(_glWidget), "key_press_event", G_CALLBACK(onKeyPress), this);
	g_signal_connect(G_OBJECT(_glWidget), "scroll_event", G_CALLBACK(onMouseScroll), this);
	
	// Make the GL widget accept the global shortcuts
	GlobalEventManager().connect(GTK_OBJECT(_glWidget));
	
	GtkWidget* vbox = gtk_vbox_new(true, 0);
	gtk_box_pack_start(GTK_BOX(vbox), frame, true, true, 0);
	
	gtk_container_add(GTK_CONTAINER(_window), vbox);
}

void TexTool::toggle() {
	// Pass the call to the utility methods that save/restore the window position
	if (GTK_WIDGET_VISIBLE(_window)) {
		gtkutil::TransientWindow::minimise(_window);
		gtk_widget_hide_all(_window);
	}
	else {
		// Simulate a selection change to trigger an update
		selectionChanged();
		// First restore the window
		gtkutil::TransientWindow::restore(_window);
		// then apply the saved position
		_windowPosition.applyPosition();
		// Now show it
		gtk_widget_show_all(_window);
	}
}

void TexTool::shutdown() {
	// De-register this as selectionsystem observer
	GlobalSelectionSystem().removeObserver(this);
	
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
	
	GlobalEventManager().disconnect(GTK_OBJECT(_glWidget));
	GlobalEventManager().disconnect(GTK_OBJECT(_window));
}

TexTool& TexTool::Instance() {
	static TexTool _instance;
	
	return _instance;
}

void TexTool::update() {
	std::string selectedShader = selection::algorithm::getShaderFromSelection();
	_shader = GlobalShaderSystem().getShaderForName(selectedShader);
}

void TexTool::selectionChanged() {
	update();
	
	// Clear the list to remove all the previously allocated items
	_items.clear();
	
	// Does the selection use one single shader?
	if (_shader->getName() != "") {
		if (_selectionInfo.patchCount > 0) {
			// One single named shader, get the selection list
			PatchPtrVector patchList = selection::algorithm::getSelectedPatches();
			
			for (std::size_t i = 0; i < patchList.size(); i++) {
				// Allocate a new PatchItem on the heap (shared_ptr)
				selection::textool::TexToolItemPtr patchItem(
					new selection::textool::PatchItem(*patchList[i])
				);
				
				// Add it to the list
				_items.push_back(patchItem);
			}
		}
		
		if (_selectionInfo.brushCount > 0) {
			BrushPtrVector brushList = selection::algorithm::getSelectedBrushes();
			
			for (std::size_t i = 0; i < brushList.size(); i++) {
				// Allocate a new BrushItem on the heap (shared_ptr)
				selection::textool::TexToolItemPtr brushItem(
					new selection::textool::BrushItem(*brushList[i])
				);
				
				// Add it to the list
				_items.push_back(brushItem);
			}
		}
		
		// Get the single selected faces
		FacePtrVector faceList = selection::algorithm::getSelectedFaces();
		
		for (std::size_t i = 0; i < faceList.size(); i++) {
			// Allocate a new FaceItem on the heap (shared_ptr)
			selection::textool::TexToolItemPtr faceItem(
				new selection::textool::FaceItem(*faceList[i])
			);
			
			// Add it to the list
			_items.push_back(faceItem);
		}
	}
	
	recalculateVisibleTexSpace();
	
	draw();
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
	
	for (unsigned int i = 0; i < _items.size(); i++) {
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
	for (unsigned int i = 0; i < _items.size(); i++) {
		_items[i]->render();
	}
}

selection::textool::TexToolItemVec 
	TexTool::getSelectables(const selection::Rectangle& rectangle)
{
	selection::textool::TexToolItemVec selectables;
	
	// Cycle through all the toplevel items and test them for selectability
	for (unsigned int i = 0; i < _items.size(); i++) {
		if (_items[i]->testSelect(rectangle)) {
			selectables.push_back(_items[i]);
		}
	}
	
	// Cycle through all the items and ask them to deliver the list of child selectables
	// residing within the test rectangle
	for (unsigned int i = 0; i < _items.size(); i++) {
		// Get the list from each item
		selection::textool::TexToolItemVec found = 
			_items[i]->getSelectableChilds(rectangle);
		
		// and append the vector to the existing vector
		selectables.insert(selectables.end(), found.begin(), found.end());
	}
	
	return selectables;
}

selection::textool::TexToolItemVec TexTool::getSelectables(const Vector2& coords) {
	// Construct a test rectangle with 2% of the width/height
	// of the visible texture space
	selection::Rectangle testRectangle;
	
	Vector3 extents = getVisibleTexSpace().extents * _zoomFactor;
	
	testRectangle.topLeft[0] = coords[0] - extents[0]*0.02; 
	testRectangle.topLeft[1] = coords[1] - extents[1]*0.02;
	testRectangle.bottomRight[0] = coords[0] + extents[0]*0.02; 
	testRectangle.bottomRight[1] = coords[1] + extents[1]*0.02;
	
	// Pass the call on to the getSelectables(<RECTANGLE>) method
	return getSelectables(testRectangle);
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
		
/*		if (_undoCommand != NULL) {
			globalOutputStream() << "Deleting undo...\n";
			// Remove the undo command from the heap, this triggers the 
			// undo state save.
			delete _undoCommand;
			_undoCommand = NULL;
		}*/
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
		
		selection::textool::TexToolItemVec selectables;
		
		if ((coords - _selectionRectangle.topLeft).getLength() < minDist) {
			// Perform a point selection test
			selectables = getSelectables(_selectionRectangle.topLeft);
		}
		else {
			// Perform the regular selectiontest
			selectables = getSelectables(_selectionRectangle);
		}
		
		// Toggle the selection
		for (unsigned int i = 0; i < selectables.size(); i++) {
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
		
		Vector3 translation(delta[0], delta[1], 0);
		
		// Create the transformation matrix
		Matrix4 transform = Matrix4::getTranslation(translation);
		
		// Transform the selected
		// The transformSelected() call is propagated down the entire tree
		// of available items (e.g. PatchItem > PatchVertexItems)
		for (unsigned int i = 0; i < _items.size(); i++) {
			_items[i]->transformSelected(transform);
		}
		
		// Store the new coords as new starting point
		_manipulateRectangle.topLeft = coords;
		
		draw();
		
		// Update the camera to reflect the changes
		GlobalCamera().update();
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
		selection::textool::TexToolItemVec selectables;
		selectables = getSelectables(coords);
		
		// Any selectables under the mouse pointer?
		if (selectables.size() > 0) {
			// Toggle the selection
			/*for (unsigned int i = 0; i < selectables.size(); i++) {
				selectables[i]->setSelected(true);
			}*/
			
			// Activate the manipulator mode
			_manipulatorMode = true;
			_manipulateRectangle.topLeft = coords; 
			_manipulateRectangle.bottomRight = coords;
			
			/*if (_undoCommand == NULL) {
				globalOutputStream() << "Creating undo...\n";
				_undoCommand = new UndoableCommand("textureUVEdit");
			}*/
			
			/*// Prepare the items for the upcoming transformation (undoSave(), etc.)
			for (unsigned int i = 0; i < selectables.size(); i++) {
				selectables[i]->beginTransformation();
			}*/
		}
	}
	else if (observerEvent == ui::obsSelect || observerEvent == ui::obsToggle) {
		// Start a drag or click operation
		_dragRectangle = true;
		_selectionRectangle.topLeft = coords;
		_selectionRectangle.bottomRight = coords;
	}
}

void TexTool::foreachItem(selection::textool::ItemVisitor& visitor) {
	for (unsigned int i = 0; i < _items.size(); i++) {
		// Visit the class
		visitor.visit(_items[i]);
		
		// Now propagate the visitor down the hierarchy
		_items[i]->foreachItem(visitor);
	}
}

void TexTool::drawGrid() {
	AABB& texSpaceAABB = getVisibleTexSpace();
	
	glEnable(GL_LINE_STIPPLE);
	glLineStipple(10, 0x5555);
	glBegin(GL_LINES);
	glVertex2f(0, texSpaceAABB.origin[1] - texSpaceAABB.extents[1] * _zoomFactor);
	glVertex2f(0, texSpaceAABB.origin[1] + texSpaceAABB.extents[1] * _zoomFactor);
	
	glVertex2f(texSpaceAABB.origin[0] - texSpaceAABB.extents[0] * _zoomFactor, 0);
	glVertex2f(texSpaceAABB.origin[0] + texSpaceAABB.extents[0] * _zoomFactor, 0);
	glEnd();
	glDisable(GL_LINE_STIPPLE);
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
	
	// Draw the u/v coordinates
	self->drawUVCoords();
	
	if (self->_dragRectangle) {
		// Create a working reference to save typing
		selection::Rectangle& rectangle = self->_selectionRectangle;
		
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
	
	self->drawGrid();
	/*glColor3f(1, 1, 1);
	std::string topLeftStr = floatToStr(orthoTopLeft[0]) + "," + floatToStr(orthoTopLeft[1]);
	std::string bottomRightStr = floatToStr(orthoBottomRight[0]) + "," + floatToStr(orthoBottomRight[1]);
	
	glRasterPos2f(orthoTopLeft[0] + 0.5, orthoTopLeft[1] + 0.5);
	GlobalOpenGL().drawString(topLeftStr.c_str());
	
	glRasterPos2f(orthoBottomRight[0] - 0.2, orthoBottomRight[1] + 0.2);
	GlobalOpenGL().drawString(bottomRightStr.c_str());*/
	
	return false;
}

gboolean TexTool::triggerRedraw(GtkWidget* widget, GdkEventFocus* event, TexTool* self) {
	// Trigger a redraw
	self->draw();
	return false;
}

gboolean TexTool::onDelete(GtkWidget* widget, GdkEvent* event, TexTool* self) {
	// Toggle the visibility of the textool window
	self->toggle();
	
	// Don't propagate the delete event
	return true;
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
		// Clear the selection using a visitor class
		selection::textool::SetSelectedWalker visitor(false);
		self->foreachItem(visitor);
		
		// Redraw to visualise the changes
		self->draw();
		
		// Don't propage the keypress event any further
		return true;
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

} // namespace ui
