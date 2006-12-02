#ifndef VERTEXINSTANCE_H_
#define VERTEXINSTANCE_H_

class VertexInstance {
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
  
	const Vector3 getVertex() const {
		return _vertex;
	} 
  
	void setSelected(const bool select) {
		_selectable.setSelected(select);
	}
  
	bool isSelected() const {
		return _selectable.isSelected();
	}

	void testSelect(Selector& selector, SelectionTest& test) {
		SelectionIntersection best;
		test.TestPoint(_vertex, best);
    
		if (best.valid()) {
			// Add the selectable to the given selector > this should trigger the callbacks
			Selector_add(selector, _selectable, best);
		}
	}
}; // class VertexInstance

#endif /*VERTEXINSTANCE_H_*/
