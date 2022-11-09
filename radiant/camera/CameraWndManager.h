#pragma once

#include <map>
#include "icommandsystem.h"
#include "ui/ieventmanager.h"
#include "imousetoolmanager.h"

#include "CamWnd.h"

class wxWindow;

namespace ui
{

/**
 * greebo: This is the gateway class to access the currently active CamWindow
 *
 * This class provides an interface for creating and deleting CamWnd instances
 * as well as some methods that are passed to the currently active CamWnd, like
 * resetCameraAngles() or lookThroughSelected().
 **/
class CameraWndManager :
	public RegisterableModule
{
private:
    std::map<int, CamWnd*> _cameras;

	// The currently active camera window (-1 if no cam active)
	int _activeCam;

    unsigned int _toggleStrafeModifierFlags;
    unsigned int _toggleStrafeForwardModifierFlags;

    float _strafeSpeed;
    float _forwardStrafeFactor;

public:
	// Constructor
	CameraWndManager();

	/**
	 * Returns the currently active CamWnd or NULL if none is active.
	 */
	CamWnd* getActiveCamWnd();

    // Register the camera with the given ID
	void addCamWnd(int id, CamWnd* cam);
	// Remove the camwnd with the given ID
	void removeCamWnd(int id);
    // Set the CamWnd with the given ID as active one
    void setActiveCamWnd(int id);

	// Resets the camera angles of the currently active Camera
	void resetCameraAngles(const cmd::ArgumentList& args);

	// Toggles between lighting and solid rendering mode (passes the call to the CameraSettings class)
	void toggleLightingMode(const cmd::ArgumentList& args);

    // Increases/decreases the far clip plane distance (passes the call to
    // CamWnd)
	void farClipPlaneIn(const cmd::ArgumentList& args);
	void farClipPlaneOut(const cmd::ArgumentList& args);

	// Change the floor up/down, passes the call on to the CamWnd class
	void changeFloorUp(const cmd::ArgumentList& args);
	void changeFloorDown(const cmd::ArgumentList& args);

	// angua: increases and decreases the movement speed of the camera
	void increaseCameraSpeed(const cmd::ArgumentList& args);
	void decreaseCameraSpeed(const cmd::ArgumentList& args);

	// greebo: This measures the rendering time for a full 360 degrees turn of the camera
	// Note: unused at the moment
	void benchmark();

	void update();
    void forceDraw();

	// Movement commands (the calls are passed on to the Camera class)
	void moveCameraCmd(const cmd::ArgumentList& args);
	void moveLeftDiscrete(const cmd::ArgumentList& args);
	void moveRightDiscrete(const cmd::ArgumentList& args);
	void pitchUpDiscrete(const cmd::ArgumentList& args);
	void pitchDownDiscrete(const cmd::ArgumentList& args);

    // Camera strafe behaviour
    float getCameraStrafeSpeed();
    float getCameraForwardStrafeFactor();
    unsigned int getStrafeModifierFlags();
    unsigned int getStrafeForwardModifierFlags();

    MouseToolStack getMouseToolsForEvent(wxMouseEvent& ev);
    void foreachMouseTool(const std::function<void(const MouseToolPtr&)>& func);

public:
    void foreachCamWnd(const std::function<void(CamWnd&)>& action);
	
    // Callbacks for the named camera KeyEvents
	void onMoveForwardKey(KeyEventType eventType);
	void onMoveBackKey(KeyEventType eventType);
	void onMoveLeftKey(KeyEventType eventType);
	void onMoveRightKey(KeyEventType eventType);
	void onMoveUpKey(KeyEventType eventType);
	void onMoveDownKey(KeyEventType eventType);

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
	void shutdownModule() override;

private:
	// greebo: The construct method registers all the commands
	void registerCommands();
    void loadCameraStrafeDefinitions();

	void doWithActiveCamWnd(const std::function<void(CamWnd&)>& action);
};

} // namespace

// The accessor function that contains the static instance of the GlobalCameraManager class
ui::CameraWndManager& GlobalCamera();
