#include "TexTool.h"

#include <limits>
#include "i18n.h"
#include "ieventmanager.h"
#include "icommandsystem.h"
#include "itexturetoolmodel.h"
#include "imainframe.h"
#include "igl.h"
#include "iundo.h"
#include "igrid.h"
#include "ibrush.h"
#include "iradiant.h"
#include "ipatch.h"
#include "itoolbarmanager.h"
#include "itextstream.h"

#include "registry/adaptors.h"
#include "wxutil/GLWidget.h"
#include "selection/Device.h"
#include "selection/SelectionVolume.h"
#include "selection/SelectionPool.h"
#include "messages/TextureChanged.h"
#include "fmt/format.h"

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
    _grid(GRID_DEFAULT),
    _gridActive(registry::getValue<bool>(RKEY_GRID_STATE)),
    _selectionRescanNeeded(false),
    _manipulatorModeToggleRequestHandler(std::numeric_limits<std::size_t>::max()),
    _componentSelectionModeToggleRequestHandler(std::numeric_limits<std::size_t>::max()),
    _textureMessageHandler(std::numeric_limits<std::size_t>::max())
{
	Bind(wxEVT_IDLE, &TexTool::onIdle, this);

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

	_sceneSelectionChanged.disconnect();
    _manipulatorChanged.disconnect();
    _selectionModeChanged.disconnect();
    _selectionChanged.disconnect();
    _gridChanged.disconnect();

    GlobalRadiantCore().getMessageBus().removeListener(_manipulatorModeToggleRequestHandler);
    _manipulatorModeToggleRequestHandler = std::numeric_limits<std::size_t>::max();

    GlobalRadiantCore().getMessageBus().removeListener(_componentSelectionModeToggleRequestHandler);
    _componentSelectionModeToggleRequestHandler = std::numeric_limits<std::size_t>::max();
    
    GlobalRadiantCore().getMessageBus().removeListener(_textureMessageHandler);
    _textureMessageHandler = std::numeric_limits<std::size_t>::max();
}

// Pre-show callback
void TexTool::_preShow()
{
	TransientWindow::_preShow();

	_sceneSelectionChanged.disconnect();
	_undoHandler.disconnect();
	_redoHandler.disconnect();
    _manipulatorChanged.disconnect();
    _selectionModeChanged.disconnect();
    _selectionChanged.disconnect();
    _gridChanged.disconnect();

	// Register self to the SelSystem to get notified upon selection changes.
	_sceneSelectionChanged = GlobalSelectionSystem().signal_selectionChanged().connect(
        [this](const ISelectable&) { _selectionRescanNeeded = true; });

    _manipulatorModeToggleRequestHandler = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::ManipulatorModeToggleRequest,
        radiant::TypeListener<selection::ManipulatorModeToggleRequest>(
            sigc::mem_fun(this, &TexTool::handleManipulatorModeToggleRequest)));

    _componentSelectionModeToggleRequestHandler = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::ComponentSelectionModeToggleRequest,
        radiant::TypeListener<selection::ComponentSelectionModeToggleRequest>(
            sigc::mem_fun(this, &TexTool::handleComponentSelectionModeToggleRequest)));

    // Get notified about texture changes
    _textureMessageHandler = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::TextureChanged,
        radiant::TypeListener<radiant::TextureChangedMessage>(
            [this](radiant::TextureChangedMessage& msg) { queueDraw(); }));

	_undoHandler = GlobalUndoSystem().signal_postUndo().connect(
		sigc::mem_fun(this, &TexTool::onUndoRedoOperation));
	_redoHandler = GlobalUndoSystem().signal_postRedo().connect(
		sigc::mem_fun(this, &TexTool::onUndoRedoOperation));

    _manipulatorChanged = GlobalTextureToolSelectionSystem().signal_activeManipulatorChanged().connect(
        sigc::mem_fun(this, &TexTool::onManipulatorModeChanged)
    );
    _selectionModeChanged = GlobalTextureToolSelectionSystem().signal_selectionModeChanged().connect(
        sigc::mem_fun(this, &TexTool::onSelectionModeChanged)
    );
    _selectionChanged = GlobalTextureToolSelectionSystem().signal_selectionChanged().connect(
        sigc::mem_fun(this, &TexTool::queueDraw)
    );

    _gridChanged = GlobalGrid().signal_gridChanged().connect(sigc::mem_fun(this, &TexTool::queueDraw));

	// Trigger an update of the current selection
    _selectionRescanNeeded = true;
    queueDraw();
}

