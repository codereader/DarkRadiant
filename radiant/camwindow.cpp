/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

//
// Camera Window
//
// Leonardo Zide (leo@lokigames.com)
//

#include "camwindow.h"

#include "debugging/debugging.h"

#include "iscenegraph.h"
#include "irender.h"
#include "iregistry.h"
#include "igl.h"
#include "icamera.h"
#include "cullable.h"
#include "renderable.h"
#include "preferencesystem.h"

#include "signal/signal.h"
#include "container/array.h"
#include "scenelib.h"
#include "render.h"
#include "cmdlib.h"
#include "math/frustum.h"

#include "gtkutil/widget.h"
#include "gtkutil/button.h"
#include "gtkutil/glwidget.h"
#include "gtkutil/xorrectangle.h"
#include "gtkmisc.h"
#include "selection/RadiantWindowObserver.h"
#include "mainframe.h"
#include "preferences.h"
#include "commands.h"
#include "xywindow.h"
#include "windowobservers.h"
#include "renderstate.h"
#include "plugin.h"

#include "ui/eventmapper/EventMapper.h"
#include "camera/CameraSettings.h"
#include "camera/CamRenderer.h"
#include "camera/CamWnd.h"

void GlobalCamera_setCamWnd(CamWnd& camwnd)
{
  GlobalCamWnd() = &camwnd;
}

ToggleShown g_camera_shown(true);

void CamWnd_setParent(CamWnd& camwnd, GtkWindow* parent)
{
  camwnd.m_parent = parent;
  g_camera_shown.connect(GTK_WIDGET(camwnd.m_parent));
}

camwindow_globals_t g_camwindow_globals;

const Vector3& Camera_getOrigin(CamWnd& camwnd)
{
  return camwnd.getCamera().getOrigin();
}

void Camera_setOrigin(CamWnd& camwnd, const Vector3& origin)
{
  camwnd.getCamera().setOrigin(origin);
}

const Vector3& Camera_getAngles(CamWnd& camwnd)
{
  return camwnd.getCamera().getAngles();
}

void Camera_setAngles(CamWnd& camwnd, const Vector3& angles)
{
  camwnd.getCamera().setAngles(angles);
}


// =============================================================================
// CamWnd class

void fill_view_camera_menu(GtkMenu* menu)
{
  create_check_menu_item_with_mnemonic(menu, "Camera View", "ToggleCamera");
}

void GlobalCamera_ResetAngles()
{
  CamWnd& camwnd = *GlobalCamWnd();
  Vector3 angles;
  angles[CAMERA_ROLL] = angles[CAMERA_PITCH] = 0;
  angles[CAMERA_YAW] = static_cast<float>(22.5 * floor((Camera_getAngles(camwnd)[CAMERA_YAW]+11)/22.5));
  Camera_setAngles(camwnd, angles);
}

void Camera_ChangeFloorUp()
{
  //CamWnd& camwnd = *GlobalCamWnd();
  GlobalCamWnd()->Cam_ChangeFloor(true);
}

void Camera_ChangeFloorDown()
{
  //CamWnd& camwnd = *GlobalCamWnd();
  GlobalCamWnd()->Cam_ChangeFloor(false);
}

void Camera_CubeIn()
{
  CamWnd& camwnd = *GlobalCamWnd();
  g_camwindow_globals.m_nCubicScale--;
  if (g_camwindow_globals.m_nCubicScale < 1)
    g_camwindow_globals.m_nCubicScale = 1;
  camwnd.getCamera().updateProjection();
  camwnd.update();
  g_pParentWnd->SetGridStatus();
}

void Camera_CubeOut()
{
  CamWnd& camwnd = *GlobalCamWnd();
  g_camwindow_globals.m_nCubicScale++;
  if (g_camwindow_globals.m_nCubicScale > 23)
    g_camwindow_globals.m_nCubicScale = 23;
  camwnd.getCamera().updateProjection();
  camwnd.update();
  g_pParentWnd->SetGridStatus();
}

bool Camera_GetFarClip() {
	return (GlobalRegistry().get("user/ui/camera/enableCubicClipping") == "1");
}

void CubicClippingExport(const BoolImportCallback& importCallback) {
	importCallback(GlobalRegistry().get("user/ui/camera/enableCubicClipping") == "1");
}

FreeCaller1<const BoolImportCallback&, CubicClippingExport> cubicClippingCaller;
BoolExportCallback cubicClippingButtonCallBack(cubicClippingCaller);
ToggleItem g_getfarclip_item(cubicClippingButtonCallBack);

void Camera_SetFarClip(bool value) {
	CamWnd& camwnd = *GlobalCamWnd();
  
	GlobalRegistry().set("user/ui/camera/enableCubicClipping", value ? "1" : "0");
  
	g_getfarclip_item.update();
	camwnd.getCamera().updateProjection();
	camwnd.update();
}

void Camera_ToggleFarClip() {
	Camera_SetFarClip(!Camera_GetFarClip());
}

