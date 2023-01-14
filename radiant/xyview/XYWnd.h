#pragma once

#include "iscenegraph.h"
#include "iorthoview.h"
#include "igl.h"

#include "math/Vector3.h"
#include "math/Matrix4.h"
#include "math/Vector4.h"
#include "wxutil/FreezePointer.h"
#include "wxutil/event/KeyEventFilter.h"
#include "wxutil/GLWidget.h"

#include <optional>
#include <wx/cursor.h>
#include <wx/stopwatch.h>
#include <sigc++/connection.h>

#include "render/View.h"
#include "imousetool.h"
#include "tools/XYMouseToolEvent.h"
#include "wxutil/MouseToolHandler.h"
#include "wxutil/DockablePanel.h"
#include "XYRenderer.h"

namespace ui
{

class XYWndManager;

class XYWnd final :
    public wxutil::DockablePanel,
    public IOrthoView,
    public scene::Graph::Observer,
    protected wxutil::MouseToolHandler
{
private:
    XYWndManager& _owner;

    int _id;
    static int _nextId;

    wxutil::GLWidget* _wxGLWidget;
    bool _drawing;
    bool _updateRequested;

    // The maximum/minimum values of a coordinate
    double _minWorldCoord;
    double _maxWorldCoord;

    // The timer used for chase mouse xyview movements
    wxStopWatch _chaseMouseTimer;

    wxutil::FreezePointer _freezePointer;

    wxCursor _defaultCursor;
    wxCursor _crossHairCursor;

    bool _chasingMouse;

    double	_scale;
    Vector3 _origin;

    render::View _view;

    // Shaders used for highlighting nodes
    static XYRenderer::HighlightShaders _highlightShaders;

    Vector3 _mousePosition;

    EViewType _viewType;

    // Context menu handling. Because we use right-click for both context menu and panning
    // (probably a bad design choice, but we're stuck with it), we need to distinguish between a
    // click-and-release (context menu) and drag (pan). For this we record the initial click
    // position and the current position, and show the context menu only if the difference is small
    // at the point of mouse release.
    struct ContextMenuClick {
        Vector2i initial;
        Vector2i current;
    };
    std::optional<ContextMenuClick> _rightClickPos;

    wxutil::KeyEventFilterPtr _escapeListener;

    // Save the current button state
    unsigned int _eventState;

    bool _isActive;

    int _chasemouseCurrentX;
    int _chasemouseCurrentY;
    int _chasemouseDeltaX;
    int _chasemouseDeltaY;

    Matrix4 _projection = Matrix4::getIdentity();
    Matrix4 _modelView = Matrix4::getIdentity();

    int _width;
    int _height;

    sigc::connection _sigCameraChanged;

    IGLFont::Ptr _font;

public:
    XYWnd(wxWindow* parent, XYWndManager& owner);
    ~XYWnd() override;

    int getId() const;

    wxutil::GLWidget* getGLWidget() const { return _wxGLWidget; }

    SelectionTestPtr createSelectionTestForPoint(const Vector2& point) override;
    const VolumeTest& getVolumeTest() const override;
    bool supportsDragSelections() override;
    int getDeviceWidth() const override;
    int getDeviceHeight() const override;
    void queueDraw() override;
    void forceRedraw() override;

    // Capture and release the selected shader
    static void captureStates();
    static void releaseStates();

    // Returns the long type string ("XY Top", "YZ Side", "XZ Front") for use in window titles
    static const std::string getViewTypeTitle(EViewType viewtype);

    // Returns the short type string (XY, XZ, YZ)
    static const std::string getViewTypeStr(EViewType viewtype);

    void positionView(const Vector3& position);
    const Vector3& getOrigin() const override;
    void setOrigin(const Vector3& origin) override;
    void scrollByPixels(int x, int y) override;
    Vector4 getWindowCoordinates();

    void draw();
    void drawBlockGrid();
    void drawGrid();

    Vector3 convertXYToWorld(int x, int y);
    void snapToGrid(Vector3& point) override;

    void mouseToPoint(int x, int y, Vector3& point);

    void zoomIn() override;
    void zoomOut() override;
    void zoomInOn( wxPoint cursor, int zoom );

    void setActive(bool b);
    bool isActive() const;

    void setCursorType(IOrthoView::CursorType type) override;

    void updateModelview();
    void updateProjection();

    void setViewType(EViewType n);
    EViewType getViewType() const override;

    void setScale(float f);
    float getScale() const override;

    int getWidth() const override;
    int getHeight() const override;

    // greebo: This gets called upon scene change
    void onSceneGraphChange() override;

    void updateFont();

protected:
    // Disconnects all widgets and unsubscribes as observer
    void destroyXYView();

    // Required overrides being a MouseToolHandler
    MouseTool::Result processMouseDownEvent(const MouseToolPtr& tool, const Vector2& point) override;
    MouseTool::Result processMouseUpEvent(const MouseToolPtr& tool, const Vector2& point) override;
    MouseTool::Result processMouseMoveEvent(const MouseToolPtr& tool, int x, int y) override;
    void startCapture(const MouseToolPtr& tool) override;
    void endCapture() override;
    IInteractiveView& getInteractiveView() override;

private:
    XYMouseToolEvent createMouseEvent(const Vector2& point, const Vector2& delta = Vector2(0, 0));

    void ensureFont();
    void onContextMenu();
    void drawSizeInfo(int nDim1, int nDim2, const Vector3& vMinBounds, const Vector3& vMaxBounds);
    void drawSelectionFocusArea(int nDim1, int nDim2, float xMin, float xMax, float yMin, float yMax);
    void drawCameraIcon();
    float getZoomedScale(int steps);

    // callbacks
    bool checkChaseMouse(unsigned int state);
    void performChaseMouse();
    void onIdle(wxIdleEvent& ev);
    void onCameraMoved();

    // The method responsible for mouseMove situations according to <event>
    void handleGLMouseMotion(int x, int y, unsigned int state, bool isDelta);

    // Active mousetools might capture the mouse, this is handled here
    void handleGLCapturedMouseMotion(const MouseToolPtr& tool, int x, int y, unsigned int state);

    // wxGLWidget-attached render method
    bool onRender();
    void onGLResize(wxSizeEvent& ev);
    void onGLWindowScroll(wxMouseEvent& ev);

    void onGLMouseButtonPress(wxMouseEvent& ev);
    void onGLMouseButtonRelease(wxMouseEvent& ev);
    void onGLMouseMove(wxMouseEvent& ev);
};

/**
 * Shared pointer typedefs.
 */
typedef std::shared_ptr<XYWnd> XYWndPtr;
typedef std::weak_ptr<XYWnd> XYWndWeakPtr;

}
