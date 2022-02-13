#pragma once

#include "irender.h"
#include "Rotatable.h"
#include "ManipulatorBase.h"
#include "ManipulatorComponents.h"
#include "selection/Pivot2World.h"
#include "selection/BasicSelectable.h"
#include "selection/ManipulationPivot.h"
#include "render.h"
#include "Renderables.h"
#include "render/StaticRenderableText.h"

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
	TranslatablePivot _pivotTranslatable;

	RotateFree _rotateFree;
	RotateAxis _rotateAxis;
	TranslateFree _translatePivot;
	Vector3 _axisScreen;
    Vertex3f _localPivotPoint;

    RenderableSemiCircle<RemapYZX> _circleX;
    RenderableSemiCircle<RemapZXY> _circleY;
    RenderableSemiCircle<RemapXYZ> _circleZ;
	RenderableCircle<RemapXYZ> _circleScreen;
	RenderableCircle<RemapXYZ> _circleSphere;
    RenderablePoint _pivotPoint;
    render::StaticRenderableText _angleText;

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

    ShaderPtr _lineShader;
    ShaderPtr _pivotPointShader;
    ITextRenderer::Ptr _textRenderer;

public:
	RotateManipulator(ManipulationPivot& pivot, std::size_t segments, float radius);

	Type getType() const override
	{
		return Rotate;
	}

    void onPreRender(const RenderSystemPtr& renderSystem, const VolumeTest& volume) override;
    void clearRenderables() override;

	void testSelect(SelectionTest& view, const Matrix4& pivot2world) override;

	Component* getActiveComponent() override;

	void setSelected(bool select) override;
	bool isSelected() const override;

	void rotate(const Quaternion& rotation) override;

private:
    void updateColours();
    void updateCircleTransforms();
    void updateAngleText();

    std::string getRotationAxisName() const;
};

}
