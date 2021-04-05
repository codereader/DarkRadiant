#pragma once

#include <istream>
#include "../StaticModelSurface.h"
#include "parser/Tokeniser.h"

namespace model
{

struct AseMaterial;
struct AseFace;

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

private:
    void parseFromTokens(parser::StringTokeniser& tokeniser);

    void finishSurface(std::vector<AseMaterial>& materials, std::size_t materialIndex,
        std::vector<Vertex3f>& vertices, std::vector<Normal3f>& normals, std::vector<TexCoord2f>& texcoords,
        std::vector<Vector3>& colours, std::vector<AseFace>& faces);
};

}
