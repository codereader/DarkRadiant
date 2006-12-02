#ifndef RENDERABLELIGHTCENTRE_H_
#define RENDERABLELIGHTCENTRE_H_

#include "irender.h"
#include "ieclass.h"
#include "math/Vector3.h"
#include <iostream>

#include <GL/glew.h>

namespace entity
{

/** Renderable class which draws the light_center dot on lights.
 */

class RenderableLightCentre
: public OpenGLRenderable
{
	// Light centre in light space
	const Vector3& _localCentre;
	
	// Origin of lightvolume in world space
	const Vector3& _worldOrigin;
	
	// Rotation of lightvolume in world space
	const Matrix4& _rotation;
	
	// Class of light entity
	IEntityClass& _eclass;

	// Shader to use for the point
	Shader* _shader;
	
	// A reference to the colour of this point (owned by the Doom3Radius class)
	const Vector3& _colour;	
	
public:

	// Constructor
	RenderableLightCentre(const Vector3& center, const Vector3& origin, const Vector3& colour, 
						  const Matrix4& rotation, IEntityClass& eclass) 
	: _localCentre(center), 
	  _worldOrigin(origin),
	  _rotation(rotation),
	  _eclass(eclass), 
	  _shader(GlobalShaderCache().capture("$BIGPOINT")),
	  _colour(colour)
	{}
  
  	// Destructor
  	~RenderableLightCentre() {
  		GlobalShaderCache().release("$BIGPOINT");
  	}
  	
  	// Return the Shader for rendering
  	Shader* getShader() const {
  		return _shader;
  	}
  
	// GL render function
  	void render(RenderStateFlags state) const {
		// Apply rotation matrix to the center point coordinates
//	    Vector3 rotCentre = _rotation.transform(_localCentre).getProjected();
//		std::cout << "rotated centre = " << rotCentre << std::endl;

		// Translate centrepoint by light origin to get coordinates
		// in world space.
//		Vector3 centreWorld = rotCentre + _worldOrigin;
		Vector3 centreWorld = _localCentre + _worldOrigin;
			
	    // Draw the center point
	    glBegin(GL_POINTS);
	    //glColor3fv(_eclass.getColour());
	    glColor3fv(_colour);
	    glVertex3fv(centreWorld);
	    glEnd();
	}
	
};

} // namespace entity

#endif /*RENDERABLELIGHTCENTRE_H_*/
