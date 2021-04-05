#include "AseModel.h"

#include <fmt/format.h>
#include "parser/Tokeniser.h"
#include "parser/ParseException.h"
#include "string/case_conv.h"
#include "string/trim.h"

/* -----------------------------------------------------------------------------

ASE Loading Code based on the original PicoModel ASE parser (licence as follows)

PicoModel Library

Copyright (c) 2002, Randy Reddig & seaw0lf
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other aseMaterialList provided with the distribution.

Neither the names of the copyright holders nor the names of its contributors may
be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------------- */

namespace
{
    inline void combineHash(std::size_t& seed, std::size_t hash)
    {
        seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
}

template<>
struct std::hash<Vector3>
{
    size_t operator()(const Vector3& v) const
    {
        std::hash<double> hasher;

        auto hash = hasher(v.x());
        combineHash(hash, hasher(v.y()));
        combineHash(hash, hasher(v.z()));
        
        return hash;
    }
};

// Hash specialisation such that ArbitraryMeshVertex can be used as key type in std::unordered_map<>
template<>
struct std::hash<ArbitraryMeshVertex>
{
    size_t operator()(const ArbitraryMeshVertex& v) const
    {
        std::hash<Vector3> vectorHash;
        std::hash<double> doubleHash;

        auto hash = vectorHash(v.vertex);

        combineHash(hash, vectorHash(v.normal));
        combineHash(hash, doubleHash(v.texcoord.x()));
        combineHash(hash, doubleHash(v.texcoord.y()));
        combineHash(hash, vectorHash(v.colour));

        return hash;
    }
};

// Assumes equality of two ArbitraryMeshVertices if all of (vertex, normal, texcoord, colour) are equal
template<>
struct std::equal_to<ArbitraryMeshVertex>
{
    bool operator()(const ArbitraryMeshVertex& a, const ArbitraryMeshVertex& b) const
    {
        return a.vertex == b.vertex && a.normal == b.normal && a.texcoord == b.texcoord && a.colour == b.colour;
    }
};

namespace model
{

struct AseModel::Face
{
    AseModel::Face()
    {
        vertexIndices[0] = vertexIndices[1] = vertexIndices[2] = 0;
        texcoordIndices[0] = texcoordIndices[1] = texcoordIndices[2] = 0;
        colourIndices[0] = colourIndices[1] = colourIndices[2] = 0;
    }

