#include "CameraWndManager.h"

#include "imousetoolmanager.h"
#include "ui/iuserinterface.h"
#include "itextstream.h"
#include "xmlutil/Node.h"

#include "CameraSettings.h"

#include "registry/registry.h"
#include "module/StaticModule.h"
#include "wxutil/MouseButton.h"
#include "string/case_conv.h"

#include "tools/ShaderClipboardTools.h"
#include "tools/JumpToObjectTool.h"
#include "tools/FreeMoveTool.h"
#include "tools/PanViewTool.h"

#include <functional>

namespace ui
{

namespace
{
    constexpr float DEFAULT_STRAFE_SPEED = 0.65f;
    constexpr float DEFAULT_FORWARD_STRAFE_FACTOR = 1.0f;
}

class CameraControl :
    public IUserControl
{
private:
    CameraWndManager& _owner;

public:
    CameraControl(CameraWndManager& owner) :
        _owner(owner)
    {}

    std::string getControlName() override
    {
        return UserControl::Camera;
    }

    std::string getDisplayName() override
    {
        return _("Camera");
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        auto cam = new CamWnd(parent, _owner);

        // Inherit origin and angles from existing cameras
        if (_owner.getActiveCamWnd())
        {
            auto activeCam = _owner.getActiveCamWnd();
            cam->setCameraOrigin(activeCam->getCameraOrigin());
            cam->setCameraAngles(activeCam->getCameraAngles());
        }

        return cam;
    }
};

CameraWndManager::CameraWndManager() :
	_activeCam(-1),
    _toggleStrafeModifierFlags(wxutil::Modifier::NONE),
    _toggleStrafeForwardModifierFlags(wxutil::Modifier::NONE),
    _strafeSpeed(DEFAULT_STRAFE_SPEED),
    _forwardStrafeFactor(DEFAULT_FORWARD_STRAFE_FACTOR)
{}

void CameraWndManager::registerCommands()
{
    GlobalCommandSystem().addStatement("NewCameraView", fmt::format("{0} {1}", CREATE_CONTROL_COMMAND, UserControl::Camera), false);

	GlobalCommandSystem().addCommand("CenterView", std::bind(&CameraWndManager::resetCameraAngles, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CubicClipZoomIn", std::bind(&CameraWndManager::farClipPlaneIn, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CubicClipZoomOut", std::bind(&CameraWndManager::farClipPlaneOut, this, std::placeholders::_1));

	GlobalCommandSystem().addCommand("UpFloor", std::bind(&CameraWndManager::changeFloorUp, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("DownFloor", std::bind(&CameraWndManager::changeFloorDown, this, std::placeholders::_1));

	// angua: increases and decreases the movement speed of the camera
	GlobalCommandSystem().addCommand("CamIncreaseMoveSpeed", std::bind(&CameraWndManager::increaseCameraSpeed, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CamDecreaseMoveSpeed", std::bind(&CameraWndManager::decreaseCameraSpeed, this, std::placeholders::_1));

	GlobalCommandSystem().addCommand("TogglePreview", std::bind(&CameraWndManager::toggleLightingMode, this, std::placeholders::_1));

	GlobalCommandSystem().addCommand("MoveCamera", std::bind(&CameraWndManager::moveCameraCmd, this, std::placeholders::_1),
		{ cmd::ARGTYPE_STRING, cmd::ARGTYPE_DOUBLE });

	// Insert movement commands
	GlobalCommandSystem().addCommand("CameraStrafeRight", std::bind(&CameraWndManager::moveRightDiscrete, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CameraStrafeLeft", std::bind(&CameraWndManager::moveLeftDiscrete, this, std::placeholders::_1));

	GlobalCommandSystem().addCommand("CameraAngleUp", std::bind(&CameraWndManager::pitchUpDiscrete, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CameraAngleDown", std::bind(&CameraWndManager::pitchDownDiscrete, this, std::placeholders::_1));

	GlobalEventManager().addRegistryToggle("ToggleCameraGrid", RKEY_CAMERA_GRID_ENABLED);
	GlobalEventManager().addRegistryToggle("ToggleShadowMapping", RKEY_ENABLE_SHADOW_MAPPING);

	GlobalEventManager().addKeyEvent("CameraMoveForward", std::bind(&CameraWndManager::onMoveForwardKey, this, std::placeholders::_1));
	GlobalEventManager().addKeyEvent("CameraMoveBack", std::bind(&CameraWndManager::onMoveBackKey, this, std::placeholders::_1));
	GlobalEventManager().addKeyEvent("CameraMoveLeft", std::bind(&CameraWndManager::onMoveLeftKey, this, std::placeholders::_1));
	GlobalEventManager().addKeyEvent("CameraMoveRight", std::bind(&CameraWndManager::onMoveRightKey, this, std::placeholders::_1));
	GlobalEventManager().addKeyEvent("CameraMoveUp", std::bind(&CameraWndManager::onMoveUpKey, this, std::placeholders::_1));
	GlobalEventManager().addKeyEvent("CameraMoveDown", std::bind(&CameraWndManager::onMoveDownKey, this, std::placeholders::_1));
}

CamWnd* CameraWndManager::getActiveCamWnd()
{
	// Sanity check in debug builds
	assert(_activeCam == -1 || _cameras.find(_activeCam) != _cameras.end());

	if (_activeCam == -1) return nullptr;

	auto cam = _cameras[_activeCam];

	if (!cam)
	{
		// Camera is not used anymore, remove it
		removeCamWnd(_activeCam);

		// Find a new active camera
		if (!_cameras.empty())
		{
			_activeCam = _cameras.begin()->first;
		}
		else
		{
			// No more cameras available
			_activeCam = -1;
		}

		if (_activeCam != -1)
		{
			cam = _cameras[_activeCam];
		}
	}

	return cam;
}

void CameraWndManager::setActiveCamWnd(int id)
{
    if (_cameras.count(id) > 0)
    {
        _activeCam = id;
    }
}

void CameraWndManager::addCamWnd(int id, CamWnd* cam)
{
    _cameras.emplace(id, cam);

    // New cameras are active immediately
    _activeCam = cam->getId();
}

void CameraWndManager::removeCamWnd(int id)
{
    // Find and remove the CamWnd
    auto i = _cameras.find(id);

    if (i != _cameras.end())
    {
        _cameras.erase(i);
    }

    if (_activeCam == id)
    {
        // Find a new active camera
        _activeCam = !_cameras.empty() ? _cameras.begin()->first : -1;
    }
}

void CameraWndManager::resetCameraAngles(const cmd::ArgumentList& args)
{
	doWithActiveCamWnd([](CamWnd& camWnd)
	{
		Vector3 angles;
		angles[camera::CAMERA_ROLL] = angles[camera::CAMERA_PITCH] = 0;
		angles[camera::CAMERA_YAW] = 22.5 * floor((camWnd.getCameraAngles()[camera::CAMERA_YAW] + 11) / 22.5);
		camWnd.setCameraAngles(angles);
	});
}

void CameraWndManager::increaseCameraSpeed(const cmd::ArgumentList& args) {

	int movementSpeed = registry::getValue<int>(RKEY_MOVEMENT_SPEED);
	movementSpeed *= 2;

	if (movementSpeed > MAX_CAMERA_SPEED){
		movementSpeed = MAX_CAMERA_SPEED;
	}

	registry::setValue(RKEY_MOVEMENT_SPEED, movementSpeed);
}

void CameraWndManager::decreaseCameraSpeed(const cmd::ArgumentList& args) {

	int movementSpeed = registry::getValue<int>(RKEY_MOVEMENT_SPEED);
	movementSpeed /= 2;

	if (movementSpeed < 1){
		movementSpeed = 1;
	}

	registry::setValue(RKEY_MOVEMENT_SPEED, movementSpeed);
}

void CameraWndManager::benchmark()
{
	doWithActiveCamWnd([](CamWnd& camWnd) { camWnd.benchmark(); });
}

void CameraWndManager::update()
{
    foreachCamWnd([](CamWnd& cam) { cam.update(); });
}

void CameraWndManager::forceDraw()
{
    // Issue the update call to all cameras
    foreachCamWnd([](CamWnd& cam) { cam.forceRedraw(); });
}

void CameraWndManager::changeFloorUp(const cmd::ArgumentList& args)
{
	doWithActiveCamWnd([](CamWnd& camWnd) { camWnd.changeFloor(true); });
}

void CameraWndManager::changeFloorDown(const cmd::ArgumentList& args)
{
	doWithActiveCamWnd([](CamWnd& camWnd) { camWnd.changeFloor(false); });
}

void CameraWndManager::toggleLightingMode(const cmd::ArgumentList& args) {
	getCameraSettings()->toggleLightingMode();
}

void CameraWndManager::farClipPlaneIn(const cmd::ArgumentList& args)
{
    auto newCubicScale = getCameraSettings()->cubicScale() - 1;
    getCameraSettings()->setCubicScale(newCubicScale);

    foreachCamWnd([=](CamWnd& camWnd)
    {
        camWnd.setFarClipPlaneDistance(calculateFarPlaneDistance(newCubicScale));
        camWnd.update();
    });
}

void CameraWndManager::farClipPlaneOut(const cmd::ArgumentList& args)
{
    auto newCubicScale = getCameraSettings()->cubicScale() + 1;
    getCameraSettings()->setCubicScale(newCubicScale);

    foreachCamWnd([=](CamWnd& camWnd)
    {
        camWnd.setFarClipPlaneDistance(calculateFarPlaneDistance(newCubicScale));
        camWnd.update();
    });
}

// --------------- Keyboard movement methods ------------------------------------------

void CameraWndManager::moveCameraCmd(const cmd::ArgumentList& args)
{
	auto camWnd = getActiveCamWnd();
	if (!camWnd) return;

	if (args.size() != 2)
	{
		rMessage() << "Usage: MoveCamera <up|down|forward|back|left|right> <units>" << std::endl;
		rMessage() << "Example: MoveCamera forward '20' performs"
			<< " a 20 unit move in the forward direction." << std::endl;
		return;
	}

	std::string arg = string::to_lower_copy(args[0].getString());
	double amount = args[1].getDouble();

	if (amount <= 0)
	{
		rWarning() << "Unit amount must be > 0" << std::endl;
		return;
	}

	if (arg == "up")
	{
		camWnd->moveUpDiscrete(amount);
	}
	else if (arg == "down")
	{
		camWnd->moveDownDiscrete(amount);
	}
	else if (arg == "left")
	{
		camWnd->moveLeftDiscrete(amount);
	}
	if (arg == "right")
	{
		camWnd->moveRightDiscrete(amount);
	}
	else if (arg == "forward")
	{
		camWnd->moveForwardDiscrete(amount);
	}
	else if (arg == "back")
	{
		camWnd->moveBackDiscrete(amount);
	}
	else
	{
		rWarning() << "Unknown direction: " << arg << std::endl;
		rMessage() << "Possible directions are: <up|down|forward|back|left|right>" << std::endl;
	}
}

void CameraWndManager::foreachCamWnd(const std::function<void(CamWnd&)>& action)
{
    for (auto i = _cameras.begin(); i != _cameras.end(); /* in-loop */)
    {
        auto cam = i->second;

        if (!cam)
        {
            _cameras.erase(i++);
            continue;
        }

        ++i;
        action(*cam);
    }
}

void CameraWndManager::doWithActiveCamWnd(const std::function<void(CamWnd&)>& action)
{
	auto camWnd = getActiveCamWnd();

	if (camWnd)
	{
		action(*camWnd);
	}
}

void CameraWndManager::onMoveForwardKey(ui::KeyEventType eventType)
{
	doWithActiveCamWnd([&](CamWnd& cam) { cam.onForwardKey(eventType); });
}

void CameraWndManager::onMoveBackKey(ui::KeyEventType eventType)
{
	doWithActiveCamWnd([&](CamWnd& cam) { cam.onBackwardKey(eventType); });
}

void CameraWndManager::onMoveLeftKey(ui::KeyEventType eventType)
{
	doWithActiveCamWnd([&](CamWnd& cam) { cam.onLeftKey(eventType); });
}

void CameraWndManager::onMoveRightKey(ui::KeyEventType eventType)
{
	doWithActiveCamWnd([&](CamWnd& cam) { cam.onRightKey(eventType); });
}

void CameraWndManager::onMoveUpKey(ui::KeyEventType eventType)
{
	doWithActiveCamWnd([&](CamWnd& cam) { cam.onUpKey(eventType); });
}

void CameraWndManager::onMoveDownKey(ui::KeyEventType eventType)
{
	doWithActiveCamWnd([&](CamWnd& cam) { cam.onDownKey(eventType); });
}

void CameraWndManager::moveLeftDiscrete(const cmd::ArgumentList& args)
{
	doWithActiveCamWnd([](CamWnd& cam) { cam.moveLeftDiscrete(SPEED_MOVE); });
}

void CameraWndManager::moveRightDiscrete(const cmd::ArgumentList& args)
{
	doWithActiveCamWnd([](CamWnd& cam) { cam.moveRightDiscrete(SPEED_MOVE); });
}

void CameraWndManager::pitchUpDiscrete(const cmd::ArgumentList& args)
{
	doWithActiveCamWnd([](CamWnd& cam) { cam.pitchUpDiscrete(); });
}

void CameraWndManager::pitchDownDiscrete(const cmd::ArgumentList& args)
{
	doWithActiveCamWnd([](CamWnd& cam) { cam.pitchDownDiscrete(); });
}

float CameraWndManager::getCameraStrafeSpeed()
{
    return _strafeSpeed;
}

float CameraWndManager::getCameraForwardStrafeFactor()
{
    return _forwardStrafeFactor;
}

unsigned int CameraWndManager::getStrafeModifierFlags()
{
    return _toggleStrafeModifierFlags;
}

unsigned int CameraWndManager::getStrafeForwardModifierFlags()
{
    return _toggleStrafeForwardModifierFlags;
}

ui::MouseToolStack CameraWndManager::getMouseToolsForEvent(wxMouseEvent& ev)
{
    unsigned int state = wxutil::MouseButton::GetButtonStateChangeForMouseEvent(ev);
    return GlobalMouseToolManager().getMouseToolsForEvent(ui::IMouseToolGroup::Type::CameraView, state);
}

void CameraWndManager::foreachMouseTool(const std::function<void(const ui::MouseToolPtr&)>& func)
{
    GlobalMouseToolManager().getGroup(ui::IMouseToolGroup::Type::CameraView).foreachMouseTool(func);
}

void CameraWndManager::loadCameraStrafeDefinitions()
{
    // Find all the camera strafe definitions
    xml::NodeList strafeList = GlobalRegistry().findXPath("user/ui/input/cameraview/strafemode");

    if (!strafeList.empty())
    {
        const xml::Node& node = strafeList[0];

        // Get the strafe condition flags
        _toggleStrafeModifierFlags = wxutil::Modifier::GetStateFromModifierString(node.getAttributeValue("toggle"));
        _toggleStrafeForwardModifierFlags = wxutil::Modifier::GetStateFromModifierString(node.getAttributeValue("forward"));

        _strafeSpeed = string::convert<float>(node.getAttributeValue("speed"), DEFAULT_STRAFE_SPEED);
        _forwardStrafeFactor = string::convert<float>(node.getAttributeValue("forwardFactor"), DEFAULT_FORWARD_STRAFE_FACTOR);
    }
    else
    {
        // No Camera strafe definitions found!
        rWarning() << "CameraWndManager: No camera strafe definitions found!" << std::endl;
    }
}

// RegisterableModule implementation
const std::string& CameraWndManager::getName() const
{
	static std::string _name("CameraWndManager");
	return _name;
}

const StringSet& CameraWndManager::getDependencies() const
{
    static StringSet _dependencies
    {
        MODULE_XMLREGISTRY,
        MODULE_EVENTMANAGER,
        MODULE_RENDERSYSTEM,
        MODULE_COMMANDSYSTEM,
        MODULE_MOUSETOOLMANAGER,
        MODULE_USERINTERFACE,
        MODULE_MAINFRAME,
    };

	return _dependencies;
}

void CameraWndManager::initialiseModule(const IApplicationContext& ctx)
{
	// greebo: If at startup time the render mode is set to LIGHTING, fall back
	// to textured. During startup the openGL contexts are not realised yet and the
	// openGL module is tricked into believing there are no GLSL shader programs supported.
	// Later on, when switching back to TEXTURED the rendersystem will attempt to destroy
	// program objects it never created.
	if (registry::getValue<int>(RKEY_DRAWMODE) == RENDER_MODE_LIGHTING)
	{
		registry::setValue(RKEY_DRAWMODE, RENDER_MODE_TEXTURED);
	}

	registerCommands();
    loadCameraStrafeDefinitions();

	CamWnd::captureStates();

    IMouseToolGroup& toolGroup = GlobalMouseToolManager().getGroup(IMouseToolGroup::Type::CameraView);

    toolGroup.registerMouseTool(std::make_shared<FreeMoveTool>());
	toolGroup.registerMouseTool(std::make_shared<PanViewTool>());
    toolGroup.registerMouseTool(std::make_shared<PickShaderTool>());
    toolGroup.registerMouseTool(std::make_shared<PasteShaderProjectedTool>());
    toolGroup.registerMouseTool(std::make_shared<PasteShaderNaturalTool>());
    toolGroup.registerMouseTool(std::make_shared<PasteShaderCoordsTool>());
    toolGroup.registerMouseTool(std::make_shared<PasteShaderToBrushTool>());
    toolGroup.registerMouseTool(std::make_shared<PasteShaderNameTool>());
    toolGroup.registerMouseTool(std::make_shared<JumpToObjectTool>());

    GlobalUserInterface().registerControl(std::make_shared<CameraControl>(*this));

    GlobalMainFrame().signal_MainFrameConstructed().connect([&]()
    {
        // When creating new camera views, use a floating window
        GlobalMainFrame().addControl(UserControl::Camera, { IMainFrame::Location::FloatingWindow, false });
    });
}

void CameraWndManager::shutdownModule()
{
    GlobalUserInterface().unregisterControl(UserControl::Camera);

	CamWnd::releaseStates();

	_cameras.clear();
}

// Define the static Camera module
module::StaticModuleRegistration<CameraWndManager> cameraWndManagerModule;

} // namespace

// The accessor function to the CameraWndManager instance
ui::CameraWndManager& GlobalCamera()
{
	return *std::static_pointer_cast<ui::CameraWndManager>(
		module::GlobalModuleRegistry().getModule("CameraWndManager")
	);
}
