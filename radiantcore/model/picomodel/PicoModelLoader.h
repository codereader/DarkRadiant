#pragma once

#include "ModelImporterBase.h"

typedef struct picoModule_s picoModule_t;

namespace model
{

class PicoModelLoader :
	public ModelImporterBase
{
private:
	const picoModule_t* _module;

public:
	PicoModelLoader(const picoModule_t* module, const std::string& extension);

  	// Load the given model from the path, VFS or absolute
	IModelPtr loadModelFromPath(const std::string& name) override;
};

} // namespace model
