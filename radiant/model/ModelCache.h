#pragma once

#include <map>
#include <string>
#include "imodelcache.h"
#include "icommandsystem.h"

namespace model
{

class ModelCache :
	public IModelCache
{
private:
	// The container maps model names to instances
	typedef std::map<std::string, IModelPtr> ModelMap;
	ModelMap _modelMap;

	// Flag to disable the cache on demand (used during clear())
	bool _enabled;

	sigc::signal<void> _sigModelsReloaded;

public:
	ModelCache();

	// greebo: For documentation, see the abstract base class.
	scene::INodePtr getModelNode(const std::string& modelPath) override;

	// greebo: For documentation, see the abstract base class.
	IModelPtr getModel(const std::string& modelPath) override;

	// Clear methods
	void removeModel(const std::string& modelPath) override;
	void clear() override;

	// Public events
	sigc::signal<void> signal_modelsReloaded() override;

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;

private:
	// Command targets
	void refreshModels(const cmd::ArgumentList& args);
	void refreshSelectedModels(const cmd::ArgumentList& args);
};

} // namespace model