void CamWnd_registerShortcuts()
{
  toggle_add_accelerator("ToggleCubicClip");
  
  if(g_pGameDescription->mGameType == "doom3")
  {
    command_connect_accelerator("TogglePreview");
  }
}


void GlobalCamera_Benchmark()
{
  CamWnd& camwnd = *GlobalCamWnd();
  camwnd.BenchMark();
}

void GlobalCamera_Update()
{
  CamWnd& camwnd = *GlobalCamWnd();
  camwnd.update();
}

/*camera_draw_mode CamWnd_GetMode()
{
  return Camera::draw_mode;
}*/
/*void CamWnd_SetMode(camera_draw_mode mode)
{
  ShaderCache_setBumpEnabled(mode == cd_lighting);
  Camera::draw_mode = mode;
  if (GlobalCamWnd() != 0) {
  		GlobalCamWnd()->update();
  }
}*/

void CamWnd_TogglePreview(void)
{
  // gametype must be doom3 for this function to work
  // if the gametype is not doom3 something is wrong with the
  // global command list or somebody else calls this function.
  ASSERT_MESSAGE(g_pGameDescription->mGameType == "doom3", "CamWnd_TogglePreview called although mGameType is not doom3 compatible");

  // switch between textured and lighting mode
  CamWnd::setMode((CamWnd::getMode() == cd_lighting) ? cd_texture : cd_lighting);
}


CameraModel* g_camera_model = 0;

void CamWnd_LookThroughCamera(CamWnd& camwnd)
{
  if(g_camera_model != 0)
  {
    camwnd.addHandlersMove();
    g_camera_model->setCameraView(0, Callback());
    g_camera_model = 0;
    camwnd.getCamera().updateModelview();
    camwnd.getCamera().updateProjection();
    camwnd.update();
  }
}

inline CameraModel* Instance_getCameraModel(scene::Instance& instance)
{
  return InstanceTypeCast<CameraModel>::cast(instance);
}

void CamWnd_LookThroughSelected(CamWnd& camwnd)
{
  if(g_camera_model != 0)
  {
    CamWnd_LookThroughCamera(camwnd);
  }

  if(GlobalSelectionSystem().countSelected() != 0)
  {
    scene::Instance& instance = GlobalSelectionSystem().ultimateSelected();
    CameraModel* cameraModel = Instance_getCameraModel(instance);
    if(cameraModel != 0)
    {
      camwnd.removeHandlersMove();
      g_camera_model = cameraModel;
      g_camera_model->setCameraView(&camwnd.getCameraView(), ReferenceCaller<CamWnd, CamWnd_LookThroughCamera>(camwnd));
    }
  }
}

void GlobalCamera_LookThroughSelected()
{
	CamWnd_LookThroughSelected(*GlobalCamWnd());
}

void GlobalCamera_LookThroughCamera()
{
  CamWnd_LookThroughCamera(*GlobalCamWnd());
}


void RenderModeImport(int value)
{
  switch(value)
  {
  case 0:
    CamWnd::setMode(cd_wire);
    break;
  case 1:
    CamWnd::setMode(cd_solid);
    break;
  case 2:
    CamWnd::setMode(cd_texture);
    break;
  case 3:
    CamWnd::setMode(cd_lighting);
    break;
  default:
    CamWnd::setMode(cd_texture);
  }
}
typedef FreeCaller1<int, RenderModeImport> RenderModeImportCaller;

void RenderModeExport(const IntImportCallback& importer)
{
	switch (CamWnd::getMode()) {
		case cd_wire:
			importer(0);
			break;
		case cd_solid:
			importer(1);
			break;
		case cd_texture:
			importer(2);
			break;
		case cd_lighting:
			importer(3);
			break;
	}
}

typedef FreeCaller1<const IntImportCallback&, RenderModeExport> RenderModeExportCaller;

void Camera_constructPreferences(PreferencesPage& page)
{
  page.appendSlider("Movement Speed", g_camwindow_globals_private.m_nMoveSpeed, TRUE, 0, 0, 100, 50, 300, 1, 10, 10);
  page.appendSlider("Rotation Speed", g_camwindow_globals_private.m_nAngleSpeed, TRUE, 0, 0, 3, 1, 180, 1, 10, 10);
    
	// Add the checkboxes and connect them with the registry key and the according observer 
	page.appendCheckBox("", "Discrete movement (non-freelook mode)", "user/ui/camera/discreteMovement", getCameraSettings());
	page.appendCheckBox("", "Enable far-clip plane (hides distant objects)", "user/ui/camera/enableCubicClipping", getCameraSettings());
	
	// Add the "inverse mouse vertical axis in free-look mode" preference
	page.appendCheckBox("", "Invert mouse vertical axis (freelook mode)", "user/ui/camera/invertMouseVerticalAxis", getCameraSettings());

  if(g_pGameDescription->mGameType == "doom3")
  {
    const char* render_mode[] = { "Wireframe", "Flatshade", "Textured", "Lighting" };

    page.appendCombo(
      "Render Mode",
      STRING_ARRAY_RANGE(render_mode),
      IntImportCallback(RenderModeImportCaller()),
      IntExportCallback(RenderModeExportCaller())
    );
  }
  else
  {
    const char* render_mode[] = { "Wireframe", "Flatshade", "Textured" };

    page.appendCombo(
      "Render Mode",
      STRING_ARRAY_RANGE(render_mode),
      IntImportCallback(RenderModeImportCaller()),
      IntExportCallback(RenderModeExportCaller())
    );
  }
}
void Camera_constructPage(PreferenceGroup& group)
{
  PreferencesPage page(group.createPage("Camera", "Camera View Preferences"));
  Camera_constructPreferences(page);
}
void Camera_registerPreferencesPage()
{
  PreferencesDialog_addSettingsPage(FreeCaller1<PreferenceGroup&, Camera_constructPage>());
}