    std::size_t vertexIndices[3];
    std::size_t texcoordIndices[3];
    std::size_t colourIndices[3];
};

AseModel::Material::Material() :
    uOffset(0),
    vOffset(0),
    uTiling(1),
    vTiling(1),
    uvAngle(0)
{}

void AseModel::finishSurface(Mesh& mesh, std::size_t materialIndex, const Matrix4& nodeMatrix)
{
    assert(mesh.vertices.size() == mesh.normals.size());
    static Vector3 White(1, 1, 1);

    if (materialIndex >= _materials.size())
    {
        throw parser::ParseException(fmt::format("Cannot submit triangles, material index {0} is out of range", materialIndex));
    }

    const auto& material = _materials[materialIndex];

    // submit the triangle to the model
    auto& surface = ensureSurface(material.diffuseBitmap);

    surface.vertices.reserve(surface.vertices.size() + mesh.vertices.size());
    surface.indices.reserve(surface.indices.size() + mesh.faces.size() * 3);

    double materialSin = sin(material.uvAngle);
    double materialCos = cos(material.uvAngle);

    // 
    std::unordered_map<ArbitraryMeshVertex, std::size_t> vertexIndices;

    for (const auto& face : mesh.faces)
    {
        // we pull the data from the vertex, color and texcoord arrays using the face index data
        for (int j = 0; j < 3; ++j)
        {
            const auto& vertex = mesh.vertices[face.vertexIndices[j]];
            const auto& normal = mesh.normals[face.vertexIndices[j]];

            double u, v;

            // greebo: Apply shift, scale and rotation
            // Also check for empty texcoords, some models surfaces don't have any tverts
            if (!mesh.texcoords.empty())
            {
                u = mesh.texcoords[face.texcoordIndices[j]].x() * material.uTiling + material.uOffset;
                v = mesh.texcoords[face.texcoordIndices[j]].y() * material.vTiling + material.vOffset;
            }
            else
            {
                u = 0;
                v = 0;
            }

            const auto& colour = !mesh.colours.empty() ? mesh.colours[face.colourIndices[j]] : White;
            
            // Try to look up an existing vertex or add a new index
            auto emplaceResult = vertexIndices.try_emplace(ArbitraryMeshVertex(
                vertex,
                nodeMatrix.transformDirection(normal).getNormalised(),
                TexCoord2f(u * materialCos + v * materialSin, u * -materialSin + v * materialCos),
                colour
            ), surface.vertices.size());

            if (emplaceResult.second)
            {
                // This was a new vertex, copy it to the vertex array
                surface.vertices.emplace_back(emplaceResult.first->first);
            }
            
            // The emplaceResult now points to a valid index in the vertex array
            surface.indices.emplace_back(static_cast<IndexBuffer::value_type>(emplaceResult.first->second));
        }
    }
}

AseModel::Surface& AseModel::addSurface(const std::string& name)
{
    return _surfaces.emplace_back(Surface{name});
}

AseModel::Surface& AseModel::ensureSurface(const std::string& name)
{
    for (auto& surface : _surfaces)
    {
        if (surface.material == name)
        {
            return surface;
        }
    }

    return addSurface(name);
}

std::vector<AseModel::Surface>& AseModel::getSurfaces()
{
    return _surfaces;
}

void AseModel::parseMaterialList(parser::StringTokeniser& tokeniser)
{
    _materials.clear();

    int blockLevel = 0;

    while (tokeniser.hasMoreTokens())
    {
        auto token = tokeniser.nextToken();
        string::to_lower(token);

        if (token == "}")
        {
            if (--blockLevel == 0) break;
        }
        else if (token == "{")
        {
            ++blockLevel;
        }
        else if (token == "*material_count")
        {
            auto numMaterials = string::convert<std::size_t>(tokeniser.nextToken());
            _materials.resize(numMaterials);
        }
        else if (token == "*material")
        {
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= _materials.size()) throw parser::ParseException("MATERIAL index out of bounds >= MATERIAL_COUNT");

            tokeniser.assertNextToken("{");
            int level = 1;

            /* parse material block */
            while (tokeniser.hasMoreTokens())
            {
                token = tokeniser.nextToken();
                string::to_lower(token);

                if (token.empty()) continue;

                /* handle levels */
                if (token[0] == '{') level++;
                if (token[0] == '}') level--;

                if (level == 0) break;

                /* parse material name */
                if (token == "*material_name")
                {
                    _materials[index].materialName = string::trim_copy(tokeniser.nextToken(), "\"");
                }
                /* material diffuse map */
                else if (token == "*map_diffuse")
                {
                    int sublevel = 0;

                    /* parse material block */
                    while (tokeniser.hasMoreTokens())
                    {
                        token = tokeniser.nextToken();
                        string::to_lower(token);

                        if (token.empty()) continue;

                        /* handle levels */
                        if (token[0] == '{') sublevel++;
                        if (token[0] == '}') sublevel--;

                        if (sublevel == 0) break;

                        /* parse diffuse map bitmap */
                        if (token == "*bitmap")
                        {
                            _materials[index].diffuseBitmap = string::trim_copy(tokeniser.nextToken(), "\"");
                        }
                        else if (token == "*uvw_u_offset")
                        {
                            // Negate the u offset value
                            _materials[index].uOffset = -string::convert<float>(tokeniser.nextToken());
                        }
                        else if (token == "*uvw_v_offset")
                        {
                            _materials[index].vOffset = string::convert<float>(tokeniser.nextToken());
                        }
                        else if (token == "*uvw_u_tiling")
                        {
                            _materials[index].uTiling = string::convert<float>(tokeniser.nextToken());
                        }
                        else if (token == "*uvw_v_tiling")
                        {
                            _materials[index].vTiling = string::convert<float>(tokeniser.nextToken());
                        }
                        else if (token == "*uvw_angle")
                        {
                            _materials[index].uvAngle = string::convert<float>(tokeniser.nextToken());
                        }
                    }
                } // end map_diffuse block
            } // end material block
        }
    }
}

void AseModel::parseMesh(Mesh& mesh, parser::StringTokeniser& tokeniser)
{
    int blockLevel = 0;

    while (tokeniser.hasMoreTokens())
    {
        auto token = tokeniser.nextToken();
        string::to_lower(token);

        if (token == "}")
        {
            if (--blockLevel == 0) break;
        }
        else if (token == "{")
        {
            ++blockLevel;
        }
        else if (token == "*mesh_numvertex")
        {
            // Parse the number to allocate space in the vertex vector
            auto numVertices = string::convert<std::size_t>(tokeniser.nextToken());
            mesh.vertices.resize(numVertices);
            mesh.normals.resize(numVertices);
        }
        else if (token == "*mesh_numfaces")
        {
            auto numFaces = string::convert<std::size_t>(tokeniser.nextToken());
            mesh.faces.resize(numFaces);
        }
        else if (token == "*mesh_numtvertex")
        {
            auto numTextureVertices = string::convert<std::size_t>(tokeniser.nextToken());
            mesh.texcoords.resize(numTextureVertices);
        }
        else if (token == "*mesh_numcvertex")
        {
            auto numColorVertices = string::convert<std::size_t>(tokeniser.nextToken());
            mesh.colours.resize(numColorVertices, Vector3(1.0, 1.0, 1.0));
        }
        /* model mesh vertex */
        else if (token == "*mesh_vertex")
        {
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= mesh.vertices.size()) throw parser::ParseException("MESH_VERTEX index out of bounds >= MESH_NUMVERTEX");

            auto& vertex = mesh.vertices[index];
            vertex.x() = string::convert<double>(tokeniser.nextToken());
            vertex.y() = string::convert<double>(tokeniser.nextToken());
            vertex.z() = string::convert<double>(tokeniser.nextToken());
        }
        /* model mesh vertex normal */
        else if (token == "*mesh_vertexnormal")
        {
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= mesh.vertices.size()) throw parser::ParseException("MESH_VERTEXNORMAL index out of bounds >= MESH_NUMVERTEX");

            auto& normal = mesh.normals[index];
            normal.x() = string::convert<double>(tokeniser.nextToken());
            normal.y() = string::convert<double>(tokeniser.nextToken());
            normal.z() = string::convert<double>(tokeniser.nextToken());
        }
        /* model mesh face */
        else if (token == "*mesh_face")
        {
            // *MESH_FACE    0:    A:    3 B:    1 C:    2 AB:    0 BC:    0 CA:    0	 *MESH_SMOOTHING 0	 *MESH_MTLID 0
            auto index = string::convert<std::size_t>(string::trim_right_copy(tokeniser.nextToken(), ":"));

            if (index >= mesh.faces.size()) throw parser::ParseException("MESH_FACE index out of bounds >= MESH_NUMFACES");

            auto& face = mesh.faces[index];

            // Note: we're reversing the winding to get CW ordering
            tokeniser.assertNextToken("A:");
            face.vertexIndices[2] = string::convert<std::size_t>(tokeniser.nextToken());

            tokeniser.assertNextToken("B:");
            face.vertexIndices[1] = string::convert<std::size_t>(tokeniser.nextToken());

            tokeniser.assertNextToken("C:");
            face.vertexIndices[0] = string::convert<std::size_t>(tokeniser.nextToken());

            if (face.vertexIndices[2] >= mesh.vertices.size()) throw parser::ParseException("MESH_FACE vertex index 0 out of bounds >= MESH_NUMFACES");
            if (face.vertexIndices[1] >= mesh.vertices.size()) throw parser::ParseException("MESH_FACE vertex index 1 out of bounds >= MESH_NUMFACES");
            if (face.vertexIndices[0] >= mesh.vertices.size()) throw parser::ParseException("MESH_FACE vertex index 2 out of bounds >= MESH_NUMFACES");

            tokeniser.skipTokens(9);
        }
        /* model texture vertex */
        else if (token == "*mesh_tvert")
        {
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= mesh.texcoords.size()) throw parser::ParseException("MESH_TVERT index out of bounds >= MESH_NUMTVERTEX");

