/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

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

#if !defined(INCLUDED_PATCH_H)
#define INCLUDED_PATCH_H

/// \file
/// \brief The patch primitive.
///
/// A 2-dimensional matrix of vertices that define a quadratic bezier surface.
/// The Boundary-Representation of this primitive is a triangle mesh.
/// The surface is recursively tesselated until the angle between each triangle
/// edge is smaller than a specified tolerance.

#include "patch/PatchControlInstance.h"
#include "patch/PatchImportExport.h"
#include "patch/PatchInstance.h"
#include "patch/PatchNode.h"
#include "patch/PatchSceneWalk.h"

#endif
