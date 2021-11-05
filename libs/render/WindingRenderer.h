#pragma once

#include "iwindingrenderer.h"

namespace render
{

class WindingRenderer :
    public IWindingRenderer
{
private:
    

public:
    Slot allocateWinding(int size) override
    {
        // Get the Bucket this Slot is referring to

        return InvalidSlot;
    }

    void deallocateWinding(Slot slot) override
    {

    }

    void updateWinding(Slot slot, const std::vector<ArbitraryMeshVertex>& vertices) override
    {

    }
};

}
