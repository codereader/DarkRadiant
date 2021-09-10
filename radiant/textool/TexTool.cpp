#include "TexTool.h"

#include "i18n.h"
#include "ieventmanager.h"
#include "imainframe.h"
#include "igl.h"
#include "iundo.h"
#include "ibrush.h"
#include "ipatch.h"
#include "itoolbarmanager.h"
#include "itextstream.h"

#include "registry/adaptors.h"
#include "texturelib.h"
#include "selectionlib.h"
#include "camera/CameraWndManager.h"
#include "wxutil/GLWidget.h"
#include "selection/Device.h"
#include "selection/SelectionVolume.h"
#include "Rectangle.h"

#include "textool/Selectable.h"
#include "textool/Transformable.h"
#include "textool/item/PatchItem.h"
#include "textool/item/BrushItem.h"
#include "textool/item/FaceItem.h"
#include "textool/tools/TextureToolMouseEvent.h"

#include <wx/sizer.h>
#include <wx/toolbar.h>

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Texture Tool");

	const std::string RKEY_WINDOW_STATE = RKEY_TEXTOOL_ROOT + "window";
	const std::string RKEY_GRID_STATE = RKEY_TEXTOOL_ROOT + "gridActive";
    const char* const RKEY_SELECT_EPSILON = "user/ui/selectionEpsilon";

	const float ZOOM_MODIFIER = 1.25f;

	const float GRID_MAX = 1.0f;
	const float GRID_DEFAULT = 0.0625f;
	const float GRID_MIN = 0.00390625f;
}

TexTool::TexTool() : 
    TransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getWxTopLevelWindow(), true),
    MouseToolHandler(IMouseToolGroup::Type::TextureTool),
    _glWidget(new wxutil::GLWidget(this, std::bind(&TexTool::onGLDraw, this), "TexTool")),
    _selectionInfo(GlobalSelectionSystem().getSelectionInfo()),
    _dragRectangle(false),
    _manipulatorMode(false),
    _grid(GRID_DEFAULT),
    _gridActive(registry::getValue<bool>(RKEY_GRID_STATE)),
    _updateNeeded(false)
{
	Bind(wxEVT_IDLE, &TexTool::onIdle, this);
    Bind(wxEVT_KEY_DOWN, &TexTool::onKeyPress, this);

	populateWindow();

	InitialiseWindowPosition(600, 400, RKEY_WINDOW_STATE);

    registry::observeBooleanKey(
        RKEY_GRID_STATE,
        sigc::bind(sigc::mem_fun(this, &TexTool::setGridActive), true),
        sigc::bind(sigc::mem_fun(this, &TexTool::setGridActive), false)
    );

    _freezePointer.connectMouseEvents(
        std::bind(&TexTool::onMouseDown, this, std::placeholders::_1),
        std::bind(&TexTool::onMouseUp, this, std::placeholders::_1));
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
	// Connect all relevant events
	_glWidget->Bind(wxEVT_SIZE, &TexTool::onGLResize, this);
	_glWidget->Bind(wxEVT_MOUSEWHEEL, &TexTool::onMouseScroll, this);
	_glWidget->Bind(wxEVT_MOTION, &TexTool::onMouseMotion, this);

	_glWidget->Bind(wxEVT_LEFT_DOWN, &TexTool::onMouseDown, this);
    _glWidget->Bind(wxEVT_LEFT_DCLICK, &TexTool::onMouseDown, this);
	_glWidget->Bind(wxEVT_LEFT_UP, &TexTool::onMouseUp, this);
	_glWidget->Bind(wxEVT_RIGHT_DOWN, &TexTool::onMouseDown, this);
    _glWidget->Bind(wxEVT_RIGHT_DCLICK, &TexTool::onMouseDown, this);
	_glWidget->Bind(wxEVT_RIGHT_UP, &TexTool::onMouseUp, this);
	_glWidget->Bind(wxEVT_MIDDLE_DOWN, &TexTool::onMouseDown, this);
    _glWidget->Bind(wxEVT_MIDDLE_DCLICK, &TexTool::onMouseDown, this);
	_glWidget->Bind(wxEVT_MIDDLE_UP, &TexTool::onMouseUp, this);

	// Connect our own key handler afterwards to receive events before the event manager
	_glWidget->Bind(wxEVT_KEY_DOWN, &TexTool::onKeyPress, this);

	SetSizer(new wxBoxSizer(wxVERTICAL));

	// Load the texture toolbar from the registry
	auto* textoolbar = GlobalToolBarManager().createToolbar("textool", this);

	if (textoolbar != nullptr)
	{
		textoolbar->SetCanFocus(false);
		GetSizer()->Add(textoolbar, 0, wxEXPAND);
    }

	GetSizer()->Add(_glWidget, 1, wxEXPAND);
}

