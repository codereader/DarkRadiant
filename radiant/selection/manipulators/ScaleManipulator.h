#pragma once

#include "Scalable.h"
#include "ManipulatorBase.h"
#include "ManipulatorComponents.h"
#include "selection/Renderables.h"
#include "selection/Pivot2World.h"
#include "selection/ManipulationPivot.h"
#include "selection/BasicSelectable.h"

namespace selection
{

/**
 * The Manipulator for scale operations
 */
class ScaleManipulator : 
	public ManipulatorBase,
	public Scalable
{
private:
	ManipulationPivot& _pivot;

	ScaleFree _scaleFree;
	ScaleAxis _scaleAxis;
	RenderableArrowLine _arrowX;
	RenderableArrowLine _arrowY;
	RenderableArrowLine _arrowZ;
	RenderableQuad _quadScreen;
	selection::BasicSelectable _selectableX;
	selection::BasicSelectable _selectableY;
	selection::BasicSelectable _selectableZ;
	selection::BasicSelectable _selectableScreen;
	Pivot2World _pivot2World;

public:
	// Constructor
	ScaleManipulator(ManipulationPivot& pivot, std::size_t segments, float length);

	Type getType() const override
	{
		return Scale;
	}

	void UpdateColours();

	void render(RenderableCollector& collector, const VolumeTest& volume) override;
	void testSelect(const render::View& view, const Matrix4& pivot2world) override;
	Component* getActiveComponent() override;

	void setSelected(bool select) override;
	bool isSelected() const override;

	void scale(const Vector3& scaling) override;

}; // class ScaleManipulator

}
