#ifndef RENDERABLEVERTICES_H_
#define RENDERABLEVERTICES_H_

#include "irender.h"
#include "ieclass.h"
#include "math/Vector3.h"
#include <iostream>

#include <GL/glew.h>

namespace entity
{

/* The renderable point base class. 
 */
class RenderableVertex
	: public OpenGLRenderable 
{
	// Light centre in light space
	const Vector3& _localCentre;

	// Shader to use for the point
	ShaderPtr _shader;

protected:
	// Origin of lightvolume in world space
	const Vector3& _worldOrigin;
	
	// A reference to the colour of this point (owned by the Doom3Radius class)
	const Vector3& _colour;
	
public:
	// Constructor
	RenderableVertex(const Vector3& center, const Vector3& origin, const Vector3& colour) 
		: 	_localCentre(center), 
	  		_shader(GlobalShaderCache().capture("$BIGPOINT")),
	  		_worldOrigin(origin),
	  		_colour(colour)
	{}
  
  	Vector3 getOrigin() {
  		return _worldOrigin;
  	}
  	
  	// Return the Shader for rendering
  	ShaderPtr getShader() const {
  		return _shader;
  	}
  
	// GL render function
  	virtual void render(RenderStateFlags state) const {
		Vector3 centreWorld = _localCentre + _worldOrigin;
			
	    // Draw the center point
	    glBegin(GL_POINTS);
	    glColor3dv(_colour);
	    glVertex3dv(centreWorld);
	    glEnd();
	}	
}; // class RenderableVertex 

/** Renderable class which draws the light_center dot on lights.
 */
class RenderableLightTarget
	: public RenderableVertex
{
public:

	// Constructor
	RenderableLightTarget(const Vector3& center, const Vector3& origin, const Vector3& colour) 
		: RenderableVertex(center, origin, colour)
	{}
  
  	// Destructor
  	~RenderableLightTarget() {}
};

/** Renderable class which draws a vertex relatively to the light_origin+light_target position.
 */
class RenderableLightRelative
	: public RenderableVertex
{
	// The target vector is needed to calculate the coordinates relative to the origin 
	const Vector3& _target;
	const Vector3& _relative;
public:

	// Constructor
	RenderableLightRelative(const Vector3& relative, const Vector3& target, const Vector3& origin, const Vector3& colour) 
		: RenderableVertex(relative, origin, colour),
		  _target(target),
		  _relative(relative)
	{}
  
  	// Destructor
  	~RenderableLightRelative() {}
  	
  	// GL render function
  	void render(RenderStateFlags state) const {
  		Vector3 centreWorld = _worldOrigin + _target + _relative;
		
	    // Draw the center point
	    glBegin(GL_POINTS);
	    glColor3dv(_colour);
	    glVertex3dv(centreWorld);
	    glEnd();
	}
};

} // namespace entity


#endif /*RENDERABLEVERTICES_H_*/
