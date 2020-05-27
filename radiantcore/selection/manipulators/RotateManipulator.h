#pragma once

#include "irender.h"
#include "Rotatable.h"
#include "ManipulatorBase.h"
#include "ManipulatorComponents.h"
#include "selection/Renderables.h"
#include "selection/Pivot2World.h"
#include "selection/BasicSelectable.h"
#include "selection/ManipulationPivot.h"
#include "render.h"

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
	public Rotatable,
	public OpenGLRenderable
{
private:
	ManipulationPivot& _pivot;
	TranslatablePivot _pivotTranslatable;

	RotateFree _rotateFree;
	RotateAxis _rotateAxis;
	TranslateFree _translatePivot;
	Vector3 _axisScreen;
	RenderableSemiCircle _circleX;
	RenderableSemiCircle _circleY;
	RenderableSemiCircle _circleZ;
	RenderableCircle _circleScreen;
	RenderableCircle _circleSphere;
	RenderablePointVector _pivotPoint;
	BasicSelectable _selectableX;
	BasicSelectable _selectableY;
	BasicSelectable _selectableZ;
	BasicSelectable _selectableScreen;
	BasicSelectable _selectableSphere;
	BasicSelectable _selectablePivotPoint;
	Pivot2World _pivot2World;
	Matrix4 _local2worldX;
	Matrix4 _local2worldY;
	Matrix4 _local2worldZ;
	bool _circleX_visible;
	bool _circleY_visible;
	bool _circleZ_visible;

public:
	static ShaderPtr _stateOuter;
	static ShaderPtr _pivotPointShader;

	// Constructor
	RotateManipulator(ManipulationPivot& pivot, std::size_t segments, float radius);

	Type getType() const override
	{
		return Rotate;
	}

	void UpdateColours();
	void updateCircleTransforms();

	void render(RenderableCollector& collector, const VolumeTest& volume) override;
	void render(const RenderInfo& info) const override;

	void testSelect(const render::View& view, const Matrix4& pivot2world) override;

	Component* getActiveComponent() override;

	void setSelected(bool select) override;
	bool isSelected() const override;

	void rotate(const Quaternion& rotation) override;
};

}
