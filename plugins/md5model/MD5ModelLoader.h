#ifndef MD5MODELLOADER_H_
#define MD5MODELLOADER_H_

#include "imodel.h"
#include "MD5Model.h"

class ArchiveFile;

namespace md5 {

class MD5ModelLoader : 
	public ModelLoader
{
public:
	// ModelLoader implementation
	// Returns a new ModelNode for the given model name
	virtual scene::INodePtr loadModel(const std::string& modelName);

	// Documentation: See imodel.h
	model::IModelPtr loadModelFromPath(const std::string& name);
	
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
};
typedef boost::shared_ptr<MD5ModelLoader> MD5ModelLoaderPtr;

} // namespace md5

#endif /*MD5MODELLOADER_H_*/