bool TexTool::textureToolHasFocus()
{
    return HasFocus() || _glWidget->HasFocus();
}

void TexTool::handleManipulatorModeToggleRequest(selection::ManipulatorModeToggleRequest& request)
{
    if (!textureToolHasFocus())
    {
        return;
    }

    // Catch specific manipulator types, let the others slip through
    switch (request.getType())
    {
    case selection::IManipulator::Drag:
    case selection::IManipulator::Rotate:
        GlobalTextureToolSelectionSystem().toggleManipulatorMode(request.getType());
        request.setHandled(true);
        break;
    };
}

void TexTool::handleComponentSelectionModeToggleRequest(selection::ComponentSelectionModeToggleRequest& request)
{
    if (!textureToolHasFocus())
    {
        return;
    }

    // Catch specific mode types, let the others slip through
    switch (request.getMode())
    {
    case selection::ComponentSelectionMode::Default:
        GlobalCommandSystem().executeCommand("ToggleTextureToolSelectionMode", { "Surface" });
        request.setHandled(true);
        break;
    case selection::ComponentSelectionMode::Vertex:
        GlobalCommandSystem().executeCommand("ToggleTextureToolSelectionMode", { "Vertex" });
        request.setHandled(true);
        break;
    };
}

void TexTool::onUndoRedoOperation()
{
    queueDraw();
}

void TexTool::onManipulatorModeChanged(selection::IManipulator::Type type)
{
    queueDraw();
}

void TexTool::onSelectionModeChanged(textool::SelectionMode mode)
{
    queueDraw();
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
	auto& instancePtr = InstancePtr();

	if (!instancePtr)
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
    const auto& activeMaterial = GlobalTextureToolSceneGraph().getActiveMaterial();
    _shader = !activeMaterial.empty() ? GlobalMaterialManager().getMaterial(activeMaterial) : MaterialPtr();

	recalculateVisibleTexSpace();
}

int TexTool::getWidth() const
{
    return static_cast<int>(_windowDims[0]);
}

int TexTool::getHeight() const
{
    return static_cast<int>(_windowDims[1]);
}

void TexTool::zoomIn()
{
    _texSpaceAABB.extents *= ZOOM_MODIFIER;
    updateProjection();
}

void TexTool::zoomOut()
{
    _texSpaceAABB.extents /= ZOOM_MODIFIER;
    updateProjection();
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
    return getWidth();
}

int TexTool::getDeviceHeight() const
{
    return getHeight();
}

const VolumeTest& TexTool::getVolumeTest() const
{
    return _view;
}

void TexTool::forceRedraw()
{
    _glWidget->Refresh();
    _glWidget->Update();
}

void TexTool::draw()
{
	// Redraw
	_glWidget->Refresh();
}

void TexTool::onIdle(wxIdleEvent& ev)
{
    if (_selectionRescanNeeded)
    {
        _selectionRescanNeeded = false;
        update();
        queueDraw();
    }
}

void TexTool::queueDraw()
{
    _glWidget->Refresh();
}

void TexTool::scrollByPixels(int x, int y)
{
    auto uPerPixel = _texSpaceAABB.extents[0] * 2 / _windowDims[0];
    auto vPerPixel = _texSpaceAABB.extents[1] * 2 / _windowDims[1];
    
    _texSpaceAABB.origin[0] -= x * uPerPixel;
    _texSpaceAABB.origin[1] -= y * vPerPixel;

    updateProjection();
}

void TexTool::flipSelected(int axis)
{
	if (GlobalTextureToolSelectionSystem().countSelected() > 0)
    {
		beginOperation();
#if 0
		for (std::size_t i = 0; i < _items.size(); i++) {
			_items[i]->flipSelected(axis);
		}
#endif
		draw();
		endOperation("TexToolMergeItems");
	}
}

