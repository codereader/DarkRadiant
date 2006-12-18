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

#include "brush.h"
#include "signal/signal.h"

Signal0 g_brushTextureChangedCallbacks;

void Brush_addTextureChangedCallback(const SignalHandler& handler)
{
  g_brushTextureChangedCallbacks.connectLast(handler);
}

void Brush_textureChanged()
{
  g_brushTextureChangedCallbacks();
}

QuantiseFunc Face::m_quantise;
//EBrushType Face::m_type;
//EBrushType FacePlane::m_type;
bool g_brush_texturelock_enabled = false;

//EBrushType Brush::m_type;
double Brush::m_maxWorldCoord = 0;
Shader* Brush::m_state_point;
Shader* BrushClipPlane::m_state = 0;
Shader* BrushInstance::m_state_selpoint;
Counter* BrushInstance::m_counter = 0;

FaceInstanceSet g_SelectedFaceInstances;

