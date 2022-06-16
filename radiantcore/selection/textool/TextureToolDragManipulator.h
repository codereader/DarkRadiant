#pragma once

#include "imanipulator.h"
#include "selection/manipulators/ManipulatorComponents.h"
#include "selection/BasicSelectable.h"

namespace textool
{

class TextureTranslator : 
    public selection::ManipulatorComponentBase
{
private:
	Vector2 _start;

    std::function<void(const Vector2&)> _translateFunctor;

public:
    TextureTranslator(const std::function<void(const Vector2&)>& translateFunctor) :
        _translateFunctor(translateFunctor)
	{}

	void beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint) override;

	void transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint, unsigned int constraints) override;
};

class TextureDragResizer :
    public selection::ManipulatorComponentBase
{
private:
    // Starting point in UV space
    Vector2 _start;

    // The pivot point for the scale operation
    Vector2 _scalePivot;

    // Multiplicative factor to apply before scaling
    // Every scaled dimension has 1.0 in the corresponding component
    Vector2 _scaleMask;

    Vector2 _startingBoundsExtents;

    std::function<void(const Vector2&, const Vector2&)> _scaleFunctor;

public:
    TextureDragResizer(const std::function<void(const Vector2&, const Vector2&)>& scaleFunctor) :
        _scaleFunctor(scaleFunctor)
    {}

    void beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint) override;

    void transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint, unsigned int constraints) override;

private:
    static Vector2 FindFarthestCorner(const AABB& bounds, const Vector2& start);
};

class TextureToolDragManipulator :
    public selection::ITextureToolManipulator
{
private:
    std::size_t _id;

    selection::BasicSelectable _translateSelectable;
    selection::BasicSelectable _scaleSelectable;
    TextureTranslator _translator;
    TextureDragResizer _resizer;

public:
    TextureToolDragManipulator();

    virtual std::size_t getId() const override;
    virtual void setId(std::size_t id) override;
    virtual Type getType() const override;

    virtual Component* getActiveComponent() override;

    virtual void setSelected(bool select) override;
    virtual bool isSelected() const override;
    virtual void testSelect(SelectionTest& test, const Matrix4& pivot2world) override;
    virtual void renderComponents(const render::IRenderView& view, const Matrix4& pivot2World) override;

private:
    void translateSelected(const Vector2& translation);
    void scaleSelected(const Vector2& scale, const Vector2& pivot);
    void testSelectDragResize(SelectionTest& test, const Matrix4& pivot2world);
};

}