// Pre-hide callback
void TexTool::_preHide()
{
	TransientWindow::_preHide();

	_undoHandler.disconnect();
	_redoHandler.disconnect();

	_selectionChanged.disconnect();

	// Clear items to prevent us from running into stale references
	// when the textool is shown again
	_items.clear();
}

// Pre-show callback
void TexTool::_preShow()
{
	TransientWindow::_preShow();

	_selectionChanged.disconnect();
	_undoHandler.disconnect();
	_redoHandler.disconnect();

	// Register self to the SelSystem to get notified upon selection changes.
	_selectionChanged = GlobalSelectionSystem().signal_selectionChanged().connect(
		[this](const ISelectable&) { queueDraw(); });

	_undoHandler = GlobalUndoSystem().signal_postUndo().connect(
		sigc::mem_fun(this, &TexTool::onUndoRedoOperation));
	_redoHandler = GlobalUndoSystem().signal_postRedo().connect(
		sigc::mem_fun(this, &TexTool::onUndoRedoOperation));

	// Trigger an update of the current selection
    queueDraw();
}

void TexTool::onUndoRedoOperation()
{
    queueDraw();
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

void TexTool::onMainFrameShuttingDown()
{
	rMessage() << "TexTool shutting down." << std::endl;

	// Release the shader
	_shader = MaterialPtr();

	if (IsShownOnScreen())
	{
		Hide();
	}

	// Destroy the window
	SendDestroyEvent();
	InstancePtr().reset();
}

TexTool& TexTool::Instance()
{
	TexToolPtr& instancePtr = InstancePtr();

	if (instancePtr == NULL)
	{
		// Not yet instantiated, do it now
		instancePtr.reset(new TexTool);

		// Wire up the pre-destruction cleanup code
		GlobalMainFrame().signal_MainFrameShuttingDown().connect(
            sigc::mem_fun(*instancePtr, &TexTool::onMainFrameShuttingDown)
        );
	}

	return *instancePtr;
}

void TexTool::update()
{
	std::string selectedShader = selection::getShaderFromSelection();
	_shader = GlobalMaterialManager().getMaterial(selectedShader);

	// Clear the list to remove all the previously allocated items
	_items.clear();

	// Does the selection use one single shader?
	if (!_shader->getName().empty())
	{
		if (GlobalSelectionSystem().countSelectedComponents() > 0)
		{
			// Check each selected face
			GlobalSelectionSystem().foreachFace([&](IFace& face)
			{
				// Allocate a new FaceItem
				_items.emplace_back(new textool::FaceItem(face));
			});
		}
		else
		{
			GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
			{
				if (Node_isBrush(node))
				{
					_items.emplace_back(new textool::BrushItem(*Node_getIBrush(node)));
				}
				else if (Node_isPatch(node))
				{
					_items.emplace_back(new textool::PatchItem(*Node_getIPatch(node)));
				}
			});
		}
	}

	recalculateVisibleTexSpace();
}

SelectionTestPtr TexTool::createSelectionTestForPoint(const Vector2& point)
{
    float selectEpsilon = registry::getValue<float>(RKEY_SELECT_EPSILON);

    // Generate the epsilon
    Vector2 deviceEpsilon(selectEpsilon / _windowDims[0], selectEpsilon / _windowDims[1]);

    // Copy the current view and constrain it to a small rectangle
    render::View scissored(_view);
    ConstructSelectionTest(scissored, selection::Rectangle::ConstructFromPoint(point, deviceEpsilon));

    return SelectionTestPtr(new SelectionVolume(scissored));
}

