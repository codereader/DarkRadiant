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

class TextureToolDragManipulator :
    public selection::ITextureToolManipulator
{
private:
    std::size_t _id;

    selection::BasicSelectable _selectable;
    TextureTranslator _translator;

public:
    TextureToolDragManipulator();

    virtual std::size_t getId() const override;
    virtual void setId(std::size_t id) override;
    virtual Type getType() const override;

    virtual Component* getActiveComponent() override;

    virtual void setSelected(bool select) override;
    virtual bool isSelected() const override;
    virtual void testSelect(SelectionTest& test, const Matrix4& pivot2world) override;
    virtual void renderComponents(const Matrix4& pivot2World) override;

private:
    void translateSelected(const Vector2& translation);
};

}
