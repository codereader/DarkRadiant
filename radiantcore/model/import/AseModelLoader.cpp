#include "AseModelLoader.h"

#include "../picomodel/lib/picomodel.h"
#include "os/path.h"
#include "string/case_conv.h"

#include "../StaticModel.h"
#include "../picomodel/PicoModelLoader.h"

extern "C"
{
    extern const picoModule_t picoModuleASE;
}

namespace model
{

namespace
{

size_t picoInputStreamReam(void* inputStream, unsigned char* buffer, size_t length)
{
    return reinterpret_cast<InputStream*>(inputStream)->read(buffer, length);
}

}

AseModelLoader::AseModelLoader() :
    ModelImporterBase("ASE")
{}

IModelPtr AseModelLoader::loadModelFromPath(const std::string& path)
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

    auto* model = PicoModuleLoadModelStream(
        &picoModuleASE,
        &file->getInputStream(),
        picoInputStreamReam,
        file->size(),
        0
    );

    // greebo: Check if the model load was successful
    if (!model || model->numSurfaces == 0)
    {
        // Model is either NULL or has no surfaces, this must've failed
        return IModelPtr();
    }

    auto surfaces = PicoModelLoader::CreateSurfaces(model, string::to_lower_copy(getExtension()));

    auto modelObj = std::make_shared<StaticModel>(surfaces);
    
    // Set the filename
    modelObj->setFilename(os::getFilename(file->getName()));
    modelObj->setModelPath(path);

    PicoFreeModel(model);

    return modelObj;
}

}