int TexTool::getDeviceWidth() const
{
    return static_cast<int>(_windowDims[0]);
}

int TexTool::getDeviceHeight() const
{
    return static_cast<int>(_windowDims[1]);
}

const VolumeTest& TexTool::getVolumeTest() const
{
    return _view;
}

void TexTool::forceRedraw()
{
    draw();
}

void TexTool::draw()
{
	// Redraw
	_glWidget->Refresh();
}

void TexTool::onIdle(wxIdleEvent& ev)
{
	if (_updateNeeded)
	{
		_updateNeeded = false;

		update();
		draw();
	}
}

void TexTool::queueDraw()
{
	_updateNeeded = true;
}

void TexTool::scrollByPixels(int x, int y)
{
    auto& texSpaceAABB = getVisibleTexSpace();

    auto uPerPixel = texSpaceAABB.extents[0] * 2 / _windowDims[0];
    auto vPerPixel = texSpaceAABB.extents[1] * 2 / _windowDims[1];
    
    texSpaceAABB.origin[0] -= x * uPerPixel;
    texSpaceAABB.origin[1] -= y * vPerPixel;

    updateProjection();
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

void TexTool::recalculateVisibleTexSpace()
{
	// Get the selection extents
	_texSpaceAABB = getExtents();
    _texSpaceAABB.extents *= 1.5; // add some padding around the selection

	// Normalise the plane to be square
	_texSpaceAABB.extents[0] = std::max(_texSpaceAABB.extents[0], _texSpaceAABB.extents[1]);
	_texSpaceAABB.extents[1] = std::max(_texSpaceAABB.extents[0], _texSpaceAABB.extents[1]);

    // Make the visible space non-uniform if the texture has a width/height ratio != 1
    double aspect = getTextureAspectRatio();

    if (aspect < 1)
    {
        _texSpaceAABB.extents.x() /= getTextureAspectRatio();
    }
    else
    {
        _texSpaceAABB.extents.y() *= getTextureAspectRatio();
    }

    updateProjection();
}

AABB& TexTool::getExtents()
{
	_selAABB = AABB();

	for (const auto& item : _items)
    {
		// Expand the selection AABB by the extents of the item
		_selAABB.includeAABB(item->getExtents());
	}

	return _selAABB;
}

AABB& TexTool::getVisibleTexSpace()
{
	return _texSpaceAABB;
}

Vector2 TexTool::getTextureCoords(const double& x, const double& y) {
	Vector2 result;

	if (_selAABB.isValid()) {
		Vector3 aabbTL = _texSpaceAABB.origin - _texSpaceAABB.extents;
		Vector3 aabbBR = _texSpaceAABB.origin + _texSpaceAABB.extents;

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

	Vector3 extents = getVisibleTexSpace().extents;

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

void TexTool::doMouseUp(const Vector2& coords, wxMouseEvent& event)
{
	// If we are in manipulation mode, end the move
    if (event.LeftUp() && !event.HasAnyModifiers() && _manipulatorMode)
    {
		_manipulatorMode = false;

		// Finish the undo recording, store the accumulated undomementos
		endOperation("TexToolDrag");
	}

	// If we are in selection mode, end the selection
    if ((event.LeftUp() && event.ShiftDown())
		 && _dragRectangle)
	{
		_dragRectangle = false;

		// Make sure the corners are in the correct order
		_selectionRectangle.sortCorners();

		// The minimim rectangle diameter for a rectangle test (3 % of visible texspace)
		float minDist = _texSpaceAABB.extents[0] * 0.03;

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

void TexTool::doMouseMove(const Vector2& coords, wxMouseEvent& event)
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

void TexTool::doMouseDown(const Vector2& coords, wxMouseEvent& event)
{
	_manipulatorMode = false;
	_dragRectangle = false;

	if (event.LeftDown() && !event.HasAnyModifiers())
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
    else if (event.LeftDown() && event.ShiftDown())
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

void TexTool::drawGrid()
{
	const float MAX_NUMBER_OF_GRID_LINES = 1024;

	AABB& texSpaceAABB = getVisibleTexSpace();

	Vector3 topLeft = texSpaceAABB.origin - texSpaceAABB.extents;
	Vector3 bottomRight = texSpaceAABB.origin + texSpaceAABB.extents;

	if (topLeft[0] > bottomRight[0])
	{
		std::swap(topLeft[0], bottomRight[0]);
	}

	if (topLeft[1] > bottomRight[1])
	{
		std::swap(topLeft[1], bottomRight[1]);
	}

	float startX = floor(topLeft[0]) - 1;
	float endX = ceil(bottomRight[0]) + 1;
	float startY = floor(topLeft[1]) - 1;
	float endY = ceil(bottomRight[1]) + 1;

	if (startX >= endX || startY >= endY)
	{
		return;
	}

	glBegin(GL_LINES);

	// To avoid drawing millions of grid lines in a window, scale up the grid interval
	// such that only a maximum number of lines are drawn
	float yIntStep = 1.0f;
	while (abs(endY - startY) / yIntStep > MAX_NUMBER_OF_GRID_LINES)
	{
		yIntStep *= 2;
	}

	float xIntStep = 1.0f;
	while (abs(endX - startX) / xIntStep > MAX_NUMBER_OF_GRID_LINES)
	{
		xIntStep *= 2;
	}

	// Draw the integer grid
	glColor4f(0.4f, 0.4f, 0.4f, 0.4f);

	for (float y = startY; y <= endY; y += yIntStep)
	{
		glVertex2f(startX, y);
		glVertex2f(endX, y);
	}

	for (float x = startX; x <= endX; x += xIntStep)
	{
		glVertex2f(x, startY);
		glVertex2f(x, endY);
	}

	if (_gridActive)
	{
		// Draw the manipulation grid
		glColor4f(0.2f, 0.2f, 0.2f, 0.4f);

		float grid = _grid;

		// scale up the grid interval such that only a maximum number of lines are drawn
		while (abs(endX - startX) / grid > MAX_NUMBER_OF_GRID_LINES ||
			   abs(endY - startY) / grid > MAX_NUMBER_OF_GRID_LINES)
		{
			grid *= 2;
		}

		for (float y = startY; y <= endY; y += grid)
		{
			glVertex2f(startX, y);
			glVertex2f(endX, y);
		}

		for (float x = startX; x <= endX; x += grid)
		{
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
	for (float y = startY; y <= endY; y += yIntStep)
	{
		glRasterPos2f(topLeft[0] + 0.05f, y + 0.05f);
		std::string ycoordStr = string::to_string(trunc(y)) + ".0";
		GlobalOpenGL().drawString(ycoordStr);
	}

	for (float x = startX; x <= endX; x += xIntStep)
	{
		glRasterPos2f(x + 0.05f, topLeft[1] + 0.03f);
		std::string xcoordStr = string::to_string(trunc(x)) + ".0";
		GlobalOpenGL().drawString(xcoordStr);
	}
}

double TexTool::getTextureAspectRatio()
{
    if (!_shader) return 1;
    
    auto editorImage = _shader->getEditorImage();
    if (!editorImage) return 1;

    return static_cast<double>(editorImage->getWidth()) / editorImage->getHeight();
}

void TexTool::updateProjection()
{
    double windowAspect = _windowDims[0] / _windowDims[1];

    // Set up the orthographic projection matrix to [b,l..t,r] => [-1,-1..+1,+1]
    // with b,l,t,r set to values centered at origin
    double left = -_texSpaceAABB.extents.x();
    double right = _texSpaceAABB.extents.x();

    double top = _texSpaceAABB.extents.y() / windowAspect;
    double bottom = -_texSpaceAABB.extents.y() / windowAspect;

    auto rMinusL = right - left;
    auto rPlusL = right + left;

    auto tMinusB = top - bottom;
    auto tPlusB = top + bottom;

    auto projection = Matrix4::getIdentity();

    projection[0] = 2.0f / rMinusL;
    projection[5] = 2.0f / tMinusB;
    projection[10] = 1;

    projection[12] = rPlusL / rMinusL;
    projection[13] = tPlusB / tMinusB;
    projection[14] = 0;

    auto modelView = Matrix4::getIdentity();

    // We have to invert the Y axis to have the negative tex coords in the upper quadrants
    modelView.xx() = 1;
    modelView.yy() = -1;

    // Shift the visible UV space such that it is centered around origin before projecting it
    modelView.tx() = -_texSpaceAABB.origin.x();
    modelView.ty() = _texSpaceAABB.origin.y();

    _view.construct(projection, modelView, 
            static_cast<std::size_t>(_windowDims[0]), static_cast<std::size_t>(_windowDims[1]));
}

bool TexTool::onGLDraw()
{
	if (_updateNeeded)
	{
		return false; // stop here, wait for the next idle event to refresh
	}

	// Initialise the viewport
	glViewport(0, 0, _windowDims[0], _windowDims[1]);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	// Clear the window with the specified background colour
	glClearColor(0.1f, 0.1f, 0.1f, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	// Do nothing, if the shader name is empty
	if (_shader == NULL || _shader->getName().empty())
	{
		return true;
	}

	AABB& selAABB = getExtents();

	// Is there a valid selection?
	if (!selAABB.isValid())
	{
		return true;
	}

	auto& texSpaceAABB = getVisibleTexSpace();

	// Get the upper left and lower right corner coordinates
	auto orthoTopLeft = texSpaceAABB.origin - texSpaceAABB.extents;
	auto orthoBottomRight = texSpaceAABB.origin + texSpaceAABB.extents;

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(_view.GetProjection());
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd(_view.GetModelview());

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

	return true;
}

void TexTool::onGLResize(wxSizeEvent& ev)
{
	// Store the window dimensions for later calculations
	_windowDims = Vector2(ev.GetSize().GetWidth(), ev.GetSize().GetHeight());

    updateProjection();

	// Queue an expose event
	_glWidget->Refresh();
}

void TexTool::onMouseUp(wxMouseEvent& ev)
{
    // Regular mouse tool processing
    MouseToolHandler::onGLMouseButtonRelease(ev);

	// Calculate the texture coords from the x/y click coordinates
	Vector2 texCoords = getTextureCoords(ev.GetX(), ev.GetY());

	// Pass the call to the member method
	doMouseUp(texCoords, ev);

	ev.Skip();
}

void TexTool::onMouseDown(wxMouseEvent& ev)
{
    // Send the event to the mouse tool handler
    MouseToolHandler::onGLMouseButtonPress(ev);

	// Calculate the texture coords from the x/y click coordinates
	Vector2 texCoords = getTextureCoords(ev.GetX(), ev.GetY());

	// Pass the call to the member method
	doMouseDown(texCoords, ev);

	ev.Skip();
}

void TexTool::onMouseMotion(wxMouseEvent& ev)
{
    MouseToolHandler::onGLMouseMove(ev);

	// Calculate the texture coords from the x/y click coordinates
	Vector2 texCoords = getTextureCoords(ev.GetX(), ev.GetY());

	// Pass the call to the member routine
	doMouseMove(texCoords, ev);

	ev.Skip();
}

void TexTool::onKeyPress(wxKeyEvent& ev)
{
	// Check for ESC to deselect all items
	if (ev.GetKeyCode() == WXK_ESCAPE)
	{
		// Don't propage the keypress if the ESC could be processed
		// setAllSelected returns TRUE in that case
		if (setAllSelected(false))
		{
			return; // without skip
		}
	}

	ev.Skip();
}

void TexTool::onMouseScroll(wxMouseEvent& ev)
{
    double factor = ev.GetWheelRotation() > 0 ? 1 / ZOOM_MODIFIER : ZOOM_MODIFIER;
    _texSpaceAABB.extents *= factor;

    updateProjection();
    draw();
}

// Static command targets
void TexTool::toggle(const cmd::ArgumentList& args)
{
	// Call the toggle() method of the static instance
	Instance().ToggleVisibility();
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

void TexTool::registerCommands()
{
	GlobalCommandSystem().addCommand("TextureTool", TexTool::toggle);
	GlobalCommandSystem().addCommand("TexToolGridUp", TexTool::texToolGridUp);
	GlobalCommandSystem().addCommand("TexToolGridDown", TexTool::texToolGridDown);
	GlobalCommandSystem().addCommand("TexToolSnapToGrid", TexTool::texToolSnapToGrid);
	GlobalCommandSystem().addCommand("TexToolMergeItems", TexTool::texToolMergeItems);
	GlobalCommandSystem().addCommand("TexToolFlipS", TexTool::texToolFlipS);
	GlobalCommandSystem().addCommand("TexToolFlipT", TexTool::texToolFlipT);
	GlobalCommandSystem().addCommand("TexToolSelectRelated", TexTool::selectRelated);

	GlobalEventManager().addRegistryToggle("TexToolToggleGrid", RKEY_GRID_STATE);
	GlobalEventManager().addRegistryToggle("TexToolToggleFaceVertexScalePivot", RKEY_FACE_VERTEX_SCALE_PIVOT_IS_CENTROID);
}

MouseTool::Result TexTool::processMouseDownEvent(const MouseToolPtr& tool, const Vector2& point)
{
    TextureToolMouseEvent ev = createMouseEvent(point);
    return tool->onMouseDown(ev);
}

MouseTool::Result TexTool::processMouseUpEvent(const MouseToolPtr& tool, const Vector2& point)
{
    TextureToolMouseEvent ev = createMouseEvent(point);
    return tool->onMouseUp(ev);
}

MouseTool::Result TexTool::processMouseMoveEvent(const MouseToolPtr& tool, int x, int y)
{
    bool mouseToolReceivesDeltas = (tool->getPointerMode() & MouseTool::PointerMode::MotionDeltas) != 0;

    // New MouseTool event, optionally passing the delta only
    TextureToolMouseEvent ev = mouseToolReceivesDeltas ?
        createMouseEvent(Vector2(0, 0), Vector2(x, y)) :
        createMouseEvent(Vector2(x, y));

    return tool->onMouseMove(ev);
}

void TexTool::handleGLCapturedMouseMotion(const MouseToolPtr& tool, int x, int y, unsigned int mouseState)
{
    if (!tool) return;

#if 0
    bool mouseToolReceivesDeltas = (tool->getPointerMode() & MouseTool::PointerMode::MotionDeltas) != 0;
#endif
    bool pointerFrozen = (tool->getPointerMode() & MouseTool::PointerMode::Freeze) != 0;

    // Check if the mouse has reached exceeded the window borders for chase mouse behaviour
    // In FreezePointer mode there's no need to check for chase since the cursor is fixed anyway
    if (tool->allowChaseMouse() && !pointerFrozen /* && checkChaseMouse(mouseState)*/)
    {
        // Chase mouse activated, an idle callback will kick in soon
        return;
    }

    // Send mouse move events to the active tool and all inactive tools that want them
    MouseToolHandler::onGLCapturedMouseMove(x, y, mouseState);
}

void TexTool::startCapture(const MouseToolPtr& tool)
{
    if (_freezePointer.isCapturing(_glWidget))
    {
        return;
    }

    unsigned int pointerMode = tool->getPointerMode();

    _freezePointer.startCapture(_glWidget,
        [&, tool](int x, int y, unsigned int state) { handleGLCapturedMouseMotion(tool, x, y, state); }, // Motion Functor
        [&, tool]() { MouseToolHandler::handleCaptureLost(tool); }, // called when the capture is lost.
        (pointerMode & MouseTool::PointerMode::Freeze) != 0,
        (pointerMode & MouseTool::PointerMode::Hidden) != 0,
        (pointerMode & MouseTool::PointerMode::MotionDeltas) != 0
    );
}

void TexTool::endCapture()
{
    if (!_freezePointer.isCapturing(_glWidget))
    {
        return;
    }

    _freezePointer.endCapture();
}


IInteractiveView& TexTool::getInteractiveView()
{
    return *this;
}

TextureToolMouseEvent TexTool::createMouseEvent(const Vector2& point, const Vector2& delta)
{
    Vector2 normalisedDeviceCoords = device_constrained(
        window_to_normalised_device(point, 
            static_cast<std::size_t>(_windowDims[0]), 
            static_cast<std::size_t>(_windowDims[1])));

    return TextureToolMouseEvent(*this, normalisedDeviceCoords, delta);
}

} // namespace ui
