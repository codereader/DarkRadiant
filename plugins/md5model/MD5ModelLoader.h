#pragma once

#include "imodel.h"
#include "MD5Model.h"

class ArchiveFile;

namespace md5
{

class MD5ModelLoader :
	public model::IModelImporter
{
public:
	const std::string& getExtension() const override;

	// ModelLoader implementation
	// Returns a new ModelNode for the given model name
	scene::INodePtr loadModel(const std::string& modelName) override;

	// Documentation: See imodel.h
	model::IModelPtr loadModelFromPath(const std::string& name) override;
};
typedef std::shared_ptr<MD5ModelLoader> MD5ModelLoaderPtr;

} // namespace md5
