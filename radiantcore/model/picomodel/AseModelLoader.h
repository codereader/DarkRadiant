#pragma once

#include "ModelImporterBase.h"

namespace model
{

class AseModelLoader :
    public ModelImporterBase
{
public:
    AseModelLoader();

    // Load the given model from the path, VFS or absolute
    IModelPtr loadModelFromPath(const std::string& name) override;
};

} // namespace model
