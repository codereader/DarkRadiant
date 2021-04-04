#pragma once

#include "imodel.h"

namespace model
{

class ModelImporterBase :
    public IModelImporter
{
private:
    // Supported file extension in UPPERCASE (ASE, LWO, whatever)
    std::string _extension;

public:
    ModelImporterBase(const std::string& extension);

    const std::string& getExtension() const override;

    // Returns a new ModelNode for the given model name
    scene::INodePtr loadModel(const std::string& modelName) override;
};

}
