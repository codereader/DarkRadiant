#include "ModelImporterBase.h"

#include "ifilesystem.h"
#include "itextstream.h"
#include "imodelcache.h"
#include "string/case_conv.h"
#include "os/path.h"
#include "../picomodel/StaticModelNode.h"
#include "../picomodel/StaticModel.h"

namespace model
{

namespace
{
    // name may be absolute or relative
    inline std::string rootPath(const std::string& name)
    {
        return GlobalFileSystem().findRoot(
            path_is_absolute(name.c_str()) ? name : GlobalFileSystem().findFile(name)
        );
    }
}

ModelImporterBase::ModelImporterBase(const std::string& extension) :
    _extension(string::to_upper_copy(extension))
{

}

const std::string& ModelImporterBase::getExtension() const
{
    return _extension;
}

scene::INodePtr ModelImporterBase::loadModel(const std::string& modelName)
{
    // Initialise the paths, this is all needed for realisation
    std::string path = rootPath(modelName);
    std::string name = os::getRelativePath(modelName, path);

    // greebo: Path is empty for models in PK4 files, don't check this

    // Try to load the model from the given VFS path
    IModelPtr model = GlobalModelCache().getModel(name);

    if (!model)
    {
        rError() << "ModelImporterBase: Could not load model << " << modelName << std::endl;
        return scene::INodePtr();
    }

    // The cached model should be a StaticModel, otherwise we're in the wrong movie
    auto picoModel = std::dynamic_pointer_cast<StaticModel>(model);

    if (picoModel)
    {
        // Load was successful, construct a modelnode using this resource
        return std::make_shared<StaticModelNode>(picoModel);
    }
    else
    {
        rError() << "ModelImporterBase: Cached model is not a PicoModel?" << std::endl;
    }

    return scene::INodePtr();
}

}