            auto& texcoord = mesh.texcoords[index];
            texcoord.x() = string::convert<double>(tokeniser.nextToken());
            /* ydnar: invert t */
            texcoord.y() = 1.0 - string::convert<double>(tokeniser.nextToken());
            // ignore the third texcoord value
            tokeniser.nextToken();
        }
        /* ydnar: model mesh texture face */
        else if (token == "*mesh_tface")
        {
            // *MESH_TFACE 0    0   1   2
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= mesh.faces.size()) throw parser::ParseException("MESH_TFACE index out of bounds >= MESH_NUMFACES");

            auto& face = mesh.faces[index];

            // Reverse the winding order
            face.texcoordIndices[2] = string::convert<std::size_t>(tokeniser.nextToken());
            face.texcoordIndices[1] = string::convert<std::size_t>(tokeniser.nextToken());
            face.texcoordIndices[0] = string::convert<std::size_t>(tokeniser.nextToken());

            if (face.texcoordIndices[2] >= mesh.texcoords.size()) throw parser::ParseException("MESH_TFACE texcoord index 0 out of bounds >= MESH_NUMTVERTEX");
            if (face.texcoordIndices[1] >= mesh.texcoords.size()) throw parser::ParseException("MESH_TFACE texcoord index 1 out of bounds >= MESH_NUMTVERTEX");
            if (face.texcoordIndices[0] >= mesh.texcoords.size()) throw parser::ParseException("MESH_TFACE texcoord index 2 out of bounds >= MESH_NUMTVERTEX");
        }
        /* model color vertex */
        else if (token == "*mesh_vertcol")
        {
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= mesh.colours.size()) throw parser::ParseException("MESH_VERTCOL index out of bounds >= MESH_NUMCVERTEX");

            auto& colour = mesh.colours[index];
            colour.x() = string::convert<double>(tokeniser.nextToken());
            colour.y() = string::convert<double>(tokeniser.nextToken());
            colour.z() = string::convert<double>(tokeniser.nextToken());
        }
        /* model color face */
        else if (token == "*mesh_cface")
        {
            // *MESH_CFACE 0    0   1   2
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= mesh.faces.size()) throw parser::ParseException("MESH_CFACE index out of bounds >= MESH_NUMFACES");

            auto& face = mesh.faces[index];

            // Reverse the winding order
            face.colourIndices[2] = string::convert<std::size_t>(tokeniser.nextToken());
            face.colourIndices[1] = string::convert<std::size_t>(tokeniser.nextToken());
            face.colourIndices[0] = string::convert<std::size_t>(tokeniser.nextToken());

            if (face.colourIndices[2] >= mesh.colours.size()) throw parser::ParseException("MESH_CFACE colour index 0 out of bounds >= MESH_NUMCVERTEX");
            if (face.colourIndices[1] >= mesh.colours.size()) throw parser::ParseException("MESH_CFACE colour index 1 out of bounds >= MESH_NUMCVERTEX");
            if (face.colourIndices[0] >= mesh.colours.size()) throw parser::ParseException("MESH_CFACE colour index 2 out of bounds >= MESH_NUMCVERTEX");
        }
    }
}

