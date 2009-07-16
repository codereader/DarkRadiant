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

#if !defined(INCLUDED_RENDERABLE_H)
#define INCLUDED_RENDERABLE_H

#include <boost/shared_ptr.hpp>

class Shader;
typedef boost::shared_ptr<Shader> ShaderPtr;

class OpenGLRenderable;
class LightList;
class Matrix4;

/** 
 * \brief
 * Class which accepts OpenGLRenderable objects during the first pass of
 * rendering.
 * 
 * Each Renderable in the scenegraph is passed a reference to a
 * RenderableCollector, on which the Renderable sets the necessary state
 * variables and then submits its OpenGLRenderable(s) for later rendering. A
 * single Renderable may submit more than one OpenGLRenderable, with a different
 * state each time -- for instance a Renderable model class may submit each of
 * its material surfaces separately with the respective shaders set beforehand.
 * 
 * \todo
 * This class probably doesn't need to be a state machine, convert it to a 
 * single submit method with necessary parameters.
 */

class RenderableCollector
{
public:
  enum EHighlightMode
  {
    eFace = 1 << 0,
    /*! Full highlighting. */
    ePrimitive = 1 << 1,
  };

	/**
	 * Enumeration containing render styles.
	 */
	enum EStyle {
		eWireframeOnly,
		eFullMaterials
	};

	/**
	 * Push a Shader onto the internal shader stack. This is an OpenGL-style
	 * push, which does not accept an argument but duplicates the topmost
	 * stack value. The new value should be set with SetState().
	 */
	virtual void PushState() = 0;
	
	/**
	 * Pop the topmost Shader off the internal stack. This discards the value
	 * without returning it.
	 */
	virtual void PopState() = 0;
  
	/**
	 * Set the Shader to be used when rendering any subsequently-submitted
	 * OpenGLRenderable object. This shader remains in effect until it is
	 * changed with a subsequent call to SetState().
	 * 
	 * @param state
	 * The Shader to be used from now on.
	 * 
	 * @param mode
	 * The type of rendering (wireframe or shaded) that this shader should be
    * used for. Individual RenderableCollector subclasses may ignore this method
    * call if it does not use the render mode they are interested in.
	 */
	virtual void SetState(const ShaderPtr& state, EStyle mode) = 0;
	
	/**
	 * Submit an OpenGLRenderable object for rendering when the backend render
	 * pass is conducted. The object will be rendered using the Shader previous-
	 * ly set with SetState().
	 * 
	 * @param renderable
	 * The renderable object to submit.
	 * 
	 * @param world
	 * The local to world transform that should be applied to this object when
	 * it is rendered.
	 */
	virtual void addRenderable(const OpenGLRenderable& renderable, 
							   const Matrix4& world) = 0;
  
	
	/**
    * Return the render style of this RenderableCollector.
	 * 
    * TODO: If a RenderableCollector has a single style, why do we pass in an
    * EStyle parameter when setting the state with SetState()?
	 */
	virtual const EStyle getStyle() const = 0;
	
	/**
	 * Set the highlighting (selection) mode.
	 */
	virtual void Highlight(EHighlightMode mode, bool bEnable = true) = 0;
  
  	/**
  	 * Set the list of lights to be used for lighting-mode rendering. This
    * method only makes sense for RenderableCollectors that support this
    * rendering mode.
  	 * 
  	 * TODO: Use boost::shared_ptr<> here.
  	 */
    virtual void setLights(const LightList& lights) { }
};

class VolumeTest;

/** Interface class for Renderable objects. All objects which wish to be
 * rendered need to implement this interface. During the scenegraph traversal
 * for rendering, each Renderable object is passed a RenderableCollector object
 * which it can use to submit its geometry and state parameters.
 */

class Renderable
{
public:
	/** Submit renderable geometry when rendering takes place in Solid mode.
	 */
	virtual void renderSolid(RenderableCollector& collector, 
  						  	 const VolumeTest& volume) const = 0;

	/** Submit renderable geometry when rendering takes place in Wireframe
	 * mode.
	 */
	virtual void renderWireframe(RenderableCollector& collector, 
								 const VolumeTest& volume) const = 0;
  							   
	virtual void renderComponents(RenderableCollector&, const VolumeTest&) const
	{ }
	
	virtual void viewChanged() const
	{ }
};
typedef boost::shared_ptr<Renderable> RenderablePtr;

#endif
