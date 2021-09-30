#pragma once

#include "selection/ManipulationPivot.h"

namespace textool
{

/**
 * ManipulationPivot specialisation for use in the Texture Tool
 */
class TextureToolManipulationPivot final :
    public selection::ManipulationPivot
{
public:
    void updateFromSelection() override;
};

}
