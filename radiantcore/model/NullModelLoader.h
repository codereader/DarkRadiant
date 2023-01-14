#pragma once

#include "imodel.h"
#include "ifilesystem.h"
#include "os/path.h"

#include "NullModelNode.h"

namespace model
{

namespace 
{
	// name may be absolute or relative
	inline std::string rootPath(const std::string& name) {
		return GlobalFileSystem().findRoot(
			path_is_absolute(name.c_str()) ? name : GlobalFileSystem().findFile(name)
		);
	}
} // namespace

class NullModelLoader;
typedef std::shared_ptr<NullModelLoader> NullModelLoaderPtr;

class NullModelLoader :
	public IModelImporter
{
public:
	// NullModelLoader returns an empty extension
	const std::string& getExtension() const override
	{
		static std::string _ext;
		return _ext;
	}

	scene::INodePtr loadModel(const std::string& modelName) override
	{
		// Initialise the paths, this is all needed for realisation
		std::string path = rootPath(modelName);
		std::string name = os::getRelativePath(modelName, path);

		// Try to load the model from the given VFS path
		NullModelPtr model =
			std::static_pointer_cast<NullModel>(loadModelFromPath(name));

		model->setModelPath(modelName);
		model->setFilename(name);

		// Construct a NullModelNode using this resource
		return std::make_shared<NullModelNode>(model);
	}

  	// Required function, not implemented.
	IModelPtr loadModelFromPath(const std::string& name) override
	{
		NullModelPtr model(new NullModel);
		model->setModelPath(name);
		return model;
	}
};

} // namespace model
