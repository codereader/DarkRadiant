#pragma once

#include "imanipulator.h"
#include "../BasicSelectable.h"
#include "ManipulatorComponents.h"
#include "../Renderables.h"

namespace selection
{

class TextureRotator : 
	public ManipulatorComponentBase
{
private:
	Vector3 _axis;
	Vector3 _start;

	// The most recently calculated angle for rendering purposes
	Vector3::ElementType _curAngle;
public:
    TextureRotator() :
		_curAngle(0)
	{}

	void beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint) override;

	/// \brief Converts current position to a normalised vector orthogonal to axis.
	void transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint, unsigned int constraints) override;

    void resetCurAngle();
    Vector3::ElementType getCurAngle() const;
};

class TextureToolRotateManipulator :
    public ITextureToolManipulator
{
private:
    std::size_t _id;

    BasicSelectable _selectableZ;
    TextureRotator _rotator;
    RenderableCircle _renderableCircle;

    ShaderPtr _shader;

public:
    TextureToolRotateManipulator();

    virtual std::size_t getId() const override;
    virtual void setId(std::size_t id) override;
    virtual Type getType() const override;

    virtual Component* getActiveComponent() override;

    virtual void setSelected(bool select) override;
    virtual bool isSelected() const override;
    virtual void testSelect(SelectionTest& test, const Matrix4& pivot2world) override;
    virtual void renderComponents(const Matrix4& pivot2World) override;
};

}
