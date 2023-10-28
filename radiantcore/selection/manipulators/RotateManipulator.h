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
#include "math/XYZ.h"

namespace selection
{

/**
 * Manipulator for performing rotations with the mouse.
 *
 * Draws circles for rotation around individual axes, plus a circle for free
 * rotation.
 */
class RotateManipulator: public ManipulatorBase, public Rotatable
{
    ManipulationPivot& _pivot;
    TranslatablePivot _pivotTranslatable;

    RotateFree _rotateFree;
    RotateAxis _rotateAxis;
    TranslateFree _translatePivot;
    Vector3 _axisScreen;
    Vertex3 _localPivotPoint;

    // Renderables
    math::XYZ<RenderableSemiCircle> _circle;
    RenderableCircle _circleScreen;
    RenderableCircle _circleSphere;
    RenderablePoint _pivotPoint;
    render::StaticRenderableText _angleText;

    // Selectable objects
    math::XYZ<BasicSelectable> _selectable;
    BasicSelectable _selectableScreen;
    BasicSelectable _selectableSphere;
    BasicSelectable _selectablePivotPoint;
    Pivot2World _pivot2World;

    // Transformation matrices
    math::XYZ<Matrix4> _local2world;

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
