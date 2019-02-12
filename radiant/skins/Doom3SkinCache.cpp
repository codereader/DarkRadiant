#include "Doom3SkinCache.h"

#include "itextstream.h"
#include "ifilesystem.h"
#include "iarchive.h"
#include "modulesystem/StaticModule.h"

#include <iostream>

namespace skins
{

namespace
{
    // CONSTANTS
    const char* SKINS_FOLDER = "skins/";
}

Doom3SkinCache::Doom3SkinCache() :
    _defLoader(std::bind(&Doom3SkinCache::loadSkinFiles, this)),
    _nullSkin("")
{}

ModelSkin& Doom3SkinCache::capture(const std::string& name)
{
    ensureDefsLoaded();

    NamedSkinMap::iterator i = _namedSkins.find(name);

    if (i != _namedSkins.end())
        return *(i->second); // dereference shared_ptr
    else
        return _nullSkin;
}

const StringList& Doom3SkinCache::getSkinsForModel(const std::string& model) 
{
    ensureDefsLoaded();
    return _modelSkins[model];
}

const StringList& Doom3SkinCache::getAllSkins()
{
    ensureDefsLoaded();
    return _allSkins;
}

sigc::signal<void> Doom3SkinCache::signal_skinsReloaded()
{
	return _sigSkinsReloaded;
}

// Realise the skin cache
void Doom3SkinCache::ensureDefsLoaded()
{
    // The worker function contained in the def loader will 
    // fill the local structures when it's done
    _defLoader.ensureFinished();
}

void Doom3SkinCache::loadSkinFiles()
{
	rMessage() << "[skins] Loading skins." << std::endl;

	// Use a functor to traverse the skins directory, catching any parse
	// exceptions that may be thrown
	try
	{
        GlobalFileSystem().forEachFile(
            SKINS_FOLDER, "skin",
            [&] (const vfs::FileInfo& fileInfo)
            {
                // Open the .skin file and get its contents as a std::string
                ArchiveTextFilePtr file = GlobalFileSystem().openTextFile(SKINS_FOLDER + fileInfo.name);
                assert(file);

                std::istream is(&(file->getInputStream()));

                try 
                {
                    // Pass the contents back to the SkinCache module for parsing
                    parseFile(is, fileInfo.name);
                }
                catch (parser::ParseException& e)
                {
                    rError() << "[skins]: in " << fileInfo.name << ": " << e.what() << std::endl;
                }
            }
        );
	}
	catch (parser::ParseException& e)
	{
        rError() << "[skins]: " << e.what() << std::endl;
	}

    rMessage() << "[skins] Found " << _allSkins.size() << " skins." << std::endl;

	// Done loading skins
	_sigSkinsReloaded.emit();
}

// Parse the contents of a .skin file
void Doom3SkinCache::parseFile(std::istream& contents, const std::string& filename)
{
    // Construct a DefTokeniser to parse the file
	parser::BasicDefTokeniser<std::istream> tok(contents);

	// Call the parseSkin() function for each skin decl
	while (tok.hasMoreTokens())
    {
		try
        {
			// Try to parse the skin
			Doom3ModelSkinPtr modelSkin = parseSkin(tok);
			std::string skinName = modelSkin->getName();

			modelSkin->setSkinFileName(filename);

			NamedSkinMap::iterator found = _namedSkins.find(skinName);

			// Is this already defined?
			if (found != _namedSkins.end()) 
            {
                rConsole() << "[skins] in " << filename << ": skin " + skinName +
						     " previously defined in " +
							 found->second->getSkinFileName() + "!" << std::endl;
				// Don't insert the skin into the list
			}
			else
            {
				// Add the populated Doom3ModelSkin to the hashtable and the name to the
				// list of all skins
				_namedSkins.insert(NamedSkinMap::value_type(skinName, modelSkin));
				_allSkins.push_back(skinName);
			}
		}
		catch (parser::ParseException& e)
        {
            rConsole() << "[skins]: in " << filename << ": " << e.what() << std::endl;
		}
	}
}

// Parse an individual skin declaration
Doom3ModelSkinPtr Doom3SkinCache::parseSkin(parser::DefTokeniser& tok) 
{
	// [ "skin" ] <name> "{"
	//			[ "model" <modelname> ]
	//			( <sourceTex> <destTex> )*
	// "}"

	// Parse the skin name, this is either the first token or the second token
	// (preceded by "skin")
	std::string skinName = tok.nextToken();

    if (skinName == "skin")
    {
        skinName = tok.nextToken();
    }

	tok.assertNextToken("{");

	// Create the skin object
	Doom3ModelSkinPtr skin(new Doom3ModelSkin(skinName));

	// Read key/value pairs until end of decl
	std::string key = tok.nextToken();
	while (key != "}")
    {
		// Read the value
		std::string value = tok.nextToken();

		if (value == "}")
        {
            rConsole() << "[skins] Warning: '}' found where shader name expected in skin: "
					  << skinName << std::endl;
		}

		// If this is a model key, add to the model->skin map, otherwise assume
		// this is a remap declaration
		if (key == "model")
        {
			_modelSkins[value].push_back(skinName);
		}
		else
        {
			skin->addRemap(key, value);
		}

		// Get next key
		key = tok.nextToken();
	}

	return skin;
}

const std::string& Doom3SkinCache::getName() const
{
	static std::string _name(MODULE_MODELSKINCACHE);
	return _name;
}

const StringSet& Doom3SkinCache::getDependencies() const 
{
	static StringSet _dependencies;

	if (_dependencies.empty())
    {
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
	}

	return _dependencies;
}

void Doom3SkinCache::refresh()
{
	_modelSkins.clear();
	_namedSkins.clear();
	_allSkins.clear();

    // Reset loader and launch a new thread
    _defLoader.reset();
    _defLoader.start();
}

void Doom3SkinCache::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << "Doom3SkinCache::initialiseModule called" << std::endl;

    // Load the skins in a new thread
    refresh();
}

// Module instance
module::StaticModule<Doom3SkinCache> skinCacheModule;

} // namespace skins
