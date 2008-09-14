#include "PicoModelLoader.h"

#include "ifilesystem.h"
#include "ifiletypes.h"
#include "iarchive.h"
#include "imodelcache.h"

#include "picomodel.h"

#include "stream/textstream.h"
#include "os/path.h"

#include "PicoModelNode.h"

#include "idatastream.h"
#include <boost/algorithm/string/case_conv.hpp>

namespace model {

namespace {
	// name may be absolute or relative
	inline std::string rootPath(const std::string& name) {
		return GlobalFileSystem().findRoot(
			path_is_absolute(name.c_str()) ? name : GlobalFileSystem().findFile(name)
		);
	}

	size_t picoInputStreamReam(void* inputStream, unsigned char* buffer, size_t length) {
		return reinterpret_cast<InputStream*>(inputStream)->read(buffer, length);
	}
} // namespace

PicoModelLoader::PicoModelLoader(const picoModule_t* module, const std::string& extension) : 
	_module(module),
	_extension(extension),
	_moduleName("ModelLoader" + extension) // e.g. ModelLoaderASE
{}

// Returns a new ModelNode for the given model name
scene::INodePtr PicoModelLoader::loadModel(const std::string& modelName) {
	// Initialise the paths, this is all needed for realisation
	std::string path = rootPath(modelName);
	std::string name = os::getRelativePath(modelName, path);

	/* greebo: Path is empty for models in PK4 files, don't give up on this

	if (path.empty()) {
		// Empty path => empty model
		return scene::INodePtr();
	}*/

	// Try to load the model from the given VFS path
	IModelPtr model = GlobalModelCache().getModel(name);

	if (model == NULL) {
		globalErrorStream() << "PicoModelLoader: Could not load model << " << modelName.c_str() << "\n";
		return scene::INodePtr();
	}

	// The cached model should be an MD5Model, otherwise we're in the wrong movie
	RenderablePicoModelPtr picoModel = 
		boost::dynamic_pointer_cast<RenderablePicoModel>(model);

	if (picoModel != NULL) {
		// Load was successful, construct a modelnode using this resource
		PicoModelNodePtr modelNode(new PicoModelNode(picoModel));
		modelNode->setSelf(modelNode);

		return modelNode;
	}
	else {
		globalErrorStream() << "PicoModelLoader: Cached model is not a PicoModel?\n";
	}

	return scene::INodePtr();
}

// Load the given model from the VFS path
IModelPtr PicoModelLoader::loadModelFromPath(const std::string& name) {
	// Open an ArchiveFile to load
	ArchiveFilePtr file = GlobalFileSystem().openFile(name);

	if (file == NULL) {
		globalErrorStream() << "Failed to load model " << name.c_str() << "\n";
		return IModelPtr();
	}

	// Determine the file extension (ASE or LWO) to pass down to the PicoModel
	std::string fName = file->getName();
	boost::algorithm::to_lower(fName);
	std::string fExt = fName.substr(fName.size() - 3, 3);

	picoModel_t* model = PicoModuleLoadModelStream(
		_module, 
		&file->getInputStream(), 
		picoInputStreamReam, 
		file->size(), 
		0
	);
	
	// greebo: Check if the model load was successful
	if (model == NULL || model->numSurfaces == 0) {
		// Model is either NULL or has no surfaces, this must've failed
		return IModelPtr();
	}

	RenderablePicoModelPtr modelObj(
		new RenderablePicoModel(model, fExt)
	);
	// Set the filename
	modelObj->setFilename(os::getFilename(file->getName()));
	modelObj->setModelPath(name);
	
	PicoFreeModel(model);
	return modelObj;
}

// RegisterableModule implementation
const std::string& PicoModelLoader::getName() const {
	return _moduleName; // e.g. ModelLoaderASE
}

const StringSet& PicoModelLoader::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
		_dependencies.insert(MODULE_SHADERCACHE);
		_dependencies.insert(MODULE_FILETYPES);
	}

	return _dependencies;
}

void PicoModelLoader::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "PicoModelLoader: " << getName().c_str() << " initialised.\n"; 
	std::string filter("*." + boost::to_lower_copy(_extension));
	
	// Register the model file extension in the FileTypRegistry
	GlobalFiletypes().addType(
		"model", getName(), 
    	FileTypePattern(_module->displayName, filter.c_str())
    );
}

} // namespace model
