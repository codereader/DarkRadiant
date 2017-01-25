#pragma once

#include <map>
#include <string>
#include "imodelcache.h"
#include "icommandsystem.h"

namespace model {

class ModelCache :
	public IModelCache
{
	// The container maps model names to instances
	typedef std::map<std::string, IModelPtr> ModelMap;
	ModelMap _modelMap;

	// Flag to disable the cache on demand (used during clear())
	bool _enabled;

public:
	ModelCache();

	// greebo: For documentation, see the abstract base class.
	scene::INodePtr getModelNode(const std::string& modelPath) override;

	// greebo: For documentation, see the abstract base class.
	IModelPtr getModel(const std::string& modelPath) override;

	// Clears the cache
	void clear() override;

	// Command target: this reloads all models in the map
	void refreshModels(const cmd::ArgumentList& args);
	// Command target: this reloads all selected models in the map
	void refreshSelectedModels(const cmd::ArgumentList& args);

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;
};

} // namespace model
