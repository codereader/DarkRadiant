#pragma once

#include "ModelImporterBase.h"

namespace model
{

class FbxModelLoader :
    public ModelImporterBase
{
public:
    FbxModelLoader();

    // Load the given model from the path, VFS or absolute
    IModelPtr loadModelFromPath(const std::string& name) override;
};

} // namespace model
