#include "GltfModelLoader.h"

#include <istream>
//#include "AseModel.h"

#include "os/path.h"
#include "string/case_conv.h"
#include "stream/ScopedArchiveBuffer.h"

#include "../StaticModel.h"
#include "parser/ParseException.h"
#include "../picomodel/PicoModelLoader.h"

//https://github.com/jkuhlmann/cgltf
#define CGLTF_IMPLEMENTATION
#include "gltf/cgltf.h"
#include "../StaticModelSurface.h"

namespace
{
    class ScopeGuard
    {
        std::function<void()> mExitFunc;
    public:
        ScopeGuard(std::function<void()> exitFunc)
            :mExitFunc(exitFunc)
        {

        }
        
        ~ScopeGuard()
        {
            mExitFunc();
        }
    };
}

namespace model
{

    GltfModelLoader::GltfModelLoader() :
        ModelImporterBase("GLB")
    {}

    IModelPtr GltfModelLoader::loadModelFromPath(const std::string& path)
    {
        // Open an ArchiveFile to load
        auto file = path_is_absolute(path.c_str()) ?
            GlobalFileSystem().openFileInAbsolutePath(path) :
            GlobalFileSystem().openFile(path);
        
        std::string fileName = path_is_absolute(path.c_str()) ?
            path : GlobalFileSystem().findFile(path) + path;

        cgltf_options options = {};
        cgltf_data* gltfData = NULL;
        cgltf_result result = cgltf_parse_file(&options, fileName.c_str(), &gltfData);
        
        if (result != cgltf_result_success)
        {
            //Failed to load
            rError() << "Failed to parse GLB file " << path << std::endl;
            return IModelPtr();
        }

        ScopeGuard freeCGLTF([gltfData] {
            cgltf_free(gltfData);
        });

        result = cgltf_load_buffers(&options, gltfData, fileName.c_str());

        if (result != cgltf_result_success)
        {
            //Failed to load
            rError() << "Failed to parse GLTF buffers " << path << std::endl;
            return IModelPtr();
        }      
        
        //Note: "mesh" = how many individual blender objects there are.
        //so if the model consists of 3 objects, then: you have 3 meshes.

        if (gltfData->meshes_count == 0)
        {
            //No mesh
            rError() << "GLB file has no meshes " << path << std::endl;
            return IModelPtr();
        }

        // Maps to help with hierarchy
        std::unordered_map<cgltf_mesh*, cgltf_node*> meshToNodeMap;

        for (size_t i = 0; i < gltfData->nodes_count; i++)
        {
            cgltf_node* node = &gltfData->nodes[i];
            if (node->mesh)
            {
                meshToNodeMap.emplace(node->mesh, node);
            }
        }

        std::vector<StaticModelSurfacePtr> staticSurfaces;

        for (size_t i = 0; i < gltfData->meshes_count; i++)
        {
            cgltf_mesh* mesh = &gltfData->meshes[i];
            if (mesh->primitives_count <= 0)
            {
                continue;
            }

            // Handle transformation of the mesh in the node, including parents
            cgltf_node* node = meshToNodeMap[mesh];
            float mtx[16];
            cgltf_node_transform_world(node, mtx);
            auto transform = Matrix4::byColumns(mtx[0], mtx[1], mtx[2], mtx[3],
                mtx[4], mtx[5], mtx[6], mtx[7],
                mtx[8], mtx[9], mtx[10], mtx[11],
                mtx[12], mtx[13], mtx[14], mtx[15]);

            for (size_t j = 0; j < mesh->primitives_count; j++)
            {
                auto& primitive = mesh->primitives[j];
                size_t numVerts = 0;
                size_t numIndices = 0;
                const uint8_t* positions = nullptr;
                const uint8_t* normals = nullptr;
                const uint8_t* texcoords = nullptr;

                for (size_t k = 0; k < primitive.attributes_count; k++)
                {
                    cgltf_attribute& attribute = primitive.attributes[k];
                    std::string attributeName = attribute.name;
                    if (attributeName == "POSITION")
                    {
                        cgltf_accessor* accessor = attribute.data;
                        if (accessor->component_type != cgltf_component_type_r_32f)
                        {
                            rError() << "GLB file has a position attribute that doesn't use floats " << path << std::endl;
                            return IModelPtr();
                        }
                        if (accessor->type != cgltf_type_vec3)
                        {
                            rError() << "GLB file has a position attribute that isn't a Vec3 " << path << std::endl;
                            return IModelPtr();
                        }

                        numVerts = accessor->count;
                        positions = reinterpret_cast<const uint8_t*>(cgltf_buffer_view_data(accessor->buffer_view));
                        
                    }
                    else if (attributeName == "NORMAL")
                    {
                        cgltf_accessor* accessor = attribute.data;
                        if (accessor->component_type != cgltf_component_type_r_32f)
                        {
                            rError() << "GLB file has a normal attribute that doesn't use floats " << path << std::endl;
                            return IModelPtr();
                        }
                        if (accessor->type != cgltf_type_vec3)
                        {
                            rError() << "GLB file has a normal attribute that isn't a Vec3 " << path << std::endl;
                            return IModelPtr();
                        }

                        normals = reinterpret_cast<const uint8_t*>(cgltf_buffer_view_data(accessor->buffer_view));
                    }
                    else if (attributeName == "TEXCOORD_0")
                    {
                        cgltf_accessor* accessor = attribute.data;
                        if (accessor->component_type != cgltf_component_type_r_32f)
                        {
                            rError() << "GLB file has a texcoord_0 attribute that doesn't use floats " << path << std::endl;
                            return IModelPtr();
                        }
                        if (accessor->type != cgltf_type_vec2)
                        {
                            rError() << "GLB file has a texcoord_0 attribute that isn't a Vec2 " << path << std::endl;
                            return IModelPtr();
                        }

                        texcoords = reinterpret_cast<const uint8_t*>(cgltf_buffer_view_data(accessor->buffer_view));
                    }
                }

                if (primitive.indices)
                {
                    const uint8_t* indicesPtr = reinterpret_cast<const uint8_t*>(cgltf_buffer_view_data(primitive.indices->buffer_view));
                    numIndices = primitive.indices->count;
                    cgltf_component_type indexType = primitive.indices->component_type;

                    std::vector<MeshVertex> vertices;
                    std::vector<unsigned int> indices;
                    indices.resize(numIndices);

                    if (indexType == cgltf_component_type_r_8u)
                    {
                        for (size_t index = 0; index < numIndices; index++)
                        {
                            indices[index] = indicesPtr[index];
                        }
                    }
                    else if (indexType == cgltf_component_type_r_16u)
                    {
                        const uint16_t* ptrAs16 = reinterpret_cast<const uint16_t*>(indicesPtr);
                        for (size_t index = 0; index < numIndices; index++)
                        {
                            indices[index] = ptrAs16[index];
                        }
                    }
                    else if (indexType == cgltf_component_type_r_32u)
                    {
                        const uint32_t* ptrAs32 = reinterpret_cast<const uint32_t*>(indicesPtr);
                        for (size_t index = 0; index < numIndices; index++)
                        {
                            indices[index] = ptrAs32[index];
                        }
                    }
                    else
                    {
                        rError() << "GLB file doesn't indices of an unsigned type " << path << std::endl;
                        return IModelPtr();
                    }

                    // GLTF is CCW winding order, but dark radiant wants CW
                    size_t numTris = numIndices / 3;
                    for (size_t tri = 0; tri < numTris; tri++)
                    {
                        std::swap(indices[tri * 3], indices[tri * 3 + 1]);
                    }

                    vertices.reserve(numVerts);
                    const std::array<float, 3>* posVec3 = reinterpret_cast<const std::array<float, 3>*>(positions);
                    const std::array<float, 3>* normalsVec3 = reinterpret_cast<const std::array<float, 3>*>(normals);
                    const std::array<float, 2>* texcoordsVec2 = reinterpret_cast<const std::array<float, 2>*>(texcoords);
                    for (size_t vertex = 0; vertex < numVerts; vertex++)
                    {
                        Vector3 pos = posVec3 ? Vertex3(posVec3[vertex][0], posVec3[vertex][1], posVec3[vertex][2]) : Vertex3();
                        Vector3 normal = normalsVec3 ? Normal3(-normalsVec3[vertex][0], -normalsVec3[vertex][1], -normalsVec3[vertex][2]) : Normal3();

                        pos = transform.transformPoint(pos);
                        normal = transform.transformDirection(normal);
                        // NOTE: Normals are negated due to winding order swap from CCW to CW
                        vertices.emplace_back(pos, normal,
                            texcoordsVec2 ? TexCoord2f(texcoordsVec2[vertex][0], texcoordsVec2[vertex][1]) : TexCoord2f());
                    }

                    auto& staticSurface = staticSurfaces.emplace_back(std::make_shared<StaticModelSurface>(std::move(vertices), std::move(indices)));
                    if (primitive.material && primitive.material->name)
                    {
                        staticSurface->setDefaultMaterial(primitive.material->name);
                        staticSurface->setActiveMaterial(staticSurface->getActiveMaterial());
                    }
                }
                else
                {
                    rError() << "GLB file doesn't seem to have indices " << path << std::endl;
                    return IModelPtr();
                }
            }
        }

        auto staticModel = std::make_shared<StaticModel>(staticSurfaces);

        // Set the filename
        staticModel->setFilename(os::getFilename(fileName));
        staticModel->setModelPath(path);

        return staticModel; //Return the model.
    }

}
