#include "TexTool.h"

#include <limits>
#include "i18n.h"
#include "ui/ieventmanager.h"
#include "icommandsystem.h"
#include "itexturetoolmodel.h"
#include "itexturetoolcolours.h"
#include "ui/imainframe.h"
#include "igl.h"
#include "iundo.h"
#include "imap.h"
#include "igrid.h"
#include "iradiant.h"
#include "ui/itoolbarmanager.h"
#include "itextstream.h"

#include "registry/adaptors.h"
#include "registry/Widgets.h"
#include "wxutil/GLWidget.h"
#include "selection/Device.h"
#include "selection/SelectionVolume.h"
#include "selection/SelectionPool.h"
#include "messages/TextureChanged.h"
#include "fmt/format.h"

#include "textool/tools/TextureToolMouseEvent.h"

#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/toolbar.h>


namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Texture Tool");

    const std::string RKEY_TEXTOOL_ROOT = "user/ui/textures/texTool/";
	const std::string RKEY_WINDOW_STATE = RKEY_TEXTOOL_ROOT + "window";
    const char* const RKEY_SELECT_EPSILON = "user/ui/selectionEpsilon";

    const std::string RKEY_HSCALE_FACTOR = RKEY_TEXTOOL_ROOT + "horizontalScaleFactor";
    const std::string RKEY_VSCALE_FACTOR = RKEY_TEXTOOL_ROOT + "verticalScaleFactor";
    const std::string RKEY_ROTATE_ANGLE = RKEY_TEXTOOL_ROOT + "rotateAngle";

	constexpr const float ZOOM_MODIFIER = 1.25f;
}

TexTool::TexTool() : 
    TransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getWxTopLevelWindow(), true),
    MouseToolHandler(IMouseToolGroup::Type::TextureTool),
    _glWidget(new wxutil::GLWidget(this, std::bind(&TexTool::onGLDraw, this), "TexTool")),
    _gridActive(registry::getValue<bool>(textool::RKEY_GRID_STATE)),
    _selectionRescanNeeded(false),
    _manipulatorPanelNeedsUpdate(true),
    _activeMaterialNeedsUpdate(true),
    _manipulatorModeToggleRequestHandler(std::numeric_limits<std::size_t>::max()),
    _componentSelectionModeToggleRequestHandler(std::numeric_limits<std::size_t>::max()),
    _textureMessageHandler(std::numeric_limits<std::size_t>::max()),
    _gridSnapHandler(std::numeric_limits<std::size_t>::max()),
    _determineThemeFromImage(false)
{
	Bind(wxEVT_IDLE, &TexTool::onIdle, this);

	populateWindow();

	InitialiseWindowPosition(600, 400, RKEY_WINDOW_STATE);

    registry::observeBooleanKey(
        textool::RKEY_GRID_STATE,
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
#ifdef __WXMSW__
        textoolbar->SetBackgroundColour(wxColor("#ababab"));
#endif
		textoolbar->SetCanFocus(false);
		GetSizer()->Add(textoolbar, 0, wxEXPAND);
    }

    auto horizontalSizer = new wxBoxSizer(wxHORIZONTAL);

	horizontalSizer->Add(_glWidget, 1, wxEXPAND);
	horizontalSizer->Add(createManipulationPanel(), 0, wxEXPAND);

    GetSizer()->Add(horizontalSizer, 1, wxEXPAND);
}

