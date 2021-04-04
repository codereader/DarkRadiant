#include "AseModelLoader.h"

#include <istream>
#include "AseModel.h"

#include "os/path.h"
#include "string/case_conv.h"

#include "../StaticModel.h"

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

    std::istream stream(&(file->getInputStream()));
    auto model = AseModel::CreateFromStream(stream);

    auto staticModel = std::make_shared<StaticModel>(model->getSurfaces());
    
    // Set the filename
    staticModel->setFilename(os::getFilename(file->getName()));
    staticModel->setModelPath(path);

    return staticModel;
}

}
