#ifndef PICOMODELLOADER_H_
#define PICOMODELLOADER_H_

#include "imodel.h"

typedef struct picoModule_s picoModule_t;

namespace model {

class PicoModelLoader : 
	public ModelLoader
{
	const picoModule_t* _module;
	
	// Supported file extension in UPPERCASE (ASE, LWO, whatever)
	std::string _extension;
	
	// The resulting name of the module (ModelLoaderASE, for instance)
	std::string _moduleName;
public:
	PicoModelLoader(const picoModule_t* module, const std::string& extension);

	// Returns a new ModelNode for the given model name
	virtual scene::INodePtr loadModel(const std::string& modelName);
  
  	// Load the given model from the VFS path
	IModelPtr loadModelFromPath(const std::string& name);

	// RegisterableModule implementation
  	virtual const std::string& getName() const;
  	virtual const StringSet& getDependencies() const;
  	virtual void initialiseModule(const ApplicationContext& ctx);
};
typedef boost::shared_ptr<PicoModelLoader> PicoModelLoaderPtr;

} // namespace model

#endif /* PICOMODELLOADER_H_ */
