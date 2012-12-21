#pragma once

#include "math/Quaternion.h"

/// Interface for an object which can be rotated
class Rotatable
{
public:
    virtual ~Rotatable() {}

    /// Rotate the object by the given Quaternion
    virtual void rotate(const Quaternion& rotation) = 0;
};


