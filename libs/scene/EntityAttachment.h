#pragma once

#include <string>
#include "math/Vector3.h"

/// Details of an attached entity
struct EntityAttachment
{
    /// Entity class of the attached entity
    std::string eclass;

    // Optional attachment name (arbitrarily chosen by the user)
    std::string name;

    /// Vector offset where the attached entity should appear
    Vector3 offset;

    /// Optional model joint to use as origin
    std::string joint;
};
