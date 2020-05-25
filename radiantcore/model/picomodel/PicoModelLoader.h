#pragma once

#include "imodel.h"

typedef struct picoModule_s picoModule_t;

namespace model
{

class PicoModelLoader :
	public IModelImporter
{
private:
	const picoModule_t* _module;

	// Supported file extension in UPPERCASE (ASE, LWO, whatever)
	std::string _extension;

public:
	PicoModelLoader(const picoModule_t* module, const std::string& extension);

	const std::string& getExtension() const override;

	// Returns a new ModelNode for the given model name
	scene::INodePtr loadModel(const std::string& modelName) override;

  	// Load the given model from the VFS path
	IModelPtr loadModelFromPath(const std::string& name) override;
};
typedef std::shared_ptr<PicoModelLoader> PicoModelLoaderPtr;

} // namespace model
