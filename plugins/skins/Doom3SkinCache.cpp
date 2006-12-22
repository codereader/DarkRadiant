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
	typedef const char* first_argument_type;
	
	// Constructor
	SkinLoader(Doom3SkinCache& c)
	: _cache(c)
	{}
	
	// Functor operator
	void operator() (const char* fileName) {

		// Open the .skin file and get its contents as a std::string
		ArchiveTextFile* file = 
			GlobalFileSystem().openTextFile(SKINS_FOLDER 
											+ std::string(fileName));
		assert(file);
		std::string contents = file->getString();
		file->release();
	
		// Pass the contents back to the SkinCache module for parsing
		_cache.parseFile(contents);				
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
		globalErrorStream() << e.what() << "\n";		
	} 
	
	// Set the realised flag
	_realised = true;
}

// Parse the contents of a .skin file
void Doom3SkinCache::parseFile(const std::string& contents) {
	
	// Construct a DefTokeniser to parse the file
	parser::DefTokeniser tok(contents);
	
	// Call the parseSkin() function for each skin decl
	while (tok.hasMoreTokens())
		parseSkin(tok);
}

// Parse an individual skin declaration
void Doom3SkinCache::parseSkin(parser::DefTokeniser& tok) {
	
	// "skin" <name> "{" [ "model" <modelname> ] ( <sourceTex> <destTex> )* "}"

	tok.assertNextToken("skin");
	
	// Parse the skin name
	std::string skinName = tok.nextToken();
	
	tok.assertNextToken("{");

	// Create the skin object
	boost::shared_ptr<Doom3ModelSkin> skin(new Doom3ModelSkin());
	
	// Read key/value pairs until end of decl
	std::string key = tok.nextToken();
	std::string model = "";
	while (key != "}") {
		
		// Read the value
		std::string value = tok.nextToken();
		
		// If this is a model key, set it on the object otherwise add as a 
		// remap
		if (key == "model") {
			model = value;
			skin->setModel(value);
		}
		else {
			skin->addRemap(key, value);
		}
			
		// Get next key
		key = tok.nextToken();
	}

	// Add the populated Doom3ModelSkin to the hashtable and the name to the
	// list of all skins
	_namedSkins[skinName] = skin;
	_allSkins.push_back(skinName);
	
	// If we have a model key, append this skin name to the list of skins for
	// the model
	if (!model.empty())
		_modelSkins[model].push_back(skinName);
}
	
} // namespace skins
