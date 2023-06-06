#pragma once

namespace math
{

/**
 * @brief Convenience class to encapsulate any object which needs to be stored separately
 * for the X, Y and Z dimensions of something.
 *
 * The object might be anything: a renderable for the X/Y/Z dimensions of a manipulator, a
 * colour for each axis, a visibility flag, whatever. This is simply a convenience to reduce
 * the need to define _memberX, _memberY and _memberZ triplets in numerous objects.
 *
 * Although an XYZ<float> or XYZ<double> are possible to define, this class is not a
 * full-featured vector and is not intended to be used in place of proper vector classes
 * like BasicVector3<T>.
 *
 * @tparam T Type of encapsulated object
 */
template<typename T> struct XYZ
{
    T x;
    T y;
    T z;
};

}