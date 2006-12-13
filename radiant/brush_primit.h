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

#if !defined(INCLUDED_BRUSH_PRIMIT_H)
#define INCLUDED_BRUSH_PRIMIT_H

#include "math/Vector3.h"
#include "brush/TexDef.h"

#include "brush/BrushPrimitTexDef.h"
#include "brush/TextureProjection.h"


// Retrieves the default texture scale (from a global variable!) 
float Texdef_getDefaultTextureScale();

class TexDef;
struct Winding;
template<typename Element> class BasicVector4;
typedef BasicVector4<float> Vector4;
typedef Vector4 Quaternion;
class Matrix4;
class Plane3;

void Normal_GetTransform(const Vector3& normal, Matrix4& transform);

void TexDef_Construct_Default(TextureProjection& projection);

void Texdef_Assign(TextureProjection& projection, const TextureProjection& other);
void Texdef_Shift(TextureProjection& projection, float s, float t);
void Texdef_Scale(TextureProjection& projection, float s, float t);
void Texdef_Rotate(TextureProjection& projection, float angle);
void Texdef_FitTexture(TextureProjection& projection, std::size_t width, std::size_t height, const Vector3& normal, const Winding& w, float s_repeat, float t_repeat);
void Texdef_EmitTextureCoordinates(const TextureProjection& projection, std::size_t width, std::size_t height, Winding& w, const Vector3& normal, const Matrix4& localToWorld);

void ShiftScaleRotate_fromFace(TexDef& shiftScaleRotate, const TextureProjection& projection);
void ShiftScaleRotate_toFace(const TexDef& shiftScaleRotate, TextureProjection& projection);

void TexDefransformLocked(TextureProjection& projection, std::size_t width, std::size_t height, const Plane3& plane, const Matrix4& transform);
void Texdef_normalise(TextureProjection& projection, float width, float height);

extern float g_texdef_default_scale;

#endif