wxWindow* TexTool::createManipulationPanel()
{
    auto panel = loadNamedPanel(this, "TextureToolManipulatorPanel");
    panel->SetCanFocus(false);

    makeLabelBold(panel, "ShiftLabel");
    makeLabelBold(panel, "ScaleLabel");
    makeLabelBold(panel, "RotateLabel");

    findNamedObject<wxButton>(panel, "ShiftUpButton")->Bind(wxEVT_BUTTON, [this] (wxCommandEvent&) { onShiftSelected("up"); });
    findNamedObject<wxButton>(panel, "ShiftDownButton")->Bind(wxEVT_BUTTON, [this] (wxCommandEvent&) { onShiftSelected("down"); });
    findNamedObject<wxButton>(panel, "ShiftLeftButton")->Bind(wxEVT_BUTTON, [this] (wxCommandEvent&) { onShiftSelected("left"); });
    findNamedObject<wxButton>(panel, "ShiftRightButton")->Bind(wxEVT_BUTTON, [this] (wxCommandEvent&) { onShiftSelected("right"); });

    findNamedObject<wxButton>(panel, "ScaleHorizSmallerButton")->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { onScaleSelected("left"); });
    findNamedObject<wxButton>(panel, "ScaleHorizLargerButton")->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { onScaleSelected("right"); });
    findNamedObject<wxButton>(panel, "ScaleVertSmallerButton")->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { onScaleSelected("down"); });
    findNamedObject<wxButton>(panel, "ScaleVertLargerButton")->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { onScaleSelected("up"); });

    findNamedObject<wxButton>(panel, "RotateClockWiseButton")->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { onRotateSelected("cw"); });
    findNamedObject<wxButton>(panel, "RotateCounterClockWiseButton")->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { onRotateSelected("ccw"); });

    convertToSpinCtrlDouble(panel, "HorizScaleFactor", 0.1, 1000, 0.5, 1);
    convertToSpinCtrlDouble(panel, "VertScaleFactor", 0.1, 1000, 0.5, 1);
    convertToSpinCtrlDouble(panel, "RotateAngle", 0, 360, 0.5, 1);

    registry::bindWidget(findNamedObject<wxSpinCtrlDouble>(panel, "HorizScaleFactor"), RKEY_HSCALE_FACTOR);
    registry::bindWidget(findNamedObject<wxSpinCtrlDouble>(panel, "VertScaleFactor"), RKEY_VSCALE_FACTOR);
    registry::bindWidget(findNamedObject<wxSpinCtrlDouble>(panel, "RotateAngle"), RKEY_ROTATE_ANGLE);

    return panel;
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
    
    GlobalRadiantCore().getMessageBus().removeListener(_gridSnapHandler);
    _gridSnapHandler = std::numeric_limits<std::size_t>::max();
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
            sigc::mem_fun(this, &TexTool::handleTextureChanged)));
    
    // Intercept the grid snap message
    _gridSnapHandler = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::GridSnapRequest,
        radiant::TypeListener<selection::GridSnapRequest>(
            sigc::mem_fun(this, &TexTool::handleGridSnapRequest)));

	_undoHandler = GlobalMapModule().signal_postUndo().connect(
		sigc::mem_fun(this, &TexTool::onUndoRedoOperation));
	_redoHandler = GlobalMapModule().signal_postRedo().connect(
		sigc::mem_fun(this, &TexTool::onUndoRedoOperation));

    _manipulatorChanged = GlobalTextureToolSelectionSystem().signal_activeManipulatorChanged().connect(
        sigc::mem_fun(this, &TexTool::onManipulatorModeChanged)
    );
    _selectionModeChanged = GlobalTextureToolSelectionSystem().signal_selectionModeChanged().connect(
        sigc::mem_fun(this, &TexTool::onSelectionModeChanged)
    );
    _selectionChanged = GlobalTextureToolSelectionSystem().signal_selectionChanged().connect(
        sigc::mem_fun(this, &TexTool::onSelectionChanged)
    );

    _gridChanged = GlobalGrid().signal_gridChanged().connect(sigc::mem_fun(this, &TexTool::queueDraw));

	// Trigger an update of the current selection
    _selectionRescanNeeded = true;
    updateThemeButtons();
    updateManipulationPanel();
    queueDraw();
}

