#pragma once

#include "iscenegraph.h"
#include "math/Vector3.h"
#include "math/Matrix4.h"
#include "math/Quaternion.h"

#include "render.h"

#include "selection/TransformationVisitors.h"
#include "Translatable.h"
#include "Rotatable.h"
#include "Scalable.h"
#include "selection/ManipulationPivot.h"
#include <functional>

namespace selection
{

class ManipulatorComponentBase :
	public Manipulator::Component
{
public:
	virtual ~ManipulatorComponentBase()
	{}

protected:
	/**
	 * Transform the device coordinates to a point in pivot space, 
	 * located on the plane going through pivot space origin, orthogonal to the view direction
	 */
	Vector3 getPlaneProjectedPoint(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint);
};

/* greebo: The following are specialised manipulatables that provide the methods as described in the ABC.
 * They basically prepare and constraing the transformations of the three base movements above (Translatable, etc.)
 *
 * So, for example, the TranslateAxis class takes care that the Translatable is only moved in the axis directions.
 * The necessary device pointer >> translation vector calculations are performed within Transform()
 */

class RotateFree : 
	public Manipulator::Component
{
	Vector3 _start;
	Rotatable& _rotatable;
public:
	RotateFree(Rotatable& rotatable) : 
		_rotatable(rotatable) 
	{}

	void beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint) override;
	void transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint) override;
};

class RotateAxis : 
	public Manipulator::Component
{
	Vector3 _axis;
	Vector3 _start;
	Rotatable& _rotatable;
public:
	RotateAxis(Rotatable& rotatable) :
		_rotatable(rotatable) 
	{}

	void beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint) override;

	/// \brief Converts current position to a normalised vector orthogonal to axis.
	void transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint) override;

	void SetAxis(const Vector3& axis)
	{
		_axis = axis;
	}
};

class TranslateAxis : 
	public ManipulatorComponentBase
{
private:
	Vector3 _start;
	Vector3 _axis;
	Translatable& _translatable;
public:
	TranslateAxis(Translatable& translatable) :
		_translatable(translatable) 
	{}

	void beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint) override;
	void transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint) override;

	void SetAxis(const Vector3& axis) 
	{
		_axis = axis.getNormalised();
	}
};

class TranslateFree : 
	public ManipulatorComponentBase
{
private:
	Vector3 _start;
	Translatable& _translatable;
public:
	TranslateFree(Translatable& translatable) :
		_translatable(translatable) 
	{}
	
	void beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint) override;
	void transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint) override;
};


class ScaleAxis : 
	public Manipulator::Component
{
private:
	Vector3 _start;
	Vector3 _axis;
	Scalable& _scalable;
public:
	ScaleAxis(Scalable& scalable) :
		_scalable(scalable) 
	{}

	void beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint) override;
	void transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint) override;

	void SetAxis(const Vector3& axis)
	{
		_axis = axis;
	}
};

class ScaleFree : 
	public Manipulator::Component
{
private:
	Vector3 _start;
	Scalable& _scalable;
public:
	ScaleFree(Scalable& scalable) :
		_scalable(scalable) 
	{}

	void beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint) override;
	void transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint) override;
};

// ========= Translatables ===============================================

class ResizeTranslatable : 
	public Translatable
{
	void translate(const Vector3& translation) override
	{
		GlobalSelectionSystem().foreachSelectedComponent(TranslateComponentSelected(translation));
	}
};

class SelectionTranslator : 
	public Translatable
{
public:
	typedef std::function<void(const Vector3&)> TranslationCallback;

private:
	TranslationCallback _onTranslation;

public:
	SelectionTranslator(const TranslationCallback& onTranslation);

	void translate(const Vector3& translation) override;
};

class TranslatablePivot :
	public Translatable
{
private:
	ManipulationPivot& _pivot;

public:
	TranslatablePivot(ManipulationPivot& pivot);

	void translate(const Vector3& translation) override;
};

// ===============================================================================

void transform_local2object(Matrix4& object, const Matrix4& local, const Matrix4& local2object);
void translation_local2object(Vector3& object, const Vector3& local, const Matrix4& local2object);

}
