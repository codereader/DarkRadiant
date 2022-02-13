#pragma once

#include "imanipulator.h"
#include "../BasicSelectable.h"
#include "selection/manipulators/ManipulatorComponents.h"
#include "selection/textool/TextureToolManipulationPivot.h"

namespace textool
{

class TextureRotator : 
    public selection::ManipulatorComponentBase
{
private:
	Vector2 _start;
	Vector2 _deviceStart;
	Vector2 _screenStart;
	Vector2 _current;
	Vector2 _deviceCurrent;
	Vector2 _screenCurrent;

	// The most recently calculated angle for rendering purposes
	Vector2::ElementType _curAngle;

    std::function<void(const Vector2&, Vector2::ElementType)> _rotateFunctor;

public:
    TextureRotator(const std::function<void(const Vector2&, Vector2::ElementType)>& rotateFunctor) :
		_curAngle(0),
        _rotateFunctor(rotateFunctor)
	{}

	void beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint) override;

	void transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint, unsigned int constraints) override;

    void resetCurAngle();
    Vector2::ElementType getCurAngle() const;

    // The vector from the pivot to the starting point of the manipulation (screen space, normalised)
    const Vector2& getStartDirection() const;

    const Vector2& getStartDirectionInScreenSpace() const;
};

class TextureToolRotateManipulator :
    public selection::ITextureToolManipulator
{
private:
    TextureToolManipulationPivot& _pivot;

    std::size_t _id;

    selection::BasicSelectable _selectableZ;
    TextureRotator _rotator;
    RenderablePointVector _renderableCircle;

    ShaderPtr _shader;
    IGLFont::Ptr _glFont;

    double _circleRadius;

public:
    TextureToolRotateManipulator(TextureToolManipulationPivot& pivot);

    virtual std::size_t getId() const override;
    virtual void setId(std::size_t id) override;
    virtual Type getType() const override;

    virtual Component* getActiveComponent() override;

    virtual void setSelected(bool select) override;
    virtual bool isSelected() const override;
    virtual void testSelect(SelectionTest& test, const Matrix4& pivot2world) override;
    virtual void renderComponents(const render::IRenderView& view, const Matrix4& pivot2World) override;

private:
    void rotateSelected(const Vector2& pivot, double angle);
};

}