bool TexTool::textureToolHasFocus()
{
    auto manipulationPanel = findNamedObject<wxWindow>(this, "TextureToolManipulatorPanel");

    return HasFocus() || _glWidget->HasFocus() || manipulationPanel->HasFocus() || manipulationPanel->FindFocus() != nullptr;
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

void TexTool::handleTextureChanged(radiant::TextureChangedMessage& message)
{
    _activeMaterialNeedsUpdate = true;
    queueDraw();
}

void TexTool::handleGridSnapRequest(selection::GridSnapRequest& request)
{
    if (!textureToolHasFocus())
    {
        return;
    }

    // Redirect this call to our own grid snap command
    GlobalCommandSystem().executeCommand("TexToolSnapToGrid");
    request.setHandled(true);
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
    _manipulatorPanelNeedsUpdate = true;
}

void TexTool::onSelectionChanged()
{
    queueDraw();
    _manipulatorPanelNeedsUpdate = true;
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

void TexTool::updateActiveMaterial()
{
    _activeMaterialNeedsUpdate = false;

    auto material = GlobalTextureToolSceneGraph().getActiveMaterial();
    _shader = !material.empty() ? GlobalMaterialManager().getMaterial(material) : MaterialPtr();

    if (material != _material)
    {
        _material = material;
        _determineThemeFromImage = true;
        recalculateVisibleTexSpace();
    }
}

void TexTool::update()
{
    updateActiveMaterial();
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

    if (_manipulatorPanelNeedsUpdate)
    {
        _manipulatorPanelNeedsUpdate = false;
        updateManipulationPanel();
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

void TexTool::recalculateVisibleTexSpace()
{
	// Get the selection extents, add some padding
    auto selectionBounds = getUvBoundsFromSceneSelection();
    selectionBounds.extents *= 1.5;

    // Center the visible tex space at the selection
    _texSpaceAABB.origin = selectionBounds.origin;

    // Use the window's aspect ratio
    _texSpaceAABB.extents.x() = _windowDims.x();
    _texSpaceAABB.extents.y() = _windowDims.y();

    // Make the visible space non-uniform if the texture has a width/height ratio != 1
    double textureAspect = getTextureAspectRatio();
    
    if (textureAspect > 1)
    {
        _texSpaceAABB.extents.y() *= textureAspect;
    }
    else if (textureAspect < 1)
    {
        _texSpaceAABB.extents.x() /= textureAspect;
    }

    // Proportionally shrink the visible bounds to fit the selection
    auto scale = std::max(selectionBounds.extents.x() / _texSpaceAABB.extents.x(),
        selectionBounds.extents.y() / _texSpaceAABB.extents.y());

    _texSpaceAABB.extents *= scale;

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

    // Don't return negative bounds in case we don't have anything selected
    if (!bounds.isValid())
    {
        bounds.origin.set(0, 0, 0);
        bounds.extents.set(0.5, 0.5, 0);
    }

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

void TexTool::drawGrid()
{
    auto gridSpacing = GlobalGrid().getGridSize(grid::Space::Texture);

	const auto& texSpaceAABB = getVisibleTexSpace();
    auto uPerPixel = texSpaceAABB.extents.x() / _windowDims.x();
    auto vPerPixel = texSpaceAABB.extents.y() / _windowDims.y();

    auto smallestUvPerPixel = uPerPixel > vPerPixel ? vPerPixel : uPerPixel;

    auto gridSpacingInPixels = gridSpacing / std::abs(smallestUvPerPixel);

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

    if (_gridActive)
    {
        // Draw the manipulation grid
        auto minorGrid = GlobalTextureToolColourSchemeManager().getColour(textool::SchemeElement::MinorGrid);
        glColor4fv(minorGrid);

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
    auto majorGrid = GlobalTextureToolColourSchemeManager().getColour(textool::SchemeElement::MajorGrid);
    glColor4fv(majorGrid);

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

	// Draw the axes through the origin
	glColor4fv(majorGrid);
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

    auto gridText = GlobalTextureToolColourSchemeManager().getColour(textool::SchemeElement::GridText);
    glColor4fv(gridText);

    float uvPerPixel = _texSpaceAABB.extents.y() / _windowDims.y();
    float uvFontHeight = (GlobalOpenGL().getFontHeight() + 3) * uvPerPixel;

	for (float y = startY; y <= endY; y += yIntStep)
	{
		glRasterPos2f(topLeft[0] + 0.05f, y + uvFontHeight);
        auto ycoordStr = fmt::format("{0:.1f}", trunc(y));
		GlobalOpenGL().drawString(ycoordStr);
	}

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
    if (_activeMaterialNeedsUpdate)
    {
        // Make sure to detect material changes
        updateActiveMaterial();
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

	// Do nothing if the shader name is empty
	if (!_shader || _shader->getName().empty())
	{
		return true;
	}

	auto& texSpaceAABB = getVisibleTexSpace();

    // Avoid drawing invalid bounds
    if (texSpaceAABB.extents.x() < 0 || texSpaceAABB.extents.y() < 0)
    {
        return true;
    }

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

    if (_determineThemeFromImage)
    {
        _determineThemeFromImage = false;

        // Download the pixel data from openGL
        std::vector<unsigned char> pixels;
        pixels.resize(tex->getWidth() * tex->getHeight() * 3);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

        determineThemeBasedOnPixelData(pixels);
    }

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
    Vector2 scaleFactors(
        ev.GetSize().GetWidth() / _windowDims.x(),
        ev.GetSize().GetHeight() / _windowDims.y()
    );

	// Store the window dimensions for later calculations
	_windowDims = Vector2(ev.GetSize().GetWidth(), ev.GetSize().GetHeight());

    // Adjust the texture space by the same amounts
    _texSpaceAABB.extents.x() *= scaleFactors.x();
    _texSpaceAABB.extents.y() *= scaleFactors.y();

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
    _glWidget->SetFocus();

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

void TexTool::resetViewCmd(const cmd::ArgumentList& args)
{
    Instance().recalculateVisibleTexSpace();
    Instance().queueDraw();
}

void TexTool::toggle(const cmd::ArgumentList& args)
{
	Instance().ToggleVisibility();
}

void TexTool::registerCommands()
{
	GlobalCommandSystem().addCommand("TextureTool", TexTool::toggle);
	GlobalCommandSystem().addCommand("TextureToolResetView", TexTool::resetViewCmd);
	GlobalEventManager().addRegistryToggle("TexToolToggleGrid", textool::RKEY_GRID_STATE);

    GlobalEventManager().addToggle("TextureToolUseLightTheme", [](bool toggled)
    {
        if (toggled)
        {
            GlobalTextureToolColourSchemeManager().setActiveScheme(textool::ColourScheme::Light);
            Instance().updateThemeButtons();
            Instance().queueDraw();
        }
    });

    GlobalEventManager().addToggle("TextureToolUseDarkTheme", [](bool toggled)
    {
        if (toggled)
        {
            GlobalTextureToolColourSchemeManager().setActiveScheme(textool::ColourScheme::Dark);
            Instance().updateThemeButtons();
            Instance().queueDraw();
        }
    });
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

void TexTool::updateThemeButtons()
{
    auto activeScheme = GlobalTextureToolColourSchemeManager().getActiveScheme();

    GlobalEventManager().setToggled("TextureToolUseDarkTheme", activeScheme == textool::ColourScheme::Dark);
    GlobalEventManager().setToggled("TextureToolUseLightTheme", activeScheme == textool::ColourScheme::Light);
}

void TexTool::determineThemeBasedOnPixelData(const std::vector<unsigned char>& pixels)
{
    std::size_t average = 0;
    auto numPixels = pixels.size();

    for (auto i = 0; i < numPixels; ++i)
    {
        average += pixels[i];
    }

    average /= numPixels;

    auto scheme = average > 128 ? textool::ColourScheme::Dark : textool::ColourScheme::Light;

    GlobalTextureToolColourSchemeManager().setActiveScheme(scheme);
    updateThemeButtons();
}

void TexTool::onShiftSelected(const std::string& direction)
{
    Vector2 translation(0, 0);

    auto gridSize = GlobalGrid().getGridSize(grid::Space::Texture);

    if (direction == "up")
    {
        translation = Vector2(0, -gridSize);
    }
    else if (direction == "down")
    {
        translation = Vector2(0, gridSize);
    }
    else if (direction == "left")
    {
        translation = Vector2(-gridSize, 0);
    }
    else if (direction == "right")
    {
        translation = Vector2(gridSize, 0);
    }

    GlobalCommandSystem().executeCommand("TexToolShiftSelected", translation);
}

void TexTool::onScaleSelected(const std::string& direction)
{
    Vector2 scale(1, 1);

    if (direction == "up")
    {
        auto factor = findNamedObject<wxSpinCtrlDouble>(this, "VertScaleFactor")->GetValue();
        scale = Vector2(1, 1 + factor/100);
    }
    else if (direction == "down")
    {
        auto factor = findNamedObject<wxSpinCtrlDouble>(this, "VertScaleFactor")->GetValue();
        scale = Vector2(1, 1 / (1 + factor / 100));
    }
    else if (direction == "left")
    {
        auto factor = findNamedObject<wxSpinCtrlDouble>(this, "HorizScaleFactor")->GetValue();
        scale = Vector2(1 / (1 + factor / 100), 1);
    }
    else if (direction == "right")
    {
        auto factor = findNamedObject<wxSpinCtrlDouble>(this, "HorizScaleFactor")->GetValue();
        scale = Vector2(1 + factor / 100, 1);
    }

    GlobalCommandSystem().executeCommand("TexToolScaleSelected", scale);
}

void TexTool::onRotateSelected(const std::string& direction)
{
    double angle = findNamedObject<wxSpinCtrlDouble>(this, "RotateAngle")->GetValue();
    angle *= direction == "cw" ? +1 : -1;

    GlobalCommandSystem().executeCommand("TexToolRotateSelected", angle);
}

void TexTool::updateManipulationPanel()
{
    bool isInComponentMode = GlobalTextureToolSelectionSystem().getSelectionMode() == textool::SelectionMode::Vertex;

    bool anythingSelected = !isInComponentMode && GlobalTextureToolSelectionSystem().countSelected() > 0 ||
        isInComponentMode && GlobalTextureToolSelectionSystem().countSelectedComponentNodes() > 0;

    findNamedObject<wxWindow>(this, "ShiftPanel")->Enable(anythingSelected);
    findNamedObject<wxWindow>(this, "ScalePanel")->Enable(anythingSelected && !isInComponentMode);
    findNamedObject<wxWindow>(this, "RotatePanel")->Enable(anythingSelected && !isInComponentMode);
}

} // namespace ui
