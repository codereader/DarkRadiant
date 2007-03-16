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

/* greebo: This is where the interface for other plugins is defined.
 * Functions that should be accessible via GlobalRadiant() are defined here 
 * as function pointers. The class RadiantCoreAPI in plugin.cpp makes sure
 * that these variables are pointing to the correct functions. 
 */

#ifndef __QERPLUGIN_H__
#define __QERPLUGIN_H__

#include "generic/constant.h"
#include "iclipper.h"
#include "math/Vector3.h"
#include "math/Plane3.h"

// ========================================
// GTK+ helper functions

// NOTE: parent can be 0 in all functions but it's best to set them

// this API does not depend on gtk+ or glib
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkWindow GtkWindow;

enum EMessageBoxType
{
  eMB_OK,
  eMB_OKCANCEL,
  eMB_YESNO,
  eMB_YESNOCANCEL,
  eMB_NOYES,
};

enum EMessageBoxIcon
{
  eMB_ICONDEFAULT,
  eMB_ICONERROR,
  eMB_ICONWARNING,
  eMB_ICONQUESTION,
  eMB_ICONASTERISK,
};

enum EMessageBoxReturn
{
  eIDOK,
  eIDCANCEL,
  eIDYES,
  eIDNO,
};

// simple Message Box, see above for the 'type' flags

typedef EMessageBoxReturn (* PFN_QERAPP_MESSAGEBOX) (GtkWidget *parent, const char* text, const char* caption/* = "GtkRadiant"*/, EMessageBoxType type/* = eMB_OK*/, EMessageBoxIcon icon/* = eMB_ICONDEFAULT*/);

// returns a gchar* string that must be g_free'd by the user
typedef char* (* PFN_QERAPP_DIRDIALOG) (GtkWidget *parent, const char* title/* = "Choose Directory"*/, const char* path/* = 0*/);

// return true if the user closed the dialog with 'Ok'
// 'color' is used to set the initial value and store the selected value
typedef bool (* PFN_QERAPP_COLORDIALOG) (GtkWidget *parent, Vector3& color,
                                               const char* title/* = "Choose Color"*/);

// load a .bmp file and create a GtkImage widget from it
// NOTE: 'filename' is relative to <radiant_path>/plugins/bitmaps/
typedef struct _GtkImage GtkImage;
typedef GtkImage* (* PFN_QERAPP_NEWIMAGE) (const char* filename);

// ========================================

// Forward declarations
namespace scene {
  class Node;
}

class ModuleObserver;

#include "signal/signalfwd.h"
#include "windowobserver.h"

typedef struct _GdkEventButton GdkEventButton;

typedef SignalHandler3<int, int, GdkEventButton*> MouseEventHandler;
typedef SignalFwd<MouseEventHandler>::handler_id_type MouseEventHandlerId;

// The radiant core API, formerly known as _QERFuncTable_1
// This contains pointers to all the core functions that should be available via GlobalRadiant()
struct IRadiant
{
  INTEGER_CONSTANT(Version, 1);
  STRING_CONSTANT(Name, "radiant");

	/** Return the main application GtkWindow.
	 */
	GtkWindow* (*getMainWindow) ();
	
	void (*setStatusText)(const std::string& statusText);

  const char* (*getEnginePath)();
  const char* (*getAppPath)();
  const char* (*getSettingsPath)();
  const char* (*getMapsPath)();

  const char* (*getGameName)();
  const char* (*getGameMode)();

  const char* (*getMapName)();
  scene::Node& (*getMapWorldEntity)();

  const char* (*getGameDescriptionKeyValue)(const char* key);
  const char* (*getRequiredGameDescriptionKeyValue)(const char* key);
  Vector3 (*getColour)(const std::string& colourName);
  
  void (*updateAllWindows)();
  void (*splitSelectedBrushes)(const Vector3 planePoints[3], const std::string& shader, EBrushSplit split);
  void (*brushSetClipPlane)(const Plane3& plane);

  EViewType (*XYWindow_getViewType)();
  Vector3 (*XYWindow_windowToWorld)(const WindowVector& position);
  
  const char* (*TextureBrowser_getSelectedShader)();

  // GTK+ functions
  PFN_QERAPP_MESSAGEBOX  m_pfnMessageBox;
  PFN_QERAPP_DIRDIALOG   m_pfnDirDialog;
  PFN_QERAPP_COLORDIALOG m_pfnColorDialog;
};

// RadiantCoreAPI Module Definitions
#include "modulesystem.h"

template<typename Type>
class GlobalModule;
typedef GlobalModule<IRadiant> GlobalRadiantModule;

template<typename Type>
class GlobalModuleRef;
typedef GlobalModuleRef<IRadiant> GlobalRadiantModuleRef;

inline IRadiant& GlobalRadiant() {
  return GlobalRadiantModule::getTable();
}

#endif
