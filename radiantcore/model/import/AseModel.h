#pragma once

#include <istream>
#include "../StaticModelSurface.h"
#include "parser/Tokeniser.h"

namespace model
{

class AseModel
{
private:
    struct Material
    {
        AseModel::Material();

        std::string materialName;   // *MATERIAL_NAME
        std::string diffuseBitmap;  // *BITMAP

        float uOffset;              // * UVW_U_OFFSET
        float vOffset;              // * UVW_V_OFFSET
        float uTiling;              // * UVW_U_TILING
        float vTiling;              // * UVW_V_TILING
        float uvAngle;              // * UVW_ANGLE
    };

    struct Face;

    struct Mesh
    {
        std::vector<Vertex3f> vertices;
        std::vector<Normal3f> normals;
        std::vector<Face> faces;
        std::vector<TexCoord2f> texcoords;
        std::vector<Vector3> colours;
    };

public:
    struct Surface
    {
        std::string material;

        std::vector<ArbitraryMeshVertex> vertices;
        std::vector<unsigned int> indices;
    };

private:
    std::vector<Surface> _surfaces;

    std::vector<Material> _materials;

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

    void parseMaterialList(parser::StringTokeniser& tokeniser);
    void parseGeomObject(parser::StringTokeniser& tokeniser);
    void parseMesh(Mesh& mesh, parser::StringTokeniser& tokeniser);

    void finishSurface(Mesh& mesh, std::size_t materialIndex);
};

}
