#ifndef VERTEXINSTANCE_H_
#define VERTEXINSTANCE_H_

class VertexInstance {
protected:
	Vector3& _vertex;

	// The Selectable
	ObservedSelectable _selectable;

public:
	// Construct the instance with the given <vertex> coordinates and connect the selectionChangeCallback 
	VertexInstance(Vector3& vertex, const SelectionChangeCallback& observer)
		: _vertex(vertex), _selectable(observer)
	{}
  
	void setVertex(const Vector3& vertex) {
		_vertex = vertex;
	}
  
	virtual const Vector3 getVertex() const {
		return _vertex;
	} 
  
	void setSelected(const bool select) {
		_selectable.setSelected(select);
	}
  
	bool isSelected() const {
		return _selectable.isSelected();
	}

	virtual void testSelect(Selector& selector, SelectionTest& test) {
		SelectionIntersection best;
		test.TestPoint(_vertex, best);
    
		if (best.valid()) {
			// Add the selectable to the given selector > this should trigger the callbacks
			Selector_add(selector, _selectable, best);
		}
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
			Selector_add(selector, _selectable, best);
		}
	}
}; // class VertexInstanceRelative

#endif /*VERTEXINSTANCE_H_*/
