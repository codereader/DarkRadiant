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
typedef struct _GdkPixbuf GdkPixbuf;

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

enum CounterType {
	counterBrushes,
	counterPatches,
	counterEntities
};

class ICounter;

/** greebo: This abstract class defines the interface to the core application.
 * 			Use this to access methods from the main codebase in radiant/
 */
class IRadiant
{
public:
	INTEGER_CONSTANT(Version, 1);
	STRING_CONSTANT(Name, "radiant");

	/** Return the main application GtkWindow.
	 */
	virtual GtkWindow* getMainWindow() = 0;
	
	// Convenience functions to load a local image (from the bitmaps directory)
	// and return a GdkPixBuf for use by certain GTK widgets (e.g. TreeView).
	virtual GdkPixbuf* getLocalPixbuf(const std::string& fileName) = 0;
	virtual GdkPixbuf* getLocalPixbufWithMask(const std::string& fileName) = 0;
	
	// Returns the Counter object of the given type
	virtual ICounter& getCounter(CounterType counter) = 0;
	
	/** greebo: Set the status text of the main window
	 */
	virtual void setStatusText(const std::string& statusText) = 0;
	
	virtual const char* getGameDescriptionKeyValue(const char* key) = 0;
	virtual const char* getRequiredGameDescriptionKeyValue(const char* key) = 0;
	
	virtual Vector3 getColour(const std::string& colourName) = 0;
  
	virtual void updateAllWindows() = 0;
	
	// Functions needed for the clipper, this should be moved back into the core app
	virtual void splitSelectedBrushes(const Vector3 planePoints[3], const std::string& shader, EBrushSplit split) = 0;
	virtual void brushSetClipPlane(const Plane3& plane) = 0;

	virtual const char* TextureBrowser_getSelectedShader() = 0;
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
