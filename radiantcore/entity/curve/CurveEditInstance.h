#pragma once

#include "Curve.h"
#include "ObservedSelectable.h"

namespace entity
{

/** greebo: This class is wrapped around a Curve class to manage
 * the selection and transformation operations. 
 * This should be merged with the Curve class.
 */
class CurveEditInstance: public sigc::trackable
{
public:
	class ControlPointFunctor
	{
	public:
	    virtual ~ControlPointFunctor() {}
		virtual void operator()(Vector3& point, const Vector3& original) = 0;
	};

	class ControlPointConstFunctor
	{
	public:
	    virtual ~ControlPointConstFunctor() {}
		virtual void operator()(const Vector3& point, const Vector3& original) = 0;
	};

private:
	// The associated curve
	Curve& _curve;

	SelectionChangedSlot _selectionChanged;
	ControlPoints& _controlPointsTransformed;
	const ControlPoints& _controlPoints;
	typedef std::vector<selection::ObservedSelectable> Selectables;
	Selectables _selectables;

public:

	// Constructor
	CurveEditInstance(Curve& curve, const SelectionChangedSlot& selectionChanged);
	// Traversal functions, these cycle through all (selected) control points
	void forEach(ControlPointFunctor& functor);

    // Iterate all (transformed) control points and their selection status
	void forEachControlPoint(const std::function<void(const Vector3&, bool)>& functor) const;

	void forEachSelected(ControlPointFunctor& functor);
	void forEachSelected(ControlPointConstFunctor& functor) const;

	void testSelect(Selector& selector, SelectionTest& test);

	bool isSelected() const;
	void setSelected(bool selected);
	void invertSelected();
	unsigned int numSelected() const;

	void write(const std::string& key, Entity& entity);

	// Transforms the (selected) control points
	void transform(const Matrix4& matrix, bool selectedOnly = true);

	// Snaps the selected control points to the grid
	void snapto(float snap);

	void curveChanged();

	// As the name states, removes the selected control points
	void removeSelectedControlPoints();

	/** greebo: This inserts a control point BEFORE each
	 * 			selected control point of the curve.
	 * 			Naturally, this doesn't work if the first vertex
	 * 			is selected.
	 */
	void insertControlPointsAtSelected();

private:
	// Returns a list with the iterators to the selected control points.
	// This is based on the "transformed" working set.
	Curve::IteratorList getSelected();
};

} // namespace entity
