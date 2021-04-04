#include "AseModel.h"

#include <fmt/format.h>
#include "parser/DefTokeniser.h"
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

namespace model
{

struct AseFace
{
    AseFace()
    {
        vertexIndices[0] = vertexIndices[1] = vertexIndices[2] = 0;
        texcoordIndices[0] = texcoordIndices[1] = texcoordIndices[2] = 0;
        colourIndices[0] = colourIndices[1] = colourIndices[2] = 0;
    }

    std::size_t vertexIndices[3];
    std::size_t texcoordIndices[3];
    std::size_t colourIndices[3];
};

struct AseMaterial
{
    AseMaterial() :
        uOffset(0),
        vOffset(0),
        uTiling(1),
        vTiling(1),
        uvAngle(0)
    {}

    std::string materialName;   // *MATERIAL_NAME
    std::string diffuseBitmap;  // *BITMAP

    float uOffset;              // * UVW_U_OFFSET
    float vOffset;              // * UVW_V_OFFSET
    float uTiling;              // * UVW_U_TILING
    float vTiling;              // * UVW_V_TILING
    float uvAngle;              // * UVW_ANGLE
};

void _ase_submit_triangles(model::AseModel& model, 
    std::vector<model::AseMaterial>& materials, std::size_t materialIndex,
    std::vector<Vertex3f>& vertices, std::vector<Normal3f>& normals, std::vector<TexCoord2f>& texcoords,
    std::vector<Vector3>& colours, std::vector<model::AseFace>& faces)
{
    assert(vertices.size() == normals.size());

    if (materialIndex >= materials.size())
    {
        throw parser::ParseException(fmt::format("Cannot submit triangles, material index {0} is out of range", materialIndex));
    }

    const auto& material = materials[materialIndex];

    // submit the triangle to the model
    auto& surface = model.ensureSurface(material.diffuseBitmap);

    surface.vertices.reserve(surface.vertices.size() + vertices.size());
    surface.indices.reserve(surface.indices.size() + faces.size() * 3);

    double materialSin = sin(material.uvAngle);
    double materialCos = cos(material.uvAngle);

    for (const auto& face : faces)
    {
        /* we pull the data from the vertex, color and texcoord arrays using the face index data */
        for (int j = 0 ; j < 3 ; j ++ )
        {
            auto nextIndex = static_cast<unsigned int>(surface.indices.size());

            auto& vertex = vertices[face.vertexIndices[j]];
            auto& normal = normals[face.vertexIndices[j]];

            double u, v;

            /* greebo: Apply shift, scale and rotation */
            /* Also check for empty texcoords, some models surfaces don't have any tverts */
            if (!texcoords.empty())
            {
                u = texcoords[face.texcoordIndices[j]].x() * material.uTiling + material.uOffset;
                v = texcoords[face.texcoordIndices[j]].y() * material.vTiling + material.vOffset;
            }
            else
            {
                u = 0;
                v = 0;
            }

            auto& meshVertex = surface.vertices.emplace_back(ArbitraryMeshVertex
            { 
                vertex, 
                normal, 
                TexCoord2f(u * materialCos + v * materialSin, u * -materialSin + v * materialCos) 
            });

            surface.indices.emplace_back(nextIndex++);

            if (!colours.empty())
            {
                meshVertex.colour = colours[face.colourIndices[j]];
            }
            else
            {
                meshVertex.colour.set(1, 1, 1);
            }
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

std::shared_ptr<AseModel> AseModel::CreateFromStream(std::istream& stream)
{
    auto model = std::make_shared<AseModel>();

    parser::BasicDefTokeniser tokeniser(stream);

    std::vector<AseMaterial> materials;
    std::vector<Vertex3f> vertices;
    std::vector<Normal3f> normals;
    std::vector<AseFace> faces;
    std::vector<TexCoord2f> texcoords;
    std::vector<Vector3> colours;
    int materialIndex = -1;

    if (string::to_lower_copy(tokeniser.nextToken()) != "*3dsmax_asciiexport")
    {
        throw parser::ParseException("Missing 3DSMAX_ASCIIEXPORT header");
    }

    while (tokeniser.hasMoreTokens())
    {
        auto token = tokeniser.nextToken();
        string::to_lower(token);

        if (token.length() == 0) continue;

        /* we skip invalid ase statements */
        if (token[0] != '*' && token[0] != '{' && token[0] != '}')
        {
            continue;
        }

        /* model mesh (originally contained within geomobject) */
        if (token == "*mesh")
        {
            /* finish existing surface */
            if (materialIndex != -1 && !vertices.empty())
            {
                _ase_submit_triangles(*model, materials, materialIndex, vertices, normals, texcoords, colours, faces);

                materialIndex = -1;
                colours.clear();
                texcoords.clear();
                faces.clear();
                normals.clear();
                vertices.clear();
            }
        }
        else if (token == "*mesh_numvertex")
        {
            // Parse the number to allocate space in the vertex vector
            auto numVertices = string::convert<std::size_t>(tokeniser.nextToken());
            vertices.resize(numVertices);
            normals.resize(numVertices);
        }
        else if (token == "*mesh_numfaces")
        {
            auto numFaces = string::convert<std::size_t>(tokeniser.nextToken());
            faces.resize(numFaces);
        }
        else if (token == "*mesh_numtvertex")
        {
            auto numTextureVertices = string::convert<std::size_t>(tokeniser.nextToken());
            texcoords.resize(numTextureVertices);
        }
        else if (token == "*mesh_numcvertex")
        {
            auto numColorVertices = string::convert<std::size_t>(tokeniser.nextToken());
            colours.resize(numColorVertices, Vector3(1.0, 1.0, 1.0));
        }
        /* mesh material reference. this usually comes at the end of 
         * geomobjects after the mesh blocks. we must assume that the
         * new mesh was already created so all we can do here is assign
         * the material reference id (shader index) now. */
        else if (token == "*material_ref")
        {
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= materials.size()) throw parser::ParseException("MATERIAL_REF index out of bounds >= MATERIAL_COUNT");

            materialIndex = static_cast<int>(index);
        }
        /* model mesh vertex */
        else if (token == "*mesh_vertex")
        {
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= vertices.size()) throw parser::ParseException("MESH_VERTEX index out of bounds >= MESH_NUMVERTEX");

            auto& vertex = vertices[index];
            vertex.x() = string::convert<double>(tokeniser.nextToken());
            vertex.y() = string::convert<double>(tokeniser.nextToken());
            vertex.z() = string::convert<double>(tokeniser.nextToken());
        }
        /* model mesh vertex normal */
        else if (token == "*mesh_vertexnormal")
        {
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= vertices.size()) throw parser::ParseException("MESH_VERTEXNORMAL index out of bounds >= MESH_NUMVERTEX");

            auto& normal = normals[index];
            normal.x() = string::convert<double>(tokeniser.nextToken());
            normal.y() = string::convert<double>(tokeniser.nextToken());
            normal.z() = string::convert<double>(tokeniser.nextToken());
        }
        /* model mesh face */
        else if (token == "*mesh_face")
        {
            // *MESH_FACE    0:    A:    3 B:    1 C:    2 AB:    0 BC:    0 CA:    0	 *MESH_SMOOTHING 0	 *MESH_MTLID 0
            auto index = string::convert<std::size_t>(string::trim_right_copy(tokeniser.nextToken(), ":"));

            if (index >= faces.size()) throw parser::ParseException("MESH_FACE index out of bounds >= MESH_NUMFACES");

            tokeniser.assertNextToken("A:");

            auto& face = faces[index];
            
            face.vertexIndices[0] = string::convert<std::size_t>(tokeniser.nextToken());

            tokeniser.assertNextToken("B:");

            face.vertexIndices[1] = string::convert<std::size_t>(tokeniser.nextToken());

            tokeniser.assertNextToken("C:");

            face.vertexIndices[2] = string::convert<std::size_t>(tokeniser.nextToken());

            if (face.vertexIndices[0] >= vertices.size()) throw parser::ParseException("MESH_FACE vertex index 0 out of bounds >= MESH_NUMFACES");
            if (face.vertexIndices[1] >= vertices.size()) throw parser::ParseException("MESH_FACE vertex index 1 out of bounds >= MESH_NUMFACES");
            if (face.vertexIndices[2] >= vertices.size()) throw parser::ParseException("MESH_FACE vertex index 2 out of bounds >= MESH_NUMFACES");

            tokeniser.skipTokens(9);
        }
        /* model texture vertex */
        else if (token == "*mesh_tvert")
        {
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= texcoords.size()) throw parser::ParseException("MESH_TVERT index out of bounds >= MESH_NUMTVERTEX");

            auto& texcoord = texcoords[index];
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

            if (index >= faces.size()) throw parser::ParseException("MESH_TFACE index out of bounds >= MESH_NUMFACES");

            auto& face = faces[index];

            face.texcoordIndices[0] = string::convert<std::size_t>(tokeniser.nextToken());
            face.texcoordIndices[1] = string::convert<std::size_t>(tokeniser.nextToken());
            face.texcoordIndices[2] = string::convert<std::size_t>(tokeniser.nextToken());

            if (face.texcoordIndices[0] >= texcoords.size()) throw parser::ParseException("MESH_TFACE texcoord index 0 out of bounds >= MESH_NUMTVERTEX");
            if (face.texcoordIndices[1] >= texcoords.size()) throw parser::ParseException("MESH_TFACE texcoord index 1 out of bounds >= MESH_NUMTVERTEX");
            if (face.texcoordIndices[2] >= texcoords.size()) throw parser::ParseException("MESH_TFACE texcoord index 2 out of bounds >= MESH_NUMTVERTEX");
        }
        /* model color vertex */
        else if (token == "*mesh_vertcol")
        {
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= colours.size()) throw parser::ParseException("MESH_VERTCOL index out of bounds >= MESH_NUMCVERTEX");

            auto& colour = colours[index];
            colour.x() = string::convert<double>(tokeniser.nextToken());
            colour.y() = string::convert<double>(tokeniser.nextToken());
            colour.z() = string::convert<double>(tokeniser.nextToken());
        }
        /* model color face */
        else if (token == "*mesh_cface")
        {
            // *MESH_CFACE 0    0   1   2
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= faces.size()) throw parser::ParseException("MESH_CFACE index out of bounds >= MESH_NUMFACES");

