#ifndef CURVEEDITINSTANCE_H_
#define CURVEEDITINSTANCE_H_

#include "Curve.h"
#include "selectionlib.h"

namespace entity {
	
	const Colour4b colour_vertex(0, 255, 0, 255);
	const Colour4b colour_selected(0, 0, 255, 255);
	
	struct CurveShaders {
		ShaderPtr controlsShader;
		ShaderPtr selectedShader;
	};

/** greebo: This class is wrapped around a Curve class to manage 
 * 			all the selection and transformation operations. 
 */
class CurveEditInstance
{
public:
	class ControlPointFunctor
	{
	public:
		virtual void operator()(Vector3& point, const Vector3& original) = 0;
	};
	
	class ControlPointConstFunctor
	{
	public:
		virtual void operator()(const Vector3& point, const Vector3& original) = 0;
	};
	
private:
	// The associated curve
	Curve& _curve;

	SelectionChangeCallback _selectionChanged;
	ControlPoints& _controlPointsTransformed;
	const ControlPoints& _controlPoints;
	typedef std::vector<ObservedSelectable> Selectables;
	Selectables _selectables;

	RenderablePointVector m_controlsRender;
	mutable RenderablePointVector m_selectedRender;

public:
	typedef Static<CurveShaders> StaticShaders;

	// Constructor
	CurveEditInstance(Curve& curve, const SelectionChangeCallback& selectionChanged);

	// Traversal functions, these cycle through all (selected) control points
	void forEach(ControlPointFunctor& functor);
	void forEachSelected(ControlPointFunctor& functor);
	void forEachSelected(ControlPointConstFunctor& functor) const;
	
	void testSelect(Selector& selector, SelectionTest& test);

	bool isSelected() const;
	void setSelected(bool selected);
	unsigned int numSelected() const;

	void write(const std::string& key, Entity& entity);

	// Transforms the (selected) control points
	void transform(const Matrix4& matrix, bool selectedOnly = true);
	
	// Snaps the selected control points to the grid
	void snapto(float snap);

	void updateSelected() const;
  
	void renderComponents(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const;

	void renderComponentsSelected(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const;

	void curveChanged();
	typedef MemberCaller<CurveEditInstance, &CurveEditInstance::curveChanged> CurveChangedCaller;
	
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

#endif /*CURVEEDITINSTANCE_H_*/