#include "preferencesystem.h"
#include "stringio.h"
#include "dialog.h"

/// \brief Initialisation for things that have the same lifespan as this module.
void CamWnd_Construct()
{
	
  GlobalCommands_insert("CenterView", FreeCaller<GlobalCamera_ResetAngles>(), Accelerator(GDK_End));

  GlobalToggles_insert("ToggleCubicClip", FreeCaller<Camera_ToggleFarClip>(), ToggleItem::AddCallbackCaller(g_getfarclip_item), Accelerator('\\', (GdkModifierType)GDK_CONTROL_MASK));
  GlobalCommands_insert("CubicClipZoomIn", FreeCaller<Camera_CubeIn>(), Accelerator('[', (GdkModifierType)GDK_CONTROL_MASK));
  GlobalCommands_insert("CubicClipZoomOut", FreeCaller<Camera_CubeOut>(), Accelerator(']', (GdkModifierType)GDK_CONTROL_MASK));

  GlobalCommands_insert("UpFloor", FreeCaller<Camera_ChangeFloorUp>(), Accelerator(GDK_Prior));
  GlobalCommands_insert("DownFloor", FreeCaller<Camera_ChangeFloorDown>(), Accelerator(GDK_Next));

  GlobalToggles_insert("ToggleCamera", ToggleShown::ToggleCaller(g_camera_shown), ToggleItem::AddCallbackCaller(g_camera_shown.m_item), Accelerator('C', (GdkModifierType)(GDK_SHIFT_MASK|GDK_CONTROL_MASK)));
  GlobalCommands_insert("LookThroughSelected", FreeCaller<GlobalCamera_LookThroughSelected>());
  GlobalCommands_insert("LookThroughCamera", FreeCaller<GlobalCamera_LookThroughCamera>());

  if(g_pGameDescription->mGameType == "doom3")
  {
    GlobalCommands_insert("TogglePreview", FreeCaller<CamWnd_TogglePreview>(), Accelerator(GDK_F3));
  }

  GlobalShortcuts_insert("CameraForward", Accelerator(GDK_Up));
  GlobalShortcuts_insert("CameraBack", Accelerator(GDK_Down));
  GlobalShortcuts_insert("CameraLeft", Accelerator(GDK_Left));
  GlobalShortcuts_insert("CameraRight", Accelerator(GDK_Right));
  GlobalShortcuts_insert("CameraStrafeRight", Accelerator(GDK_period));
  GlobalShortcuts_insert("CameraStrafeLeft", Accelerator(GDK_comma));

  GlobalShortcuts_insert("CameraUp", Accelerator('D'));
  GlobalShortcuts_insert("CameraDown", Accelerator('C'));
  GlobalShortcuts_insert("CameraAngleUp", Accelerator('A'));
  GlobalShortcuts_insert("CameraAngleDown", Accelerator('Z'));

  GlobalShortcuts_insert("CameraFreeMoveForward", Accelerator(GDK_Up));
  GlobalShortcuts_insert("CameraFreeMoveBack", Accelerator(GDK_Down));
  GlobalShortcuts_insert("CameraFreeMoveLeft", Accelerator(GDK_Left));
  GlobalShortcuts_insert("CameraFreeMoveRight", Accelerator(GDK_Right));

  GlobalPreferenceSystem().registerPreference("MoveSpeed", IntImportStringCaller(g_camwindow_globals_private.m_nMoveSpeed), IntExportStringCaller(g_camwindow_globals_private.m_nMoveSpeed));
  GlobalPreferenceSystem().registerPreference("AngleSpeed", IntImportStringCaller(g_camwindow_globals_private.m_nAngleSpeed), IntExportStringCaller(g_camwindow_globals_private.m_nAngleSpeed));
  GlobalPreferenceSystem().registerPreference("CubicScale", IntImportStringCaller(g_camwindow_globals.m_nCubicScale), IntExportStringCaller(g_camwindow_globals.m_nCubicScale));
  GlobalPreferenceSystem().registerPreference("CameraRenderMode", makeIntStringImportCallback(RenderModeImportCaller()), makeIntStringExportCallback(RenderModeExportCaller()));

  CamWnd_constructStatic();

  Camera_registerPreferencesPage();
}
void CamWnd_Destroy()
{
  CamWnd_destroyStatic();
}
