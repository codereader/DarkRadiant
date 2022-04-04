#include "FbxModelLoader.h"

#include <istream>

#include "openfbx/ofbx.h"

#include "os/path.h"
#include "string/case_conv.h"
#include "stream/ScopedArchiveBuffer.h"

#include "FbxSurface.h"
#include "../StaticModel.h"
#include "../StaticModelSurface.h"

namespace model
{

FbxModelLoader::FbxModelLoader() :
    ModelImporterBase("FBX")
{}

namespace
{

inline MeshVertex ConstructMeshVertex(const ofbx::Geometry& geometry, int index)
{
    auto vertices = geometry.getVertices();
    auto normals = geometry.getNormals();
    auto uvs = geometry.getUVs();
    auto colours = geometry.getColors();

    return MeshVertex(
        Vertex3(vertices[index].x, vertices[index].y, vertices[index].z),
        normals != nullptr ? Normal3(normals[index].x, normals[index].y, normals[index].z) : Normal3(1, 0, 0),
        uvs != nullptr ? TexCoord2f(uvs[index].x, 1.0 - uvs[index].y) : TexCoord2f(0, 0), // invert v
        colours != nullptr ? Vector3(colours[index].x, colours[index].y, colours[index].z) : Vector3(1, 1, 1)
    );
}

}

IModelPtr FbxModelLoader::loadModelFromPath(const std::string& path)
{
    // Open an ArchiveFile to load
    auto file = path_is_absolute(path.c_str()) ?
        GlobalFileSystem().openFileInAbsolutePath(path) :
        GlobalFileSystem().openFile(path);

    if (!file)
    {
        rError() << "Failed to load model " << path << std::endl;
        return IModelPtr();
    }

    // Load the model data from the given stream
    archive::ScopedArchiveBuffer data(*file);

    auto scene = ofbx::load(static_cast<ofbx::u8*>(data.buffer),
        static_cast<int>(data.length), (ofbx::u64)ofbx::LoadFlags::TRIANGULATE);

    if (!scene)
    {
        rError() << "Failed to load FBX model " << path << std::endl;
        return IModelPtr();
    }

    std::vector<FbxSurface> surfaces;

    for (int meshIndex = 0; meshIndex < scene->getMeshCount(); ++meshIndex)
    {
        auto mesh = scene->getMesh(meshIndex);
        auto geometry = mesh->getGeometry();

        // Assign the materials for each surface
        for (int m = 0; m < mesh->getMaterialCount(); ++m)
        {
            auto material = mesh->getMaterial(m);
            surfaces.emplace_back().setMaterial(material->name);
        }

        if (surfaces.empty())
        {
            surfaces.emplace_back().setMaterial("Material"); // create at least one surface
        }

        auto materials = geometry->getMaterials();
        auto faceIndices = geometry->getFaceIndices();

        for (int i = 0; i < geometry->getIndexCount(); i += 3)
        {
            // Material index is assigned per triangle
            auto polyIndex = i / 3;
            auto materialIndex = materials ? materials[polyIndex] : 0; // put into first material by default

            // Reverse the poly indices to get the CCW order
            auto indexA = (faceIndices[i + 2] * -1) - 1; // last index is negative and 1-based
            auto indexB = faceIndices[i + 1];
            auto indexC = faceIndices[i + 0];

            auto& surface = surfaces[materialIndex];

            surface.addVertex(ConstructMeshVertex(*geometry, indexA));
            surface.addVertex(ConstructMeshVertex(*geometry, indexB));
            surface.addVertex(ConstructMeshVertex(*geometry, indexC));
        }

        // Apply the global transformation matrix
        auto t = geometry->getGlobalTransform();
#if 0
        auto transform = Matrix4::byColumns(
            t.m[0], t.m[1], t.m[2], t.m[3],
            t.m[4], t.m[5], t.m[6], t.m[7],
            t.m[8], t.m[9], t.m[10], t.m[11],
            t.m[12], t.m[13], t.m[14], t.m[15]
        );
#endif
        // "Objects in the FBX SDK are always created in the right handed, Y-Up axis system"
        auto transform = Matrix4::getIdentity();

        if (scene->getGlobalSettings()->UpAxis == ofbx::UpVector_AxisY)
        {
            transform = transform.getPremultipliedBy(Matrix4::getRotationForEulerXYZDegrees(Vector3(90, 0, 0)));
        }
    }

    // Construct a set of static surfaces from the FBX surfaces, destroying them on the go
    std::vector<StaticModelSurfacePtr> staticSurfaces;

    for (auto& fbxSurface : surfaces)
    {
        auto& staticSurface = staticSurfaces.emplace_back(std::make_shared<StaticModelSurface>(
            std::move(fbxSurface.getVertexArray()), std::move(fbxSurface.getIndexArray())));

        staticSurface->setDefaultMaterial(fbxSurface.getMaterial());
        staticSurface->setActiveMaterial(staticSurface->getDefaultMaterial());
    }

    surfaces.clear();

    auto staticModel = std::make_shared<StaticModel>(staticSurfaces);

    // Set the filename
    staticModel->setFilename(os::getFilename(file->getName()));
    staticModel->setModelPath(path);

    return staticModel;
}

}
