#include "AseModelLoader.h"

#include <istream>
#include "AseModel.h"

#include "os/path.h"
#include "string/case_conv.h"

#include "../StaticModel.h"
#include "parser/ParseException.h"
#include "../picomodel/PicoModelLoader.h"

namespace model
{

AseModelLoader::AseModelLoader() :
    ModelImporterBase("ASE")
{}

IModelPtr AseModelLoader::loadModelFromPath(const std::string& path)
{
    // Open an ArchiveFile to load
    auto file = path_is_absolute(path.c_str()) ?
        GlobalFileSystem().openTextFileInAbsolutePath(path) :
        GlobalFileSystem().openTextFile(path);

    if (!file)
    {
        rError() << "Failed to load model " << path << std::endl;
        return IModelPtr();
    }

    try
    {
        // Parse the ASE model data from the given stream
        std::istream stream(&(file->getInputStream()));
        auto model = AseModel::CreateFromStream(stream);

        // Convert the AseModel to StaticModelSurfaces, destructing it during the process
        std::vector<StaticModelSurfacePtr> staticSurfaces;

        for (auto& aseSurface : model->getSurfaces())
        {
            // Move the vertex and index data to construct the StaticModelSurface
            auto& staticSurface = staticSurfaces.emplace_back(std::make_shared<StaticModelSurface>(
                std::move(aseSurface.vertices), std::move(aseSurface.indices)));

            staticSurface->setDefaultMaterial(PicoModelLoader::CleanupShaderName(aseSurface.material));
        }

        auto staticModel = std::make_shared<StaticModel>(staticSurfaces);

        // Set the filename
        staticModel->setFilename(os::getFilename(file->getName()));
        staticModel->setModelPath(path);

        return staticModel;
    }
    catch (const parser::ParseException& ex)
    {
        rError() << "AseModelLoader: " << ex.what() << std::endl;
        return IModelPtr();
    }
}

}
