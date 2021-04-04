#pragma once

#include <istream>
#include "../StaticModelSurface.h"

namespace model
{

class AseModel
{
public:
    struct Surface
    {
        std::string material;

        std::vector<ArbitraryMeshVertex> vertices;
        std::vector<unsigned int> indices;
    };

private:
    std::vector<Surface> _surfaces;

public:
    Surface& addSurface();

    // Read/Write access
    std::vector<Surface>& getSurfaces();

    static std::shared_ptr<AseModel> CreateFromStream(std::istream& stream);
};

}
