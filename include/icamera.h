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

#include "generic/callbackfwd.h"
#include "scenelib.h"

class Matrix4;

class CameraView
{
public:
  virtual void setModelview(const Matrix4& modelview) = 0;
  virtual void setFieldOfView(float fieldOfView) = 0;
};

class CameraModel
{
public:
	virtual void setCameraView(CameraView* view, const Callback& disconnect) = 0;
};

inline CameraModel* Instance_getCameraModel(scene::Instance& instance) {
	return dynamic_cast<CameraModel*>(&instance);
}

#endif
