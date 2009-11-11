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

//-----------------------------------------------------------------------------
//
// DESCRIPTION:
// camera interface
//

#if !defined(INCLUDED_ICAMERA_H)
#define INCLUDED_ICAMERA_H

#include "imodule.h"
#include "math/Vector3.h"

enum {
    CAMERA_PITCH = 0, // up / down
    CAMERA_YAW = 1, // left / right
    CAMERA_ROLL = 2, // fall over
};

/**
 * The "global" interface of DarkRadiant's camera module.
 */
class ICamera :
	public RegisterableModule
{
public:
	/**
	 * greebo: Sets the camera origin to the given <point> using the given <angles>.
	 */
	virtual void focusCamera(const Vector3& point, const Vector3& angles) = 0;
};
typedef boost::shared_ptr<ICamera> ICameraPtr;

const std::string MODULE_CAMERA("Camera");

// Accessor 
// (this is named CameraView to avoid name conflicts with the existing GlobalCamera() accessor)
inline ICamera& GlobalCameraView() {
	// Cache the reference locally
	static ICamera& _camera(
		*boost::static_pointer_cast<ICamera>(
			module::GlobalModuleRegistry().getModule(MODULE_CAMERA)
		)
	);
	return _camera;
}

class Matrix4;

class CameraView
{
public:
  virtual ~CameraView() {}
  virtual void setModelview(const Matrix4& modelview) = 0;
  virtual void setFieldOfView(float fieldOfView) = 0;
};

#endif
