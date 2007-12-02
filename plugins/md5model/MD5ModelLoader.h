#ifndef MD5MODELLOADER_H_
#define MD5MODELLOADER_H_

#include "imodel.h"

namespace md5 {

class MD5ModelLoader : 
	public ModelLoader
{
public:
	// ModelLoader implementation
	scene::INodePtr loadModel(ArchiveFile& file);

	// Not yet implemented
	model::IModelPtr loadModelFromPath(const std::string& name);
	
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
};
typedef boost::shared_ptr<MD5ModelLoader> MD5ModelLoaderPtr;

} // namespace md5

#endif /*MD5MODELLOADER_H_*/
