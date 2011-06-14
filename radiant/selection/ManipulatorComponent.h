#pragma once

#include "math/Matrix4.h"

/**
 * Part of a Manipulator which can be operated upon by the user.
 *
 * \see Manipulator
 */
class ManipulatorComponent
{
public:
    virtual ~ManipulatorComponent() {}
    virtual void Construct(const Matrix4& device2manip, const float x, const float y) = 0;

    // greebo: An abstract Transform() method, the implementation has to decide
    // which operations are actually called. This may be a translation,
    // rotation, or anything else.
    virtual void Transform(const Matrix4& manip2object,
                           const Matrix4& device2manip,
                           const float x,
                           const float y) = 0;
};


