#ifndef NULLMODELLOADER_H_
#define NULLMODELLOADER_H_

#include "imodel.h"
#include "ifilesystem.h"
#include "os/path.h"

#include "NullModelNode.h"

namespace model {

namespace {
	// name may be absolute or relative
	inline std::string rootPath(const std::string& name) {
		return GlobalFileSystem().findRoot(
			path_is_absolute(name.c_str()) ? name : GlobalFileSystem().findFile(name)
		);
	}
} // namespace

class NullModelLoader;
typedef boost::shared_ptr<NullModelLoader> NullModelLoaderPtr;

class NullModelLoader : 
	public ModelLoader
{
public:
	virtual scene::INodePtr loadModel(const std::string& modelName) {
		// Initialise the paths, this is all needed for realisation
		std::string path = rootPath(modelName);
		std::string name = os::getRelativePath(modelName, path);

		// Try to load the model from the given VFS path
		NullModelPtr model = 
			boost::static_pointer_cast<NullModel>(loadModelFromPath(name));

		model->setModelPath(modelName);
		model->setFilename(name);

		// Construct a NullModelNode using this resource
		return NullModelNodePtr(new NullModelNode(model));
	}
  
  	// Required function, not implemented.
	IModelPtr loadModelFromPath(const std::string& name) {
		NullModelPtr model(new NullModel);
		model->setModelPath(name);
		return model;
	}
  
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name(MODULE_MODELLOADER + "NULL");
		return _name;
	}
	
	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies; // no dependencies
		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << getName().c_str() << "::initialiseModule called.\n";
	}
	
	static NullModelLoader& Instance() {
		return *InstancePtr();
	}
	
	static NullModelLoaderPtr& InstancePtr() {
		static NullModelLoaderPtr _instancePtr(new NullModelLoader);
		return _instancePtr;
	}
};

} // namespace model

#endif /*NULLMODELLOADER_H_*/
