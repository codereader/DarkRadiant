#include "TexTool.h"

#include "i18n.h"
#include "ieventmanager.h"
#include "imainframe.h"
#include "igl.h"
#include "iundo.h"
#include "iuimanager.h"

#include "registry/adaptors.h"
#include "patch/PatchNode.h"
#include "texturelib.h"
#include "selectionlib.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/FramedWidget.h"
#include "gtkutil/GLWidgetSentry.h"
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

#include <gtkmm/toolbar.h>
#include <gtkmm/box.h>
#include <gdk/gdkkeysyms.h>

namespace ui {

	namespace {
		const char* const WINDOW_TITLE = N_("Texture Tool");

		const std::string RKEY_WINDOW_STATE = RKEY_TEXTOOL_ROOT + "window";
		const std::string RKEY_GRID_STATE = RKEY_TEXTOOL_ROOT + "gridActive";

		const float DEFAULT_ZOOM_FACTOR = 1.5f;
		const float ZOOM_MODIFIER = 1.25f;
		const float MOVE_FACTOR = 2.0f;

		const float GRID_MAX = 1.0f;
		const float GRID_DEFAULT = 0.0625f;
		const float GRID_MIN = 0.00390625f;
	}

TexTool::TexTool()
: gtkutil::PersistentTransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow(), true),
  _glWidget(Gtk::manage(new gtkutil::GLWidget(true, "TexTool"))),
  _selectionInfo(GlobalSelectionSystem().getSelectionInfo()),
  _zoomFactor(DEFAULT_ZOOM_FACTOR),
  _dragRectangle(false),
  _manipulatorMode(false),
  _viewOriginMove(false),
  _grid(GRID_DEFAULT),
  _gridActive(registry::getValue<bool>(RKEY_GRID_STATE))
{
	// Set the default border width in accordance to the HIG
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	signal_focus_in_event().connect(sigc::mem_fun(*this, &TexTool::triggerRedraw));
	signal_key_press_event().connect(sigc::mem_fun(*this, &TexTool::onKeyPress), false);

	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connect(this);

	populateWindow();

	// Connect the window position tracker
	_windowPosition.loadFromPath(RKEY_WINDOW_STATE);

	_windowPosition.connect(this);
	_windowPosition.applyPosition();

	// Register self to the SelSystem to get notified upon selection changes.
	GlobalSelectionSystem().addObserver(this);

    registry::observeBooleanKey(
        RKEY_GRID_STATE,
        sigc::bind(sigc::mem_fun(this, &TexTool::setGridActive), true),
        sigc::bind(sigc::mem_fun(this, &TexTool::setGridActive), false)
    );
}

TexToolPtr& TexTool::InstancePtr()
{
	static TexToolPtr _instancePtr;
	return _instancePtr;
}

void TexTool::setGridActive(bool active)
{
	_gridActive = active;
	draw();
}

void TexTool::populateWindow()
{
	// Create the GL widget
	Gtk::Frame* frame = Gtk::manage(new gtkutil::FramedWidget(*_glWidget));

	// Connect the events
	_glWidget->set_events(Gdk::KEY_PRESS_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK | Gdk::SCROLL_MASK);

	_glWidget->signal_expose_event().connect(sigc::mem_fun(*this, &TexTool::onExpose));
	_glWidget->signal_focus_in_event().connect(sigc::mem_fun(*this, &TexTool::triggerRedraw));
	_glWidget->signal_button_press_event().connect(sigc::mem_fun(*this, &TexTool::onMouseDown));
	_glWidget->signal_button_release_event().connect(sigc::mem_fun(*this, &TexTool::onMouseUp));
	_glWidget->signal_motion_notify_event().connect(sigc::mem_fun(*this, &TexTool::onMouseMotion));
	_glWidget->signal_key_press_event().connect(sigc::mem_fun(*this, &TexTool::onKeyPress), false);
	_glWidget->signal_scroll_event().connect(sigc::mem_fun(*this, &TexTool::onMouseScroll));

	// greebo: The "size-allocate" event is needed to determine the window size, as expose-event is
	// often called for subsets of the widget and the size info in there is therefore not reliable.
	_glWidget->signal_size_allocate().connect(sigc::mem_fun(*this, &TexTool::onSizeAllocate));

	// Make the GL widget accept the global shortcuts
	GlobalEventManager().connect(_glWidget);

	// Create a top-level vbox, pack it and add it to the window
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 0));

	// Load the texture toolbar from the registry
    IToolbarManager& tbCreator = GlobalUIManager().getToolbarManager();
	Gtk::Toolbar* textoolbar = tbCreator.getToolbar("textool");

	if (textoolbar != NULL)
	{
		textoolbar->unset_flags(Gtk::CAN_FOCUS);
    	vbox->pack_start(*textoolbar, false, false, 0);
    }

	vbox->pack_start(*frame, true, true, 0);

	add(*vbox);
}

