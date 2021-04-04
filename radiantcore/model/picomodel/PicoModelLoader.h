#pragma once

#include "../import/ModelImporterBase.h"
#include "../StaticModel.h"

typedef struct picoModule_s picoModule_t;
typedef struct picoSurface_s picoSurface_t;
typedef struct picoModel_s picoModel_t;

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

public:
    static std::vector<StaticModelSurfacePtr> CreateSurfaces(picoModel_t* picoModel, const std::string& extension);

    static std::string DetermineDefaultMaterial(picoSurface_t* picoSurface, const std::string& extension);

private:
    static std::string CleanupShaderName(const std::string& inName);

    static StaticModelSurfacePtr CreateSurface(picoSurface_t* picoSurface, const std::string& extension);
};

} // namespace model
