#pragma once

#include "imodule.h"
#include "imodel.h"
#include "inode.h"
#include <sigc++/signal.h>

namespace model 
{

/** Modelcache interface.
 */
class IModelCache :
	public RegisterableModule
{
public:
	/**
	 * greebo: This method returns the readily fabricated scene::Node
	 * containing the suitable model node. The modelPath is analysed
	 * and the correct ModelLoader is invoked to create the node.
	 *
	 * @returns: a valid scene::INodePtr, which is never empty. If the model
	 * could not be loaded, the fallback "NullModel" is returned.
	 *
	 * Note that the model path should refer to an actual object in the VFS,
	 * modelPaths referring to modelDef declarations need to be resolved
	 * by the caller.
	 */
	virtual scene::INodePtr getModelNode(const std::string& modelPath) = 0;

	/**
	 * greebo: Get the IModel object for the given VFS path. The request is cached,
	 * so calling this with the same path twice will return the same
	 * IModelPtr to save memory.
	 *
	 * This method is primarily used by the ModelLoaders to acquire their model data.
	 */
	virtual IModelPtr getModel(const std::string& modelPath) = 0;

    // Loads a model from the static resources in DarkRadiant's runtime data/resources folder
    virtual scene::INodePtr getModelNodeForStaticResource(const std::string& resourcePath) = 0;

	// This reloads all models in the map
	virtual void refreshModels(bool blockScreenUpdates = true) = 0;

	// This reloads all selected models in the map
	virtual void refreshSelectedModels(bool blockScreenUpdates = true) = 0;

	// Clears a specific model from the cache
	virtual void removeModel(const std::string& modelPath) = 0;

	// Clears the modelcache
	virtual void clear() = 0;

	/// Signal emitted after models are reloaded
	virtual sigc::signal<void> signal_modelsReloaded() = 0;
};

} // namespace model

constexpr const char* const MODULE_MODELCACHE("ModelCache");

inline model::IModelCache& GlobalModelCache()
{
	static module::InstanceReference<model::IModelCache> _reference(MODULE_MODELCACHE);
	return _reference;
}