void TexTool::mergeSelectedItems()
{
	if (GlobalTextureToolSelectionSystem().countSelected() > 0)
    {
		AABB selExtents;
#if 0
		for (std::size_t i = 0; i < _items.size(); i++) {
			selExtents.includeAABB(_items[i]->getSelectedExtents());
		}
#endif
		if (selExtents.isValid()) {
			beginOperation();

			Vector2 centroid(
				selExtents.origin[0],
				selExtents.origin[1]
			);
#if 0
			for (std::size_t i = 0; i < _items.size(); i++) {
				_items[i]->moveSelectedTo(centroid);
			}
#endif
			draw();

			endOperation("TexToolMergeItems");
		}
	}
}

void TexTool::snapToGrid() {
	if (_gridActive) {
		beginOperation();
#if 0
		for (std::size_t i = 0; i < _items.size(); i++) {
			_items[i]->snapSelectedToGrid(_grid);
		}
#endif
		endOperation("TexToolSnapToGrid");

		draw();
	}
}

void TexTool::recalculateVisibleTexSpace()
{
	// Get the selection extents
	_texSpaceAABB = getUvBoundsFromSceneSelection();
    _texSpaceAABB.extents *= 2; // add some padding around the selection

	// Normalise the plane to be square
	_texSpaceAABB.extents[0] = std::max(_texSpaceAABB.extents[0], _texSpaceAABB.extents[1]);
	_texSpaceAABB.extents[1] = _texSpaceAABB.extents[0];

    // Make the visible space non-uniform if the texture has a width/height ratio != 1
    double textureAspect = getTextureAspectRatio();

    if (textureAspect < 1)
    {
        _texSpaceAABB.extents.x() /= textureAspect;
    }
    else
    {
        _texSpaceAABB.extents.y() *= textureAspect;
    }

    // Initially grow this texture space to match/fit the window aspect
    double windowAspect = _windowDims.x() / _windowDims.y();
    double currentAspect = _texSpaceAABB.extents.x() / _texSpaceAABB.extents.y();

    if (currentAspect < windowAspect)
    {
        _texSpaceAABB.extents.x() = windowAspect * _texSpaceAABB.extents.y();
    }
    else
    {
        _texSpaceAABB.extents.y() = 1 / windowAspect * _texSpaceAABB.extents.x();
    }

    updateProjection();
}

AABB TexTool::getUvBoundsFromSceneSelection()
{
	AABB bounds;

    GlobalTextureToolSceneGraph().foreachNode([&](const textool::INode::Ptr& node)
    {
        // Expand the selection AABB by the extents of the item
        bounds.includeAABB(node->localAABB());
        return true;
    });

	return bounds;
}

const AABB& TexTool::getVisibleTexSpace()
{
	return _texSpaceAABB;
}

void TexTool::drawUVCoords()
{
    GlobalTextureToolSceneGraph().foreachNode([&](const textool::INode::Ptr& node)
    {
        node->render(GlobalTextureToolSelectionSystem().getSelectionMode());
        return true;
    });
}

void TexTool::beginOperation()
{
	// Start an undo recording session
	GlobalUndoSystem().start();
#if 0
	// Tell all the items to save their memento
	for (std::size_t i = 0; i < _items.size(); i++)
	{
		_items[i]->beginTransformation();
	}
#endif
}

void TexTool::endOperation(const std::string& commandName)
{
#if 0
	for (std::size_t i = 0; i < _items.size(); i++)
	{
		_items[i]->endTransformation();
	}
#endif
	GlobalUndoSystem().finish(commandName);
}

void TexTool::selectRelatedItems()
{
#if 0
	for (std::size_t i = 0; i < _items.size(); i++) {
		_items[i]->selectRelated();
	}
#endif
	draw();
}

