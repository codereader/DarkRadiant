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

#if !defined(INCLUDED_IGLRENDER_H)
#define INCLUDED_IGLRENDER_H

#include "igl.h"
#include "imodule.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
class AABB;
class Matrix4;

/**
 * Representation of a GL vertex/fragment program.
 */
class GLProgram
{
public:
	
	/**
	 * Create this program using glGenProgramsARB and siblings. This needs the
	 * OpenGL system to be initialised so cannot happen in the constructor.
	 */
	virtual void create() = 0;
	
	/**
	 * Destroy this program using GL calls.
	 */
	virtual void destroy() = 0;

	/**
	 * Bind this program as the currently-operative shader.
	 */
	virtual void enable() = 0;
  
  	/**
  	 * Unbind this program from OpenGL.
  	 */
	virtual void disable() = 0;
	
	/**
	 * Provide the local parameters that should be passed to the shader program.
	 * 
	 * @param viewer
	 * Location of the viewer in 3D space.
	 * 
	 * @param localToWorld
	 * Local to world transformation matrix.
	 * 
	 * @param origin
	 * Origin of the light in 3D space.
	 * 
	 * @param colour
	 * Colour of the light in 3D space.
	 * 
	 * @param world2light
	 * Transformation from world space into light space, based on the position
	 * and transformations of the light volume.
	 * 
	 * @param ambientFactor
	 * 0.0 for a normal light, 1.0 for an ambient light. This affects whether
	 * the lighting is directional or not.
	 */
	virtual void setParameters(const Vector3& viewer, 
  							   const Matrix4& localToWorld, 
  							   const Vector3& origin, 
  							   const Vector3& colour, 
  							   const Matrix4& world2light,
  							   float ambientFactor) = 0;
};

//! A collection of opengl state information.
class OpenGLState
{
public:
  enum ESort
  {
    eSortFirst = 0,
    eSortOpaque = 1,
    eSortMultiFirst = 2,
    eSortMultiLast = 1023,
    eSortOverbrighten = 1024,
    eSortFullbright = 1025,
    eSortHighlight = 1026,
    eSortTranslucent = 1027,
    eSortOverlayFirst = 1028,
    eSortOverlayLast = 2047,
    eSortControlFirst = 2048,
    eSortControlLast = 3071,
    eSortGUI0 = 3072,
    eSortGUI1 = 3073,
    eSortLast = 4096,
  };

  unsigned int m_state;
  std::size_t m_sort;
  GLint m_texture;
  GLint m_texture1;
  GLint m_texture2;
  GLint m_texture3;
  GLint m_texture4;
  GLint m_texture5;
  GLint m_texture6;
  GLint m_texture7;
  Vector4 m_colour;
  GLenum m_blend_src, m_blend_dst;
  GLenum m_depthfunc;
  GLenum m_alphafunc;
  GLfloat m_alpharef;
  GLfloat m_linewidth;
  GLfloat m_pointsize;
  GLint m_linestipple_factor;
  GLushort m_linestipple_pattern;
  GLProgram* m_program;

	// Default constructor
	OpenGLState() 
	: m_state(0), // corresponds to RENDER_DEFAULT. TODO: potentially fragile
	  m_texture(0),
	  m_texture1(0),
	  m_texture2(0),
	  m_texture3(0),
	  m_texture4(0),
	  m_texture5(0),
	  m_texture6(0),
	  m_texture7(0),
	  m_colour(1, 1, 1, 1),
	  m_blend_src(GL_SRC_ALPHA),
	  m_blend_dst(GL_ONE_MINUS_SRC_ALPHA),
	  m_depthfunc(GL_LESS),
	  m_alphafunc(GL_ALWAYS),
	  m_alpharef(0),
	  m_linewidth(1),
	  m_pointsize(1),
	  m_linestipple_factor(1),
	  m_linestipple_pattern(0xAAAA),
	  m_program(0)
	{ }
};

const std::string MODULE_OPENGL_STATE_LIBRARY("OpenGLStateLibrary");

/**
 * Library of named OpenGLState objects.
 */
class OpenGLStateLibrary :
	public RegisterableModule
{
public:
	virtual void getDefaultState(OpenGLState& state) const = 0;

	/** 
	 * Add a named state.
	 */
	virtual void insert(const std::string& name, const OpenGLState& state) = 0;
	
	/**
	 * Erase a named state.
	 */
	virtual void erase(const std::string& name) = 0;
	
	/**
	 * Find a named state. Throws an std::runtime_error exception if the
	 * state is not found.
	 */
	virtual const OpenGLState& find(const std::string& name) = 0;
	
};

inline OpenGLStateLibrary& GlobalOpenGLStateLibrary() {
	// Cache the reference locally
	static OpenGLStateLibrary& _openGLStateLib(
		*boost::static_pointer_cast<OpenGLStateLibrary>(
			module::GlobalModuleRegistry().getModule(MODULE_OPENGL_STATE_LIBRARY)
		)
	);
	return _openGLStateLib;
}

#endif
