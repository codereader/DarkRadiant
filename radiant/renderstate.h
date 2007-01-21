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

#if !defined(INCLUDED_RENDERSTATE_H)
#define INCLUDED_RENDERSTATE_H

#include "irender.h"
#include "moduleobservers.h"
#include "string/string.h"

#include <list>

void ShaderCache_setBumpEnabled(bool enabled);
void ShaderCache_extensionsInitialised();

class OpenGLState;
void OpenGLState_apply(const OpenGLState& self, 
					   OpenGLState& current, 
					   unsigned int globalstate);


class OpenGLStateBucket;

/* Sorted state map */

#include "render/backend/OpenGLStateLess.h"
#include "generic/reference.h"
#include <map>

typedef ConstReference<OpenGLState> OpenGLStateReference;
typedef std::map<OpenGLStateReference, 
				 OpenGLStateBucket*, 
				 OpenGLStateLess> OpenGLStates;

void insertSortedState(const OpenGLStates::value_type& val);
void eraseSortedState(const OpenGLStates::key_type& key);

/**
 * Load a GL Program from the given filename and load directly into OpenGL.
 */
void createARBProgram(const char* filename, GLenum type);

#endif
