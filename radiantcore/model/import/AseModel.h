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
    Surface& addSurface(const std::string& name);

    Surface& ensureSurface(const std::string& name);

    // Read/Write access
    std::vector<Surface>& getSurfaces();

    // Create a new ASE model from the given stream
    // throws parser::ParseException on any failure
    static std::shared_ptr<AseModel> CreateFromStream(std::istream& stream);
};

}
