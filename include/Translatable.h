#pragma once

#include "math/Vector3.h"

/// Interface for an object which can be translated
class Translatable
{
public:
    virtual ~Translatable() {}

    /// Translate this object by the given vector
    virtual void translate(const Vector3& translation) = 0;
};


