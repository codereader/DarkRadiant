#pragma once

#include "iregistry.h"
#include "iselection.h"
#include "iselectiontest.h"
#include "math/Vector3.h"
#include "ObservedSelectable.h"
#include "EntitySettings.h"

class VertexInstance :
	public ISelectable
{
protected:
	Vector3& _vertex;

	// The Selectable
	selection::ObservedSelectable _selectable;

	Vector3 _colour;

public:
	// Construct the instance with the given <vertex> coordinates and connect the selectionChangeCallback
	VertexInstance(Vector3& vertex, const SelectionChangedSlot& observer) :
		_vertex(vertex),
		_selectable(observer),
		_colour(entity::EntitySettings::InstancePtr()->getLightVertexColour(LightEditVertexType::Deselected))
	{}

	void setVertex(const Vector3& vertex) {
		_vertex = vertex;
	}

	virtual const Vector3 getVertex() const {
		return _vertex;
	}

	void setSelected(bool select) {
		_selectable.setSelected(select);
		// Change the colour according to the selection
		_colour = entity::EntitySettings::InstancePtr()->getLightVertexColour(
			select ? LightEditVertexType::Selected : LightEditVertexType::Deselected
		);
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

		if (best.isValid()) {
			// Add the selectable to the given selector > this should trigger the callbacks
			Selector_add(selector, *this, best);
		}
	}
};

/* This is the vertexinstance class for the light_right and light_up vertex, as they
 * are calculated relatively to the light_target, which in turn is relative to the light origin
 */
class VertexInstanceRelative : public VertexInstance {
	Vector3& _origin;
public:
	// Construct the instance with the given <vertex> coordinates and connect the selectionChangeCallback
	VertexInstanceRelative(Vector3& relativeToOrigin, Vector3& origin, const SelectionChangedSlot& observer)
		: VertexInstance(relativeToOrigin, observer),
		  _origin(origin)
	{}

	const Vector3 getVertex() const override
    {
		return _origin + _vertex;
	}

	void testSelect(Selector& selector, SelectionTest& test) override
    {
		SelectionIntersection best;
		Vector3 testVertex = _origin + _vertex;
		test.TestPoint(testVertex, best);

		if (best.isValid()) {
			// Add the selectable to the given selector > this should trigger the callbacks
			Selector_add(selector, *this, best);
		}
	}
};
