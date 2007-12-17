#ifndef NULLMODELLOADER_H_
#define NULLMODELLOADER_H_

#include "imodel.h"
#include "stream/textstream.h"
#include "nullmodel.h"

namespace model {

class NullModelLoader : 
	public ModelLoader
{
public:
	scene::INodePtr loadModel(ArchiveFile& file) {
		return SingletonNullModel();
	}
  
  	// Required function, not implemented.
	model::IModelPtr loadModelFromPath(const std::string& name) {
		return model::IModelPtr();
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
		static NullModelLoader _instance;
		return _instance;
	}
};

} // namespace model

#endif /*NULLMODELLOADER_H_*/
