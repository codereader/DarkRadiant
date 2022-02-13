#include "ModelCache.h"

#include "ieclass.h"
#include "ifilesystem.h"
#include "imodel.h"
#include "imd5model.h"
#include "imd5anim.h"
#include "iparticles.h"
#include "iparticlenode.h"

#include <iostream>
#include "os/path.h"
#include "os/file.h"

#include "module/StaticModule.h"
#include <functional>

#include "map/algorithm/Models.h"

namespace model
{

ModelCache::ModelCache() :
	_enabled(true)
{}

scene::INodePtr ModelCache::getModelNode(const std::string& modelPath)
{
	// Check if we have a reference to a modeldef
	IModelDefPtr modelDef = GlobalEntityClassManager().findModel(modelPath);

	// The actual model path (is usually the same as the incoming modelPath)
	std::string actualModelPath(modelPath);

	if (modelDef)
	{
		// We have a valid modelDef, override the model path
		actualModelPath = modelDef->mesh;
	}

	// Get the extension of this model
	std::string type = actualModelPath.substr(actualModelPath.rfind(".") + 1);

	if (type == "prt")
	{
		// This is a particle, pass the call to the Particles Manager
		return GlobalParticlesManager().createParticleNode(actualModelPath);
	}

	// Get a suitable model loader
	IModelImporterPtr modelLoader = GlobalModelFormatManager().getImporter(type);

	// Try to construct a model node using the suitable loader
	auto node =  modelLoader->loadModel(actualModelPath);

	if (node)
	{
		// For MD5 models, apply the idle animation by default
		if (modelDef)
		{
			model::ModelNodePtr modelNode = Node_getModel(node);

			if (!modelNode)
			{
				return node;
			}

			// Set the animation to play
			try
			{
				md5::IMD5Model& md5model = dynamic_cast<md5::IMD5Model&>(modelNode->getIModel());

				// Look up the "idle" anim if there is one
				IModelDef::Anims::const_iterator found = modelDef->anims.find("idle");

				if (found != modelDef->anims.end())
				{
					// Load the anim
					md5::IMD5AnimPtr anim = GlobalAnimationCache().getAnim(found->second);

					if (anim)
					{
						md5model.setAnim(anim);
						md5model.updateAnim(0);
					}
				}
			}
			catch (std::bad_cast&)
			{
				// not an MD5 model, do nothing
			}
		}

		// Model load was successful
		return node;
	}

	// The model load failed, let's return a NullModel
	return loadNullModel(actualModelPath);
}

IModelPtr ModelCache::getModel(const std::string& modelPath)
{
	// Try to lookup the existing model
	auto found = _modelMap.find(modelPath);

	if (_enabled && found != _modelMap.end())
	{
		return found->second;
	}

	// The model is not cached or the cache is disabled, load afresh

	// Get the extension of this model
	std::string type = os::getExtension(modelPath);

	// Find a suitable model loader
	IModelImporterPtr modelLoader = GlobalModelFormatManager().getImporter(type);

	IModelPtr model = modelLoader->loadModelFromPath(modelPath);

	if (model)
	{
		// Model successfully loaded, insert a reference into the map
		_modelMap.emplace(modelPath, model);
	}

	return model;
}

scene::INodePtr ModelCache::getModelNodeForStaticResource(const std::string& resourcePath)
{
    // Get the extension of this model
    auto extension = os::getExtension(resourcePath);

    // Find a suitable model loader
    auto modelLoader = GlobalModelFormatManager().getImporter(extension);

    auto fullPath = module::GlobalModuleRegistry().getApplicationContext().getRuntimeDataPath();
    fullPath += "resources/" + resourcePath;

    auto node = modelLoader->loadModel(fullPath);

    return node ? node : loadNullModel(resourcePath);
}

scene::INodePtr ModelCache::loadNullModel(const std::string& modelPath)
{
    auto nullModelLoader = GlobalModelFormatManager().getImporter("");

    return nullModelLoader->loadModel(modelPath);
}

void ModelCache::removeModel(const std::string& modelPath)
{
	// greebo: Disable the modelcache. During map::clear(), the nodes
	// get cleared, which might trigger a loopback to insert().
	_enabled = false;

	ModelMap::iterator found = _modelMap.find(modelPath);

	if (found != _modelMap.end())
	{
		_modelMap.erase(found);
	}

	// Allow usage of the modelnodemap again.
	_enabled = true;
}

void ModelCache::clear()
{
	// greebo: Disable the modelcache. During map::clear(), the nodes
	// get cleared, which might trigger a loopback to insert().
	_enabled = false;

	_modelMap.clear();

	// Allow usage of the modelnodemap again.
	_enabled = true;
}

sigc::signal<void> ModelCache::signal_modelsReloaded()
{
	return _sigModelsReloaded;
}

// RegisterableModule implementation
const std::string& ModelCache::getName() const
{
	static std::string _name(MODULE_MODELCACHE);
	return _name;
}

const StringSet& ModelCache::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_MODELFORMATMANAGER);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
	}

	return _dependencies;
}

void ModelCache::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	GlobalCommandSystem().addCommand("RefreshModels",
		std::bind(&ModelCache::refreshModelsCmd, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("RefreshSelectedModels",
		std::bind(&ModelCache::refreshSelectedModelsCmd, this, std::placeholders::_1));
}

void ModelCache::shutdownModule()
{
	clear();
}

void ModelCache::refreshModels(bool blockScreenUpdates)
{
	map::algorithm::refreshModels(blockScreenUpdates);
}

void ModelCache::refreshSelectedModels(bool blockScreenUpdates)
{
	map::algorithm::refreshSelectedModels(blockScreenUpdates);
}

void ModelCache::refreshModelsCmd(const cmd::ArgumentList& args)
{
	map::algorithm::refreshModels(true);
}

void ModelCache::refreshSelectedModelsCmd(const cmd::ArgumentList& args)
{
	map::algorithm::refreshSelectedModels(true);
}

// The static module
module::StaticModuleRegistration<ModelCache> modelCacheModule;

} // namespace model