void TexTool::drawGrid()
{
    auto gridSpacing = GlobalGrid().getGridSize(grid::Space::Texture);

	const auto& texSpaceAABB = getVisibleTexSpace();
    auto uPerPixel = texSpaceAABB.extents.x() / _windowDims.x();
    auto vPerPixel = texSpaceAABB.extents.y() / _windowDims.y();

    auto smallestUvPerPixel = uPerPixel > vPerPixel ? vPerPixel : uPerPixel;

    auto gridSpacingInPixels = gridSpacing / smallestUvPerPixel;

    // Ensure that the grid spacing is at least 10 pixels wide
    while (gridSpacingInPixels < 12)
    {
        gridSpacing *= GlobalGrid().getGridBase(grid::Space::Texture);
        gridSpacingInPixels = gridSpacing / smallestUvPerPixel;
    }

	const float MAX_NUMBER_OF_GRID_LINES = 256;
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

		for (float y = startY; y <= endY; y += gridSpacing)
		{
			glVertex2f(startX, y);
			glVertex2f(endX, y);
		}

		for (float x = startX; x <= endX; x += gridSpacing)
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

	// Draw coordinate strings, with a higher minimum spacing than the lines
    while (yIntStep / vPerPixel < 32)
    {
        yIntStep *= 2;
    }

    while (xIntStep / uPerPixel < 64)
    {
        xIntStep *= 2;
    }

	for (float y = startY; y <= endY; y += yIntStep)
	{
		glRasterPos2f(topLeft[0] + 0.05f, y + 0.05f);
        auto ycoordStr = fmt::format("{0:.1f}", trunc(y));
		GlobalOpenGL().drawString(ycoordStr);
	}

    float uvPerPixel = _texSpaceAABB.extents.y() / _windowDims.y();
    float uvFontHeight = (GlobalOpenGL().getFontHeight() + 3) * uvPerPixel;

	for (float x = startX; x <= endX; x += xIntStep)
	{
		glRasterPos2f(x + 0.05f, topLeft.y() + uvFontHeight);
		auto xcoordStr = fmt::format("{0:.1f}", trunc(x));
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
    _view.constructFromTextureSpaceBounds(_texSpaceAABB,
        static_cast<std::size_t>(_windowDims[0]),
        static_cast<std::size_t>(_windowDims[1]));
}

bool TexTool::onGLDraw()
{
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

	// Do nothing if the shader name is empty
	if (!_shader || _shader->getName().empty())
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

    GlobalTextureToolSelectionSystem().getActiveManipulator()->renderComponents(_view, GlobalTextureToolSelectionSystem().getPivot2World());

    if (!_activeMouseTools.empty())
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, _windowDims[0], 0, _windowDims[1], 0, 1);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        for (const auto& i : _activeMouseTools)
        {
            i.second->renderOverlay();
        }
    }

	return true;
}

void TexTool::onGLResize(wxSizeEvent& ev)
{
	// Store the window dimensions for later calculations
	_windowDims = Vector2(ev.GetSize().GetWidth(), ev.GetSize().GetHeight());

    // Adjust the texture space to match up the new window aspect
    double texSpaceAspect = _texSpaceAABB.extents.x() / _texSpaceAABB.extents.y();
    double windowAspect = _windowDims.x() / _windowDims.y();

    if (windowAspect > texSpaceAspect)
    {
        _texSpaceAABB.extents.x() = windowAspect * _texSpaceAABB.extents.y();
    }
    else
    {
        _texSpaceAABB.extents.y() = 1 / windowAspect * _texSpaceAABB.extents.x();
    }

    updateProjection();

	// Queue an expose event
	_glWidget->Refresh();
}

void TexTool::onMouseUp(wxMouseEvent& ev)
{
    // Regular mouse tool processing
    MouseToolHandler::onGLMouseButtonRelease(ev);

	ev.Skip();
}

void TexTool::onMouseDown(wxMouseEvent& ev)
{
    // Send the event to the mouse tool handler
    MouseToolHandler::onGLMouseButtonPress(ev);

	ev.Skip();
}

void TexTool::onMouseMotion(wxMouseEvent& ev)
{
    MouseToolHandler::onGLMouseMove(ev);

	ev.Skip();
}

void TexTool::onMouseScroll(wxMouseEvent& ev)
{
    if (ev.GetWheelRotation() > 0)
    {
        zoomOut();
    }
    else
    {
        zoomIn();
    }

    draw();
}

// Static command targets
void TexTool::toggle(const cmd::ArgumentList& args)
{
	// Call the toggle() method of the static instance
	Instance().ToggleVisibility();
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