void AseModel::parseNodeMatrix(Matrix4& matrix, parser::StringTokeniser& tokeniser)
{
    int blockLevel = 0;

    // We parse the rows in the ASE file into the columns of the matrix
    // to be able to just use Matrix4::transformDirection() to transform the normal
    while (tokeniser.hasMoreTokens())
    {
        auto token = tokeniser.nextToken();
        string::to_lower(token);

        if (token == "}")
        {
            if (--blockLevel == 0) break;
        }
        else if (token == "{")
        {
            ++blockLevel;
        }
        else if (token == "*tm_row0")
        {
            matrix.xx() = string::convert<double>(tokeniser.nextToken());
            matrix.xy() = string::convert<double>(tokeniser.nextToken());
            matrix.xz() = string::convert<double>(tokeniser.nextToken());
        }
        else if (token == "*tm_row1")
        {
            matrix.yx() = string::convert<double>(tokeniser.nextToken());
            matrix.yy() = string::convert<double>(tokeniser.nextToken());
            matrix.yz() = string::convert<double>(tokeniser.nextToken());
        }
        else if (token == "*tm_row2")
        {
            matrix.zx() = string::convert<double>(tokeniser.nextToken());
            matrix.zy() = string::convert<double>(tokeniser.nextToken());
            matrix.zz() = string::convert<double>(tokeniser.nextToken());
        }
        // The fourth row *TM_ROW3 is ignored, translations are not applicable to normals
    }
}

