#include "MD5ModelLoader.h"

#include "idatastream.h"
#include "iarchive.h"
#include "imodule.h"
#include "ishaders.h"
#include "imodelcache.h"
#include "ifilesystem.h"
#include "stream/BinaryToTextInputStream.h"
#include "os/path.h"

#include "MD5ModelNode.h"

namespace md5
 {

namespace
{
	// name may be absolute or relative
	inline std::string rootPath(const std::string& name)
	{
		return GlobalFileSystem().findRoot(
			path_is_absolute(name.c_str()) ? name : GlobalFileSystem().findFile(name)
		);
	}
} // namespace

const std::string& MD5ModelLoader::getExtension() const
{
	static std::string _ext("MD5MESH");
	return _ext;
}

scene::INodePtr MD5ModelLoader::loadModel(const std::string& modelName)
{
	// Initialise the paths, this is all needed for realisation
	std::string path = rootPath(modelName);
	std::string name = os::getRelativePath(modelName, path);

	// greebo: Path is empty for models in PK4 files, don't check for that

	// Try to load the model from the given VFS path
	model::IModelPtr model = GlobalModelCache().getModel(name);

	if (model == NULL)
	{
		rError() << "MD5ModelLoader: Could not load model << " << modelName << std::endl;
		return scene::INodePtr();
	}

	// The cached model should be an MD5Model, otherwise we're in the wrong movie
	MD5ModelPtr md5Model = std::dynamic_pointer_cast<MD5Model>(model);

	if (md5Model != NULL)
	{
		// Load was successful, construct a modelnode using this resource
		return MD5ModelNodePtr(new MD5ModelNode(md5Model));
	}
	else
	{
		rError() << "MD5ModelLoader: Cached model is not an MD5Model?" << std::endl;
	}

	return scene::INodePtr();
}

model::IModelPtr MD5ModelLoader::loadModelFromPath(const std::string& name)
{
	// Open an ArchiveFile to load
	ArchiveFilePtr file = GlobalFileSystem().openFile(name);

	if (file != NULL)
	{
		// Construct a new MD5Model container
		MD5ModelPtr model(new MD5Model);

		// Store the VFS path in this model
		model->setModelPath(name);
		// Set the filename this model was loaded from
		model->setFilename(os::getFilename(file->getName()));

		// greebo: Get the Inputstream from the given file
		stream::BinaryToTextInputStream<InputStream> inputStream(file->getInputStream());

		// Construct a Tokeniser object and start reading the file
		try
		{
			std::istream is(&inputStream);
			parser::BasicDefTokeniser<std::istream> tokeniser(is);

			// Invoke the parser routine (might throw)
			model->parseFromTokens(tokeniser);
		}
		catch (parser::ParseException& e)
		{
			rError() << "[md5model] Parse failure. Exception was:" << std::endl
								<< e.what() << std::endl;
			// Return an empty model on error
			return model::IModelPtr();
		}

		// Load was successful, return the model
		return model;
	}
	else
	{
		rError() << "Failed to load model " << name << std::endl;
		return model::IModelPtr(); // delete the model
	}
}

} // namespace md5
