#include "math/FloatTools.h"
#include "Manipulatables.h"
#include "Intersection.h"

#include "igrid.h"

void transform_local2object(Matrix4& object, const Matrix4& local, const Matrix4& local2object)
{
	object = local2object.getMultipliedBy(local).getMultipliedBy(local2object.getFullInverse());
}

void translation_local2object(Vector3& object, const Vector3& local, const Matrix4& local2object)
{
	object = local2object.getTranslatedBy(local).getMultipliedBy(local2object.getFullInverse()).translation();
}

// ===============================================================================================

void RotateFree::Construct(const Matrix4& device2manip, const float x, const float y) {
    point_on_sphere(_start, device2manip, x, y);
    _start.normalise();
}

void RotateFree::Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y) {
    Vector3 current;

    point_on_sphere(current, device2manip, x, y);
    current.normalise();

	// call the Rotatable with its transform method
    _rotatable.rotate(Quaternion::createForUnitVectors(_start, current));
}

// ===============================================================================================

void RotateAxis::Construct(const Matrix4& device2manip, const float x, const float y) {
    point_on_sphere(_start, device2manip, x, y);
    constrain_to_axis(_start, _axis);
}

/// \brief Converts current position to a normalised vector orthogonal to axis.
void RotateAxis::Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y) {
    Vector3 current;
    point_on_sphere(current, device2manip, x, y);
    constrain_to_axis(current, _axis);

	_rotatable.rotate(Quaternion::createForAxisAngle(_axis, angle_for_axis(_start, current, _axis)));
}

// ===============================================================================================

void TranslateAxis::Construct(const Matrix4& device2manip, const float x, const float y) {
    point_on_axis(_start, _axis, device2manip, x, y);
}

void TranslateAxis::Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y) {
    Vector3 current;
    point_on_axis(current, _axis, device2manip, x, y);
    current = _axis * distance_for_axis(_start, current, _axis);

    translation_local2object(current, current, manip2object);
    current.snap(GlobalGrid().getGridSize());

    _translatable.translate(current);
}

// ===============================================================================================

void TranslateFree::Construct(const Matrix4& device2manip, const float x, const float y) {
    point_on_plane(_start, device2manip, x, y);
}

void TranslateFree::Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y) {
    Vector3 current;
    point_on_plane(current, device2manip, x, y);
    current = current - _start;

    translation_local2object(current, current, manip2object);
    current.snap(GlobalGrid().getGridSize());

    _translatable.translate(current);
}

// ===============================================================================================

void ScaleAxis::Construct(const Matrix4& device2manip, const float x, const float y) {
    point_on_axis(_start, _axis, device2manip, x, y);
}

void ScaleAxis::Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y) {
    Vector3 current;
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
    _scalable.scale(scale);
}

// ===============================================================================================

void ScaleFree::Construct(const Matrix4& device2manip, const float x, const float y) {
    point_on_plane(_start, device2manip, x, y);
}

void ScaleFree::Transform(const Matrix4& manip2object, const Matrix4& device2manip, const float x, const float y) {
    Vector3 current;
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
    _scalable.scale(scale);
}
