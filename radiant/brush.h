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

#if !defined(INCLUDED_BRUSH_H)
#define INCLUDED_BRUSH_H

/// \file
/// \brief The brush primitive.
///
/// A collection of planes that define a convex polyhedron.
/// The Boundary-Representation of this primitive is a manifold polygonal mesh.
/// Each face polygon is represented by a list of vertices in a \c Winding.
/// Each vertex is associated with another face that is adjacent to the edge
/// formed by itself and the next vertex in the winding. This information can
/// be used to find edge-pairs and vertex-rings.

#include "signal/isignal.h"

#include "brush/FaceShader.h"
#include "brush/ContentsFlagsValue.h"
#include "brush/FaceTexDef.h"
#include "brush/PlanePoints.h"
#include "brush/FacePlane.h"
#include "brush/Face.h"
#include "brush/SelectableComponents.h"
#include "brush/BrushClass.h"
#include "brush/VectorLightList.h"
#include "brush/FaceInstance.h"
#include "brush/BrushClipPlane.h"
#include "brush/EdgeInstance.h"
#include "brush/VertexInstance.h"
#include "brush/BrushInstance.h"
#include "brush/BrushVisit.h"

#endif
