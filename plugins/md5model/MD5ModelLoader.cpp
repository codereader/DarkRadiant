#include "MD5ModelLoader.h"

#include "imodule.h"
#include "ishaders.h"
#include "imodelcache.h"
#include "ifilesystem.h"
#include "ifiletypes.h"
#include "archivelib.h"
#include "stream/textstream.h"
#include "os/path.h"

#include "MD5ModelNode.h"

namespace md5 {

	namespace {
		// name may be absolute or relative
		inline std::string rootPath(const std::string& name) {
			return GlobalFileSystem().findRoot(
				path_is_absolute(name.c_str()) ? name : GlobalFileSystem().findFile(name)
			);
		}
	} // namespace

scene::INodePtr MD5ModelLoader::loadModel(const std::string& modelName) {
	// Initialise the paths, this is all needed for realisation
	std::string path = rootPath(modelName);
	std::string name = os::getRelativePath(modelName, path);

	/* greebo: Path is empty for models in PK4 files, don't give up on this

	if (path.empty()) {
		// Empty path => empty model
		return scene::INodePtr();
	}*/

	// Try to load the model from the given VFS path
	model::IModelPtr model = GlobalModelCache().getModel(name);

	if (model == NULL) {
		globalErrorStream() << "MD5ModelLoader: Could not load model << " << modelName.c_str() << "\n";
		return scene::INodePtr();
	}

	// The cached model should be an MD5Model, otherwise we're in the wrong movie
	MD5ModelPtr md5Model = boost::dynamic_pointer_cast<MD5Model>(model);

	if (md5Model != NULL) {
		// Load was successful, construct a modelnode using this resource
		MD5ModelNodePtr modelNode(new MD5ModelNode(md5Model));
		modelNode->setSelf(modelNode);

		return modelNode;
	}
	else {
		globalErrorStream() << "MD5ModelLoader: Cached model is not an MD5Model?\n";
	}

	return scene::INodePtr();
}

model::IModelPtr MD5ModelLoader::loadModelFromPath(const std::string& name) {
	// Open an ArchiveFile to load
	ArchiveFilePtr file = GlobalFileSystem().openFile(name);

	if (file != NULL) {
		// Construct a new MD5Model container
		MD5ModelPtr model(new MD5Model);

		// Store the VFS path in this model
		model->setModelPath(name);
		// Set the filename this model was loaded from
		model->setFilename(os::getFilename(file->getName()));

		// greebo: Get the Inputstream from the given file
		BinaryToTextInputStream<InputStream> inputStream(file->getInputStream());

		// Construct a Tokeniser object and start reading the file
		try {
			std::istream is(&inputStream);
			parser::BasicDefTokeniser<std::istream> tokeniser(is);
					
			// Invoke the parser routine (might throw)
			model->parseFromTokens(tokeniser);
		}
		catch (parser::ParseException e) {
			globalErrorStream() << "[md5model] Parse failure. Exception was:\n"
								<< e.what() << "\n";
			// Return an empty model on error
			return model::IModelPtr();
		}
		
		// Load was successful, return the model
		return model;
	}
	else {
		globalErrorStream() << "Failed to load model " << name.c_str() << "\n";
		return model::IModelPtr(); // delete the model
	}
}

// RegisterableModule implementation
const std::string& MD5ModelLoader::getName() const {
	static std::string _name("ModelLoaderMD5MESH");
	return _name;
}

const StringSet& MD5ModelLoader::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
		_dependencies.insert(MODULE_FILETYPES);
		_dependencies.insert(MODULE_SHADERCACHE);
	}

	return _dependencies;
}

void MD5ModelLoader::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "MD5Model::initialiseModule called.\n";
	
	GlobalFiletypes().addType(
		"model", getName(),
		FileTypePattern("md5 meshes", "*.md5mesh")
	);
}

} // namespace md5