// Pre-hide callback
void TexTool::_preHide() {
	// Save the window position, to make sure
	_windowPosition.readPosition();
}

// Pre-show callback
void TexTool::_preShow() {
	// Trigger an update of the current selection
	queueUpdate();
	// Restore the position
	_windowPosition.applyPosition();
}

void TexTool::_postShow()
{
#ifdef WIN32
	// Hack to prevent the textool renderview from going all grey, not receiving expose events
	_glWidget->reference();

	Gtk::Container* container = _glWidget->get_parent();
	container->remove(*_glWidget);
	container->add(*_glWidget);

	_glWidget->unreference();
#endif
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

void TexTool::onRadiantShutdown()
{
	// Release the shader
	_shader = MaterialPtr();

	// De-register this as selectionsystem observer
	GlobalSelectionSystem().removeObserver(this);

	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);

	GlobalEventManager().disconnect(_glWidget);
	GlobalEventManager().disconnect(this);

	// Destroy the window
	destroy();

	InstancePtr().reset();
}

TexTool& TexTool::Instance()
{
	TexToolPtr& instancePtr = InstancePtr();

	if (instancePtr == NULL)
	{
		// Not yet instantiated, do it now
		instancePtr.reset(new TexTool);

		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().signal_radiantShutdown().connect(
            sigc::mem_fun(*instancePtr, &TexTool::onRadiantShutdown)
        );
	}

	return *instancePtr;
}

