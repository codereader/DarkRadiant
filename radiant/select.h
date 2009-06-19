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

#if !defined(INCLUDED_SELECT_H)
#define INCLUDED_SELECT_H

#include "icommandsystem.h"

void Selection_Flipx(const cmd::ArgumentList& args);
void Selection_Flipy(const cmd::ArgumentList& args);
void Selection_Flipz(const cmd::ArgumentList& args);
void Selection_Rotatex(const cmd::ArgumentList& args);
void Selection_Rotatey(const cmd::ArgumentList& args);
void Selection_Rotatez(const cmd::ArgumentList& args);

void Selection_MoveDown(const cmd::ArgumentList& args);
void Selection_MoveUp(const cmd::ArgumentList& args);

#endif
