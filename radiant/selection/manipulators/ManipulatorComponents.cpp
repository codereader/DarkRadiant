#include "ManipulatorComponents.h"

#include "igrid.h"
#include "math/FloatTools.h"
#include "pivot.h"

#include "../Intersection.h"

namespace selection
{

void transform_local2object(Matrix4& object, const Matrix4& local, const Matrix4& local2object)
{
	object = local2object.getMultipliedBy(local).getMultipliedBy(local2object.getFullInverse());
}

void translation_local2object(Vector3& object, const Vector3& local, const Matrix4& local2object)
{
	object = local2object.getTranslatedBy(local).getMultipliedBy(local2object.getFullInverse()).translation();
}

Vector3 ManipulatorComponentBase::getPlaneProjectedPoint(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint)
{
	Matrix4 device2pivot = constructDevice2Pivot(pivot2world, view);

	// greebo: We need to know the z-distance (or depth) of the pivot plane in device coordinates.
	// x and y are defined by the mouse clicks, but we need the third depth component to pass into the
	// device2pivot matrix. Lucky, this value can be extracted from the pivot2device matrix itself,
	// because the distance of that plane in device space is stored in the tz matrix component.
	// The trick is to invert the device2pivot matrix to get the tz value, to get a complete 4D point
	// to transform back into pivot space.
	Matrix4 pivot2device = constructPivot2Device(pivot2world, view);

	// This is now the complete 4D point to transform back into pivot space
	Vector4 point(devicePoint.x(), devicePoint.y(), pivot2device.tz(), 1);

	// Running the point through the device2pivot matrix will give us the mouse coordinates relative to pivot origin
	return device2pivot.transform(point).getProjected();
}

// ===============================================================================================

void RotateFree::beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint)
{
    //point_on_sphere(_start, device2manip, x, y);
    //_start.normalise();
}

void RotateFree::transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint)
{
 //   Vector3 current;

 //   point_on_sphere(current, device2manip, x, y);
 //   current.normalise();

	//// call the Rotatable with its transform method
 //   _rotatable.rotate(Quaternion::createForUnitVectors(_start, current));
}

// ===============================================================================================

void RotateAxis::beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint)
{
    /*point_on_sphere(_start, device2manip, x, y);
    constrain_to_axis(_start, _axis);*/
}

/// \brief Converts current position to a normalised vector orthogonal to axis.
void RotateAxis::transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint)
{
    /*Vector3 current;
    point_on_sphere(current, device2manip, x, y);
    constrain_to_axis(current, _axis);

	_rotatable.rotate(Quaternion::createForAxisAngle(_axis, angle_for_axis(_start, current, _axis)));*/
}

// ===============================================================================================

void TranslateAxis::beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint)
{
	_start = getPlaneProjectedPoint(pivot2world, view, devicePoint);
}

void TranslateAxis::transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint)
{
	// Get the regular difference between the starting point and the current mouse point
	Vector3 current = getPlaneProjectedPoint(pivot2world, view, devicePoint);
	Vector3 diff = current - _start;

	// Project this diff vector to our constraining axis
	Vector3 axisProjected = _axis * diff.dot(_axis);
	
	// Snap and apply translation
	axisProjected.snap(GlobalGrid().getGridSize());

	_translatable.translate(axisProjected);
}

// ===============================================================================================

void TranslateFree::beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint)
{
	// Transform the device coordinates to a point in pivot space 
	// The point is part of the plane going through pivot space origin, orthogonal to the view direction
	_start = getPlaneProjectedPoint(pivot2world, view, devicePoint);
}

void TranslateFree::transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint)
{
    Vector3 current = getPlaneProjectedPoint(pivot2world, view, devicePoint);
    Vector3 diff = current - _start;

    //translation_local2object(current, current, manip2object);
	diff.snap(GlobalGrid().getGridSize());

    _translatable.translate(diff);
}

// ===============================================================================================

void ScaleAxis::beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint)
{
    //point_on_axis(_start, _axis, device2manip, x, y);
}

void ScaleAxis::transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint)
{
 /*   Vector3 current;
    point_on_axis(current, _axis, device2manip, x, y);
    Vector3 delta = current - _start;

    translation_local2object(delta, delta, manip2object);
    delta.snap(GlobalGrid().getGridSize());

	Vector3 start(_start.getSnapped(GlobalGrid().getGridSize()));
    Vector3 scale(
      start[0] == 0 ? 1 : 1 + delta[0] / start[0],
      start[1] == 0 ? 1 : 1 + delta[1] / start[1],
      start[2] == 0 ? 1 : 1 + delta[2] / start[2]
    );
    _scalable.scale(scale);*/
}

// ===============================================================================================

void ScaleFree::beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint) 
{
    //point_on_plane(_start, device2manip, x, y);
}

void ScaleFree::transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint)
{
    /*Vector3 current;
    point_on_plane(current, device2manip, x, y);
    Vector3 delta = current - _start;

    translation_local2object(delta, delta, manip2object);
    delta.snap(GlobalGrid().getGridSize());

    Vector3 start(_start.getSnapped(GlobalGrid().getGridSize()));
    Vector3 scale(
      start[0] == 0 ? 1 : 1 + delta[0] / start[0],
      start[1] == 0 ? 1 : 1 + delta[1] / start[1],
      start[2] == 0 ? 1 : 1 + delta[2] / start[2]
    );
    _scalable.scale(scale);*/
}

SelectionTranslator::SelectionTranslator(const TranslationCallback& onTranslation) :
	_onTranslation(onTranslation)
{}

void SelectionTranslator::translate(const Vector3& translation)
{
	if (GlobalSelectionSystem().Mode() == SelectionSystem::eComponent)
	{
		GlobalSelectionSystem().foreachSelectedComponent(TranslateComponentSelected(translation));
	}
	else
	{
		// Cycle through the selected items and apply the translation
		GlobalSelectionSystem().foreachSelected(TranslateSelected(translation));
	}

	// Invoke the feedback function
	if (_onTranslation)
	{
		_onTranslation(translation);
	}
}

TranslatablePivot::TranslatablePivot(ManipulationPivot& pivot) :
	_pivot(pivot)
{}

void TranslatablePivot::translate(const Vector3& translation)
{
	Vector3 translationSnapped = translation.getSnapped(GlobalGrid().getGridSize());

	_pivot.applyTranslation(translationSnapped);

	// User is placing the pivot manually, so let's keep it that way
	_pivot.setUserLocked(true);
}

}