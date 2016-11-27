#pragma once

#include "ManipulatorBase.h"
#include "ManipulatorComponents.h"
#include "../Renderables.h"
#include "../Pivot2World.h"

#include "../BasicSelectable.h"

namespace selection
{

/**
 * The Manipulator for scale operations
 */
class ScaleManipulator : 
	public ManipulatorBase
{
private:
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
	Pivot2World _pivot;

public:
	// Constructor
	ScaleManipulator(Scalable& scalable, std::size_t segments, float length);

	Type getType() const override
	{
		return Scale;
	}

	Pivot2World& getPivot()
	{
		return _pivot;
	}

	void UpdateColours();

	void render(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& pivot2world) override;
	void testSelect(const render::View& view, const Matrix4& pivot2world) override;
	Component* getActiveComponent() override;

	void setSelected(bool select) override;
	bool isSelected() const override;

}; // class ScaleManipulator

}
