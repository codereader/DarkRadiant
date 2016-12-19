#pragma once

#include "ManipulatorBase.h"
#include "ManipulatorComponents.h"
#include "../Renderables.h"
#include "../Pivot2World.h"
#include "../BasicSelectable.h"
#include "selection/ManipulationPivot.h"

namespace selection
{

/**
 * Manipulator for performing rotations with the mouse.
 *
 * Draws circles for rotation around individual axes, plus a circle for free
 * rotation.
 */
class RotateManipulator : 
	public ManipulatorBase,
	public Rotatable
{
private:
	ManipulationPivot& _pivot;

	RotateFree _rotateFree;
	RotateAxis _rotateAxis;
	Vector3 _axisScreen;
	RenderableSemiCircle _circleX;
	RenderableSemiCircle _circleY;
	RenderableSemiCircle _circleZ;
	RenderableCircle _circleScreen;
	RenderableCircle _circleSphere;
	BasicSelectable _selectableX;
	BasicSelectable _selectableY;
	BasicSelectable _selectableZ;
	BasicSelectable _selectableScreen;
	BasicSelectable _selectableSphere;
	Pivot2World _pivot2World;
	Matrix4 _local2worldX;
	Matrix4 _local2worldY;
	Matrix4 _local2worldZ;
	bool _circleX_visible;
	bool _circleY_visible;
	bool _circleZ_visible;

public:
	static ShaderPtr _stateOuter;

	// Constructor
	RotateManipulator(ManipulationPivot& pivot, std::size_t segments, float radius);

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

	void rotate(const Quaternion& rotation) override;
};

}
