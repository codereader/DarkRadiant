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

#ifndef IRADIANT_H_
#define IRADIANT_H_

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

  const char* (*getGameDescriptionKeyValue)(const char* key);
  const char* (*getRequiredGameDescriptionKeyValue)(const char* key);
  Vector3 (*getColour)(const std::string& colourName);
  
  void (*updateAllWindows)();
  void (*splitSelectedBrushes)(const Vector3 planePoints[3], const std::string& shader, EBrushSplit split);
  void (*brushSetClipPlane)(const Plane3& plane);

  const char* (*TextureBrowser_getSelectedShader)();

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
