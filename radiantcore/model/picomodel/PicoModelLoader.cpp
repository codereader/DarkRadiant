#include "PicoModelLoader.h"

#include "ifilesystem.h"
#include "iarchive.h"
#include "imodelcache.h"

#include "lib/picomodel.h"

#include "os/path.h"

#include "idatastream.h"
#include "string/case_conv.h"
#include "StaticModel.h"

namespace model {

namespace
{
	size_t picoInputStreamReam(void* inputStream, unsigned char* buffer, size_t length) {
		return reinterpret_cast<InputStream*>(inputStream)->read(buffer, length);
	}
} // namespace

PicoModelLoader::PicoModelLoader(const picoModule_t* module, const std::string& extension) :
    ModelImporterBase(extension),
	_module(module)
{}

// Load the given model from the VFS path
IModelPtr PicoModelLoader::loadModelFromPath(const std::string& path)
{
	// Open an ArchiveFile to load
	auto file = path_is_absolute(path.c_str()) ?
        GlobalFileSystem().openFileInAbsolutePath(path) :
        GlobalFileSystem().openFile(path);

	if (!file)
	{
		rError() << "Failed to load model " << path << std::endl;
		return IModelPtr();
	}

	// Determine the file extension (ASE or LWO) to pass down to the PicoModel
	std::string fName = file->getName();
	string::to_lower(fName);
	std::string fExt = fName.substr(fName.size() - 3, 3);

	picoModel_t* model = PicoModuleLoadModelStream(
		_module,
		&file->getInputStream(),
		picoInputStreamReam,
		file->size(),
		0
	);

	// greebo: Check if the model load was successful
	if (!model || model->numSurfaces == 0)
	{
		// Model is either NULL or has no surfaces, this must've failed
		return IModelPtr();
	}

	auto modelObj = std::make_shared<StaticModel>(model, fExt);

	// Set the filename
	modelObj->setFilename(os::getFilename(file->getName()));
	modelObj->setModelPath(path);

	PicoFreeModel(model);

	return modelObj;
}

} // namespace model
