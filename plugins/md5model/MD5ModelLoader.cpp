#include "MD5ModelLoader.h"

#include "imodule.h"
#include "ifilesystem.h"
#include "ifiletypes.h"
#include "archivelib.h"
#include "stream/textstream.h"

#include "MD5ModelNode.h"

namespace md5 {

scene::INodePtr MD5ModelLoader::loadModel(ArchiveFile& file) {
	// greebo: Get the Inputstream from the given file
	BinaryToTextInputStream<InputStream> inputStream(file.getInputStream());
	
	// Construct a new Node
	MD5ModelNodePtr modelNode(new MD5ModelNode);
	
	// Construct a Tokeniser object and start reading the file
	try {
		std::istream is(&inputStream);
		parser::BasicDefTokeniser<std::istream> tokeniser(is);
				
		// Invoke the parser routine (might throw)
		MD5Model& md5model(modelNode->model());
		md5model.parseFromTokens(tokeniser);
	}
	catch (parser::ParseException e) {
		globalErrorStream() << "[md5model] Parse failure. Exception was:\n"
							<< e.what() << "\n";		
	}
	
	// Upcast the MD5ModelNode to scene::INode and return
	return modelNode;
}

model::IModelPtr MD5ModelLoader::loadModelFromPath(const std::string& name) {
	return model::IModelPtr();
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