void TexTool::update()
{
	std::string selectedShader = selection::algorithm::getShaderFromSelection();
	_shader = GlobalMaterialManager().getMaterialForName(selectedShader);

	// Clear the list to remove all the previously allocated items
	_items.clear();

	// Does the selection use one single shader?
	if (!_shader->getName().empty())
	{
		if (_selectionInfo.patchCount > 0) {
			// One single named shader, get the selection list
			PatchPtrVector patchList = selection::algorithm::getSelectedPatches();

			for (std::size_t i = 0; i < patchList.size(); i++) {
				// Allocate a new PatchItem on the heap (shared_ptr)
				textool::TexToolItemPtr patchItem(
					new textool::PatchItem(patchList[i]->getPatchInternal())
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

void TexTool::draw()
{
	// Redraw
	_glWidget->queue_draw();
}

void TexTool::onGtkIdle()
{
	update();
	draw();
}

void TexTool::queueUpdate()
{
	// Request a callback once the application is idle
	requestIdleCallback();
}

void TexTool::selectionChanged(const scene::INodePtr& node, bool isComponent)
{
	queueUpdate();
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
			_items[i]->getSelectableChildren(rectangle);

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

void TexTool::beginOperation()
{
	// Start an undo recording session
	GlobalUndoSystem().start();

	// Tell all the items to save their memento
	for (std::size_t i = 0; i < _items.size(); i++)
	{
		_items[i]->beginTransformation();
	}
}

void TexTool::endOperation(const std::string& commandName)
{
	for (std::size_t i = 0; i < _items.size(); i++)
	{
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

void TexTool::doMouseMove(const Vector2& coords, GdkEventMotion* event)
{
	if (_dragRectangle)
	{
		_selectionRectangle.bottomRight = coords;
		draw();
	}
	else if (_manipulatorMode)
	{
		Vector2 delta = coords - _manipulateRectangle.topLeft;

		// Snap the operations to the grid
		Vector3 snapped(0,0,0);

		if (_gridActive)
		{
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

		if (snapped.getLength() > 0)
		{
			// Create the transformation matrix
			Matrix4 transform = Matrix4::getTranslation(snapped);

			// Transform the selected
			// The transformSelected() call is propagated down the entire tree
			// of available items (e.g. PatchItem > PatchVertexItems)
			for (std::size_t i = 0; i < _items.size(); i++)
			{
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

	if (observerEvent == ui::obsManipulate)
	{
		// Get the list of selectables of this point
		textool::TexToolItemVec selectables = getSelectables(coords);

		// Any selectables under the mouse pointer?
		if (!selectables.empty())
		{
			// Activate the manipulator mode
			_manipulatorMode = true;
			_manipulateRectangle.topLeft = coords;
			_manipulateRectangle.bottomRight = coords;

			// Begin the undoable operation
			beginOperation();
		}
	}
	else if (observerEvent == ui::obsSelect || observerEvent == ui::obsToggle)
	{
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
		std::string ycoordStr = string::to_string(y) + ".0";
		GlobalOpenGL().drawString(ycoordStr);
	}

	for (int x = static_cast<int>(startX); x <= static_cast<int>(endX); x++) {
		glRasterPos2f(x + 0.05f, topLeft[1] + 0.03f * _zoomFactor);
		std::string xcoordStr = string::to_string(x) + ".0";
		GlobalOpenGL().drawString(xcoordStr);
	}
}

bool TexTool::onExpose(GdkEventExpose* ev)
{
	// Perform any pending updates
	flushIdleCallback();

	// Activate the GL widget
	gtkutil::GLWidgetSentry sentry(*_glWidget);

	// Initialise the viewport
	glViewport(0, 0, _windowDims[0], _windowDims[1]);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Clear the window with the specified background colour
	glClearColor(0.1f, 0.1f, 0.1f, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	// Do nothing, if the shader name is empty
	if (_shader == NULL || _shader->getName().empty())
	{
		return false;
	}

	AABB& selAABB = getExtents();

	// Is there a valid selection?
	if (!selAABB.isValid()) {
		return false;
	}

	AABB& texSpaceAABB = getVisibleTexSpace();

	// Get the upper left and lower right corner coordinates
	Vector3 orthoTopLeft = texSpaceAABB.origin - texSpaceAABB.extents * _zoomFactor;
	Vector3 orthoBottomRight = texSpaceAABB.origin + texSpaceAABB.extents * _zoomFactor;

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
	TexturePtr tex = _shader->getEditorImage();
	glBindTexture(GL_TEXTURE_2D, tex->getGLTexNum());

	// Draw the background texture
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);

	glTexCoord2d(orthoTopLeft[0], orthoTopLeft[1]);
	glVertex2d(orthoTopLeft[0], orthoTopLeft[1]);	// Upper left

	glTexCoord2d(orthoBottomRight[0], orthoTopLeft[1]);
	glVertex2d(orthoBottomRight[0], orthoTopLeft[1]);	// Upper right

	glTexCoord2d(orthoBottomRight[0], orthoBottomRight[1]);
	glVertex2d(orthoBottomRight[0], orthoBottomRight[1]);	// Lower right

	glTexCoord2d(orthoTopLeft[0], orthoBottomRight[1]);
	glVertex2d(orthoTopLeft[0], orthoBottomRight[1]);	// Lower left

	glEnd();
	glDisable(GL_TEXTURE_2D);

	// Draw the grid
	drawGrid();

	// Draw the u/v coordinates
	drawUVCoords();

	if (_dragRectangle) {
		// Create a working reference to save typing
		textool::Rectangle& rectangle = _selectionRectangle;

		// Define the blend function for transparency
		glEnable(GL_BLEND);
		glBlendColor(0,0,0, 0.2f);
		glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);

		glColor3f(0.8f, 0.8f, 1);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		// The transparent fill rectangle
		glBegin(GL_QUADS);
		glVertex2d(rectangle.topLeft[0], rectangle.topLeft[1]);
		glVertex2d(rectangle.bottomRight[0], rectangle.topLeft[1]);
		glVertex2d(rectangle.bottomRight[0], rectangle.bottomRight[1]);
		glVertex2d(rectangle.topLeft[0], rectangle.bottomRight[1]);
		glEnd();
		// The solid borders
		glBlendColor(0,0,0, 0.8f);
		glBegin(GL_LINE_LOOP);
		glVertex2d(rectangle.topLeft[0], rectangle.topLeft[1]);
		glVertex2d(rectangle.bottomRight[0], rectangle.topLeft[1]);
		glVertex2d(rectangle.bottomRight[0], rectangle.bottomRight[1]);
		glVertex2d(rectangle.topLeft[0], rectangle.bottomRight[1]);
		glEnd();
		glDisable(GL_BLEND);
	}

	return false;
}

void TexTool::onSizeAllocate(Gtk::Allocation& allocation)
{
	// Store the window dimensions for later calculations
	_windowDims = Vector2(allocation.get_width(), allocation.get_height());

	// Queue an expose event
	_glWidget->queue_draw();
}

bool TexTool::triggerRedraw(GdkEventFocus* ev)
{
	// Trigger a redraw
	_glWidget->queue_draw();
	return false;
}

bool TexTool::onMouseUp(GdkEventButton* ev)
{
	// Calculate the texture coords from the x/y click coordinates
	Vector2 texCoords = getTextureCoords(ev->x, ev->y);

	// Pass the call to the member method
	doMouseUp(texCoords, ev);

	// Check for view origin movements
	IMouseEvents& mouseEvents = GlobalEventManager().MouseEvents();

	if (mouseEvents.stateMatchesXYViewEvent(ui::xyMoveView, ev)) {
		_viewOriginMove = false;
	}

	return false;
}

bool TexTool::onMouseDown(GdkEventButton* ev)
{
	// Calculate the texture coords from the x/y click coordinates
	Vector2 texCoords = getTextureCoords(ev->x, ev->y);

	// Pass the call to the member method
	doMouseDown(texCoords, ev);

	// Check for view origin movements
	IMouseEvents& mouseEvents = GlobalEventManager().MouseEvents();

	if (mouseEvents.stateMatchesXYViewEvent(ui::xyMoveView, ev)) {
		_moveOriginRectangle.topLeft = Vector2(ev->x, ev->y);
		_viewOriginMove = true;
	}

	return false;
}

bool TexTool::onMouseMotion(GdkEventMotion* ev)
{
	// Calculate the texture coords from the x/y click coordinates
	Vector2 texCoords = getTextureCoords(ev->x, ev->y);

	// Pass the call to the member routine
	doMouseMove(texCoords, ev);

	// Check for view origin movements
	IMouseEvents& mouseEvents = GlobalEventManager().MouseEvents();

	if (mouseEvents.stateMatchesXYViewEvent(ui::xyMoveView, ev->state)) {

		// Calculate the movement delta relative to the old window x,y coords
		Vector2 delta = Vector2(ev->x, ev->y) - _moveOriginRectangle.topLeft;

		AABB& texSpaceAABB = getVisibleTexSpace();

		float speedFactor = _zoomFactor * MOVE_FACTOR;

		float factorX = texSpaceAABB.extents[0] / _windowDims[0] * speedFactor;
		float factorY = texSpaceAABB.extents[1] / _windowDims[1] * speedFactor;

		texSpaceAABB.origin[0] -= delta[0] * factorX;
		texSpaceAABB.origin[1] -= delta[1] * factorY;

		// Store the new coordinates
		_moveOriginRectangle.topLeft = Vector2(ev->x, ev->y);

		// Redraw to visualise the changes
		draw();
	}

	return false;
}

bool TexTool::onKeyPress(GdkEventKey* ev)
{
	// Check for ESC to deselect all items
	if (ev->keyval == GDK_Escape)
	{
		// Don't propage the keypress if the ESC could be processed
		// setAllSelected returns TRUE in that case
		return setAllSelected(false);
	}

	return false;
}

bool TexTool::onMouseScroll(GdkEventScroll* ev)
{
	if (ev->direction == GDK_SCROLL_UP) {
		_zoomFactor /= ZOOM_MODIFIER;

		draw();
	}
	else if (ev->direction == GDK_SCROLL_DOWN) {
		_zoomFactor *= ZOOM_MODIFIER;
		draw();
	}

	return false;
}

// Static command targets
void TexTool::toggle(const cmd::ArgumentList& args) {
	// Call the toggle() method of the static instance
	Instance().toggleVisibility();
}

void TexTool::texToolGridUp(const cmd::ArgumentList& args) {
	Instance().gridUp();
}

void TexTool::texToolGridDown(const cmd::ArgumentList& args) {
	Instance().gridDown();
}

void TexTool::texToolSnapToGrid(const cmd::ArgumentList& args) {
	Instance().snapToGrid();
}

void TexTool::texToolMergeItems(const cmd::ArgumentList& args) {
	Instance().mergeSelectedItems();
}

void TexTool::texToolFlipS(const cmd::ArgumentList& args) {
	Instance().flipSelected(0);
}

void TexTool::texToolFlipT(const cmd::ArgumentList& args) {
	Instance().flipSelected(1);
}

void TexTool::selectRelated(const cmd::ArgumentList& args) {
	Instance().selectRelatedItems();
}

void TexTool::registerCommands() {
	GlobalCommandSystem().addCommand("TextureTool", TexTool::toggle);
	GlobalCommandSystem().addCommand("TexToolGridUp", TexTool::texToolGridUp);
	GlobalCommandSystem().addCommand("TexToolGridDown", TexTool::texToolGridDown);
	GlobalCommandSystem().addCommand("TexToolSnapToGrid", TexTool::texToolSnapToGrid);
	GlobalCommandSystem().addCommand("TexToolMergeItems", TexTool::texToolMergeItems);
	GlobalCommandSystem().addCommand("TexToolFlipS", TexTool::texToolFlipS);
	GlobalCommandSystem().addCommand("TexToolFlipT", TexTool::texToolFlipT);
	GlobalCommandSystem().addCommand("TexToolSelectRelated", TexTool::selectRelated);

	GlobalEventManager().addCommand("TextureTool", "TextureTool");
	GlobalEventManager().addCommand("TexToolGridUp", "TexToolGridUp");
	GlobalEventManager().addCommand("TexToolGridDown", "TexToolGridDown");
	GlobalEventManager().addCommand("TexToolSnapToGrid", "TexToolSnapToGrid");
	GlobalEventManager().addCommand("TexToolMergeItems", "TexToolMergeItems");
	GlobalEventManager().addCommand("TexToolFlipS", "TexToolFlipS");
	GlobalEventManager().addCommand("TexToolFlipT", "TexToolFlipT");
	GlobalEventManager().addCommand("TexToolSelectRelated", "TexToolSelectRelated");
	GlobalEventManager().addRegistryToggle("TexToolToggleGrid", RKEY_GRID_STATE);
	GlobalEventManager().addRegistryToggle("TexToolToggleFaceVertexScalePivot", RKEY_FACE_VERTEX_SCALE_PIVOT_IS_CENTROID);
}

} // namespace ui