void AseModel::parseGeomObject(parser::StringTokeniser& tokeniser)
{
    Mesh mesh;
    Matrix4 nodeMatrix = Matrix4::getIdentity();
    int materialIndex = -1;

    int blockLevel = 0;

    while (tokeniser.hasMoreTokens())
    {
        auto token = tokeniser.nextToken();
        string::to_lower(token);

        if (token == "}")
        {
            if (--blockLevel == 0) break;
        }
        else if (token == "{")
        {
            ++blockLevel;
        }
        else if (token == "*mesh")
        {
            parseMesh(mesh, tokeniser);
        }
        else if (token == "*node_tm")
        {
            // The NODE_TM block is parsed by the engine and applied to the
            // normals of the mesh.
            parseNodeMatrix(nodeMatrix, tokeniser);
        }
        /* mesh material reference. this usually comes at the end of
         * geomobjects after the mesh blocks. we must assume that the
         * new mesh was already created so all we can do here is assign
         * the material reference id (shader index) now. */
        else if (token == "*material_ref")
        {
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= _materials.size()) throw parser::ParseException("MATERIAL_REF index out of bounds >= MATERIAL_COUNT");

            materialIndex = static_cast<int>(index);
        }
    }

    finishSurface(mesh, materialIndex, nodeMatrix);
}

void AseModel::parseFromTokens(parser::StringTokeniser& tokeniser)
{
    if (string::to_lower_copy(tokeniser.nextToken()) != "*3dsmax_asciiexport")
    {
        throw parser::ParseException("Missing 3DSMAX_ASCIIEXPORT header");
    }

    while (tokeniser.hasMoreTokens())
    {
        auto token = tokeniser.nextToken();
        string::to_lower(token);

        // skip invalid ase statements
        if (token[0] != '*' && token[0] != '{' && token[0] != '}')
        {
            continue;
        }

        if (token == "*material_list")
        {
            parseMaterialList(tokeniser);
        }
        else if (token== "*geomobject")
        {
            parseGeomObject(tokeniser);
        }
    }
}

std::shared_ptr<AseModel> AseModel::CreateFromStream(std::istream& stream)
{
    auto model = std::make_shared<AseModel>();

    parser::BasicStringTokeniser tokeniser(stream);
    model->parseFromTokens(tokeniser);

    return model;
}

} // namespace
