#ifndef IMODELCACHE_H_
#define IMODELCACHE_H_

#include "imodule.h"
#include "imodel.h"
#include "inode.h"

const std::string MODULE_MODELCACHE("ModelCache"); 

namespace model {

/** Modelcache interface.
 */
class IModelCache :
	public RegisterableModule
{
public:
	/**
	 * greebo: This method returns the readily fabricated scene::Node 
	 *         containing the suitable model node. The modelPath is analysed
	 *         and the correct ModelLoader is invoked to create the node.
	 * 
	 * @returns: a valid scene::INodePtr, which is never NULL. If the model
	 *           could not be loaded, the fallback "NullModel" is returned.
	 */
	virtual scene::INodePtr getModelNode(const std::string& modelPath) = 0;

	/** 
	 * greebo: Get the IModel object for the given VFS path. The request is cached,
	 *         so calling this with the same path twice will return the same 
	 *         IModelPtr to save memory.
	 *
	 * This method is primarily used by the ModelLoaders to acquire their model data.
	 */ 
	virtual IModelPtr getModel(const std::string& modelPath) = 0;

	/** 
	 * greebo: Utility function to get the ModelLoader for a certain type.
	 * 
	 * @type: The model type (usually the file extension, e.g. lwo, ase).
	 */
	virtual ModelLoaderPtr getModelLoaderForType(const std::string& type) = 0;

	// Clears the modelcache
	virtual void clear() = 0;
};

} // namespace model

inline model::IModelCache& GlobalModelCache() {
	static model::IModelCache& _modelCache(
		*boost::static_pointer_cast<model::IModelCache>(
			module::GlobalModuleRegistry().getModule(MODULE_MODELCACHE) 
		)
	);
	return _modelCache;
}

#endif /* IMODELCACHE_H_ */