            auto& face = faces[index];

            face.colourIndices[0] = string::convert<std::size_t>(tokeniser.nextToken());
            face.colourIndices[1] = string::convert<std::size_t>(tokeniser.nextToken());
            face.colourIndices[2] = string::convert<std::size_t>(tokeniser.nextToken());

            if (face.colourIndices[0] >= colours.size()) throw parser::ParseException("MESH_CFACE colour index 0 out of bounds >= MESH_NUMCVERTEX");
            if (face.colourIndices[1] >= colours.size()) throw parser::ParseException("MESH_CFACE colour index 1 out of bounds >= MESH_NUMCVERTEX");
            if (face.colourIndices[2] >= colours.size()) throw parser::ParseException("MESH_CFACE colour index 2 out of bounds >= MESH_NUMCVERTEX");
        }
        else if (token == "*material_count")
        {
            auto numMaterials = string::convert<std::size_t>(tokeniser.nextToken());
            materials.resize(numMaterials);
        }
        /* model material */
        else if (token == "*material")
        {
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= materials.size()) throw parser::ParseException("MATERIAL index out of bounds >= MATERIAL_COUNT");

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
                    materials[index].materialName = string::trim_copy(tokeniser.nextToken(), "\"");
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
                            materials[index].diffuseBitmap = string::trim_copy(tokeniser.nextToken(), "\"");
                        }
                        else if (token ==  "*uvw_u_offset")
                        {
                            // Negate the u offset value
                            materials[index].uOffset = -string::convert<float>(tokeniser.nextToken());
                        }
                        else if (token == "*uvw_v_offset")
                        {
                            materials[index].vOffset = string::convert<float>(tokeniser.nextToken());
                        }
                        else if (token == "*uvw_u_tiling")
                        {
                            materials[index].uTiling = string::convert<float>(tokeniser.nextToken());
                        }
                        else if (token == "*uvw_v_tiling")
                        {
                            materials[index].vTiling = string::convert<float>(tokeniser.nextToken());
                        }
                        else if (token == "*uvw_angle")
                        {
                            materials[index].uvAngle = string::convert<float>(tokeniser.nextToken());
                        }
                    }
                } /* end map_diffuse block */
            } /* end material block */
        }
    }

    /* ydnar: finish existing surface */
    _ase_submit_triangles(*model, materials, materialIndex, vertices, normals, texcoords, colours, faces);

    return model;
}

} // namespace
