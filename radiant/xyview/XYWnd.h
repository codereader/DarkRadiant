#pragma once

#include "iscenegraph.h"
#include "iorthoview.h"

#include "math/Vector3.h"
#include "math/Matrix4.h"
#include "math/Vector4.h"
#include "wxutil/FreezePointer.h"
#include "wxutil/event/KeyEventFilter.h"
#include "wxutil/GLWidget.h"

#include <wx/cursor.h>
#include <wx/stopwatch.h>

#include "camera/CameraObserver.h"
#include "render/View.h"
#include "imousetool.h"
#include "tools/XYMouseToolEvent.h"
#include "wxutil/MouseToolHandler.h"

namespace ui
{

class XYWnd :
    public IOrthoView,
    public CameraObserver,
    public scene::Graph::Observer,
    public wxEvtHandler,
    protected wxutil::MouseToolHandler
{
protected:
    // Unique ID of this XYWnd
    int _id;

    wxutil::GLWidget* _wxGLWidget;
    bool _drawing;

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

    // Shader to use for selected items
    static ShaderPtr _selectedShader;
	static ShaderPtr _selectedShaderGroup;

    Vector3 _mousePosition;

    EViewType _viewType;

    int _contextMenu_x;
    int _contextMenu_y;
    bool _contextMenu;

    wxutil::KeyEventFilterPtr _escapeListener;

    // Save the current button state
    unsigned int _eventState;

    bool _isActive;

    int _chasemouseCurrentX;
    int _chasemouseCurrentY;
    int _chasemouseDeltaX;
    int _chasemouseDeltaY;

    Matrix4 _projection;
    Matrix4 _modelView;

    int _width;
    int _height;

public:
    // Constructor, this allocates the GL widget
    XYWnd(int uniqueId, wxWindow* parent);

    // Destructor
    virtual ~XYWnd();

    int getId() const;

    wxutil::GLWidget* getGLWidget() const { return _wxGLWidget; }

    SelectionTestPtr createSelectionTestForPoint(const Vector2& point) override;
    const VolumeTest& getVolumeTest() const override;
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
    const Vector3& getOrigin();
    void setOrigin(const Vector3& origin);
    void scroll(int x, int y) override;
    Vector4 getWindowCoordinates();

    void draw();
    void drawCameraIcon(const Vector3& origin, const Vector3& angles);
    void drawBlockGrid();
    void drawGrid();

    Vector3 convertXYToWorld(int x, int y);
    void snapToGrid(Vector3& point) override;

    void mouseToPoint(int x, int y, Vector3& point);

    void beginMove();
    void endMove();

    void zoomIn() override;
    void zoomOut() override;

    void setActive(bool b);
    bool isActive() const;

    void setCursorType(IOrthoView::CursorType type) override;

    void updateModelview();
    void updateProjection();

    virtual void setViewType(EViewType n);
    EViewType getViewType() const override;

    void setScale(float f);
    float getScale() const override;

    int getWidth() const;
    int getHeight() const;

    // greebo: CameraObserver implementation; gets called when the camera is moved
    void cameraMoved() override;

    // greebo: This gets called upon scene change
    void onSceneGraphChange() override;

protected:
    // Disconnects all widgets and unsubscribes as observer
    void destroyXYView();

    // Required overrides being a MouseToolHandler
    virtual MouseTool::Result processMouseDownEvent(const MouseToolPtr& tool, const Vector2& point) override;
    virtual MouseTool::Result processMouseUpEvent(const MouseToolPtr& tool, const Vector2& point) override;
    virtual MouseTool::Result processMouseMoveEvent(const MouseToolPtr& tool, int x, int y) override;
    virtual void startCapture(const MouseToolPtr& tool) override;
    virtual void endCapture() override;
    virtual IInteractiveView& getInteractiveView() override;

private:
    XYMouseToolEvent createMouseEvent(const Vector2& point, const Vector2& delta = Vector2(0, 0));

    void onContextMenu();
    void drawSizeInfo(int nDim1, int nDim2, const Vector3& vMinBounds, const Vector3& vMaxBounds);

    // callbacks
    bool checkChaseMouse(unsigned int state);
    void performChaseMouse();
    void onIdle(wxIdleEvent& ev);

    // The method responsible for mouseMove situations according to <event>
    void handleGLMouseMotion(int x, int y, unsigned int state, bool isDelta);

    // Active mousetools might capture the mouse, this is handled here
    void handleGLCapturedMouseMotion(const MouseToolPtr& tool, int x, int y, unsigned int state);

    // wxGLWidget-attached render method
    void onRender();
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
