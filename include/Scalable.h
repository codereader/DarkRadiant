#pragma once

#include "math/Vector3.h"

/// Interface for an object which can be scaled
class Scalable
{
public:
    virtual ~Scalable() {}

    /// Scale the object by the given vector
    virtual void scale(const Vector3& scaling) = 0;
};


