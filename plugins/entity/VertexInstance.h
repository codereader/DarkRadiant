#ifndef VERTEXINSTANCE_H_
#define VERTEXINSTANCE_H_

#include "irender.h"
#include "iuimanager.h"
#include "iregistry.h"
#include "iselection.h"
#include "math/Vector3.h"

class VertexInstance :
	public OpenGLRenderable,
	public Selectable
{
protected:
	Vector3& _vertex;

	// The Selectable
	ObservedSelectable _selectable;
	
	Vector3 _colour;

	// Shader to use for the point
	ShaderPtr _shader;

public:
	// Construct the instance with the given <vertex> coordinates and connect the selectionChangeCallback 
	VertexInstance(Vector3& vertex, const SelectionChangeCallback& observer) : 
		_vertex(vertex), 
		_selectable(observer),
		_colour(ColourSchemes().getColour("light_vertex_deselected")),
		_shader(GlobalShaderCache().capture("$BIGPOINT"))
	{}
  
	void setVertex(const Vector3& vertex) {
		_vertex = vertex;
	}
  
	virtual const Vector3 getVertex() const {
		return _vertex;
	} 
	
	// Return the Shader for rendering
  	ShaderPtr getShader() const {
  		return _shader;
  	}
  
	void setSelected(bool select) {
		_selectable.setSelected(select);
		// Change the colour according to the selection
		_colour = (select) ? 
			ColourSchemes().getColour("light_vertex_selected") : 
			ColourSchemes().getColour("light_vertex_deselected");
	}
  
	bool isSelected() const {
		return _selectable.isSelected();
	}
	
	void invertSelected() {
		setSelected(!isSelected());
	}

	virtual void testSelect(Selector& selector, SelectionTest& test) {
		SelectionIntersection best;
		test.TestPoint(_vertex, best);
    
		if (best.valid()) {
			// Add the selectable to the given selector > this should trigger the callbacks
			Selector_add(selector, *this, best);
		}
	}
	
	// Front-end render function
	void render(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const {
		renderer.Highlight(Renderer::ePrimitive, false);
		renderer.Highlight(Renderer::eFace, false);
		renderer.SetState(_shader, Renderer::eFullMaterials);
		renderer.SetState(_shader, Renderer::eWireframeOnly);
		
		renderer.addRenderable(*this, localToWorld);
	}
	
	// GL render function (backend)
  	virtual void render(RenderStateFlags state) const {
		// Draw the center point
	    glBegin(GL_POINTS);
	    glColor3dv(_colour);
	    glVertex3dv(_vertex);
	    glEnd();
	}
}; // class VertexInstance

/* This is the vertexinstance class for the light_right and light_up vertex, as they
 * are calculated relatively to the light_target, which in turn is relative to the light origin
 */
class VertexInstanceRelative : public VertexInstance {
	Vector3& _origin;
public:
	// Construct the instance with the given <vertex> coordinates and connect the selectionChangeCallback 
	VertexInstanceRelative(Vector3& relativeToOrigin, Vector3& origin, const SelectionChangeCallback& observer)
		: VertexInstance(relativeToOrigin, observer),
		  _origin(origin)
	{}
  
	const Vector3 getVertex() const {
		return _origin + _vertex;
	} 
  
	void testSelect(Selector& selector, SelectionTest& test) {
		SelectionIntersection best;
		Vector3 testVertex = _origin + _vertex;
		test.TestPoint(testVertex, best);
    
		if (best.valid()) {
			// Add the selectable to the given selector > this should trigger the callbacks
			Selector_add(selector, *this, best);
		}
	}
}; // class VertexInstanceRelative

#endif /*VERTEXINSTANCE_H_*/
