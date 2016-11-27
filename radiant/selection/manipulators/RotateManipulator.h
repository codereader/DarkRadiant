#pragma once

#include "ManipulatorBase.h"
#include "ManipulatorComponents.h"
#include "../Renderables.h"
#include "../Pivot2World.h"
#include "../BasicSelectable.h"

namespace selection
{

/**
 * Manipulator for performing rotations with the mouse.
 *
 * Draws circles for rotation around individual axes, plus a circle for free
 * rotation.
 */
class RotateManipulator : 
	public ManipulatorBase
{
private:
	RotateFree _rotateFree;
	RotateAxis _rotateAxis;
	Vector3 _axisScreen;
	RenderableSemiCircle _circleX;
	RenderableSemiCircle _circleY;
	RenderableSemiCircle _circleZ;
	RenderableCircle _circleScreen;
	RenderableCircle _circleSphere;
	selection::BasicSelectable _selectableX;
	selection::BasicSelectable _selectableY;
	selection::BasicSelectable _selectableZ;
	selection::BasicSelectable _selectableScreen;
	selection::BasicSelectable _selectableSphere;
	Pivot2World _pivot;
	Matrix4 _local2worldX;
	Matrix4 _local2worldY;
	Matrix4 _local2worldZ;
	bool _circleX_visible;
	bool _circleY_visible;
	bool _circleZ_visible;

public:
	static ShaderPtr _stateOuter;

	// Constructor
	RotateManipulator(Rotatable& rotatable, std::size_t segments, float radius);

	Type getType() const override
	{
		return Rotate;
	}

	void UpdateColours();
	void updateCircleTransforms();

	void render(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& pivot2world) override;

	void testSelect(const render::View& view, const Matrix4& pivot2world) override;

	Component* getActiveComponent() override;

	void setSelected(bool select) override;
	bool isSelected() const override;
};

}
