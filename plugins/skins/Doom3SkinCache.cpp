#include "Doom3SkinCache.h"

#include "itextstream.h"
#include "ifilesystem.h"
#include "iarchive.h"
#include "generic/callback.h"

#include <iostream>

namespace skins {
	
namespace {
	
/* CONSTANTS */
const char* SKINS_FOLDER = "skins/";
	
/**
 * Filesystem traversal functor to load .skin files from the skins/ directory.
 * The functor opens each file and passes the contents (as a string) back to
 * the Doom3SkinCache module for parsing.
 */
class SkinLoader
{
	// Doom3SkinCache to parse files
	Doom3SkinCache& _cache;
	
public:

	// Required typedef
	typedef const std::string& first_argument_type;
	
	// Constructor
	SkinLoader(Doom3SkinCache& c)
	: _cache(c)
	{}
	
	// Functor operator
	void operator() (const std::string& fileName) {

		// Open the .skin file and get its contents as a std::string
		ArchiveTextFilePtr file = 
			GlobalFileSystem().openTextFile(SKINS_FOLDER + fileName);
		assert(file);
		std::istream is(&(file->getInputStream()));
	
		try {
			// Pass the contents back to the SkinCache module for parsing
			_cache.parseFile(is, fileName);
		}
		catch (parser::ParseException e) {
			std::cout << "[skins]: in " << fileName << ": " << e.what() << "\n";
		}
	}
}; 

} // blank namespace
	
// Realise the skin cache
void Doom3SkinCache::realise() {

	// Return if already realised
	if (_realised)
		return;
		
	globalOutputStream() << "[skins] Loading skins.\n"; 

	// Use a functor to traverse the skins directory, catching any parse
	// exceptions that may be thrown
	try {
		SkinLoader ldr(*this);
		GlobalFileSystem().forEachFile(SKINS_FOLDER, 
									   "skin",
									   makeCallback1(ldr));
	}
	catch (parser::ParseException e) {
		std::cout << "[skins]: " << e.what() << "\n";
	}
	
	// Set the realised flag
	_realised = true;
}

// Parse the contents of a .skin file
void Doom3SkinCache::parseFile(std::istream& contents, const std::string& filename) {
	
	// Construct a DefTokeniser to parse the file
	parser::BasicDefTokeniser<std::istream> tok(contents);
	
	// Call the parseSkin() function for each skin decl
	while (tok.hasMoreTokens()) {
		try {
			// Try to parse the skin
			Doom3ModelSkinPtr modelSkin = parseSkin(tok);
			std::string skinName = modelSkin->getName();
			
			modelSkin->setSkinFileName(filename);
			
			NamedSkinMap::iterator found = _namedSkins.find(skinName);
			
			// Is this already defined?
			if (found != _namedSkins.end()) {
				std::cout << "[skins] in " << filename << ": skin " + skinName + 
						     " previously defined in " +
							 found->second->getSkinFileName() + "!\n";
				// Don't insert the skin into the list
			}
			else {
				// Add the populated Doom3ModelSkin to the hashtable and the name to the
				// list of all skins
				_namedSkins.insert(NamedSkinMap::value_type(skinName, modelSkin));
				_allSkins.push_back(skinName);
			}
		}
		catch (parser::ParseException e) {
			std::cout << "[skins]: in " << filename << ": " << e.what() << "\n";
		}
	}
}

// Parse an individual skin declaration
Doom3ModelSkinPtr Doom3SkinCache::parseSkin(parser::DefTokeniser& tok) {
	
	// [ "skin" ] <name> "{" 
	//			[ "model" <modelname> ] 
	//			( <sourceTex> <destTex> )* 
	// "}"

	// Parse the skin name, this is either the first token or the second token
	// (preceded by "skin")
	std::string skinName = tok.nextToken();
	if (skinName == "skin")
		skinName = tok.nextToken();
	
	tok.assertNextToken("{");

	// Create the skin object
	Doom3ModelSkinPtr skin(new Doom3ModelSkin(skinName));
	
	// Read key/value pairs until end of decl
	std::string key = tok.nextToken();
	while (key != "}") {
		
		// Read the value
		std::string value = tok.nextToken();
		
		if (value == "}") {
			std::cout << "[skins] Warning: '}' found where shader name expected in skin: " 
					  << skinName << "\n";
		}
		
		// If this is a model key, add to the model->skin map, otherwise assume
		// this is a remap declaration
		if (key == "model") {
			_modelSkins[value].push_back(skinName);		
		}
		else {
			skin->addRemap(key, value);
		}
			
		// Get next key
		key = tok.nextToken();
	}
	
	return skin;
}

const std::string& Doom3SkinCache::getName() const {
	static std::string _name(MODULE_MODELSKINCACHE);
	return _name;
}

const StringSet& Doom3SkinCache::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
	}

	return _dependencies;
}

void Doom3SkinCache::refresh() {
	_modelSkins.clear();
	_namedSkins.clear();
	_allSkins.clear();

	_realised = false;
}

void Doom3SkinCache::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "Doom3SkinCache::initialiseModule called\n";
	
	realise();
}

} // namespace skins
