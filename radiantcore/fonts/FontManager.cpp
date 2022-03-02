#include "FontManager.h"

#include "ifilesystem.h"
#include "ishaders.h"
#include "itextstream.h"
#include "iregistry.h"
#include "igame.h"
#include "os/path.h"
#include "module/StaticModule.h"

#include "xmlutil/MissingXMLNodeException.h"

#include "FontLoader.h"

namespace fonts
{

namespace
{
	const char* MISSING_BASEPATH_NODE =
		"Failed to find \"/game/filesystem/fonts/basepath\" node \
in game descriptor";

	const char* MISSING_EXTENSION_NODE =
		"Failed to find \"/game/filesystem/fonts/extension\" node \
in game descriptor";
}

FontManager::FontManager() :
	_curLanguage("english")
{}

const std::string& FontManager::getName() const
{
	static std::string _name(MODULE_FONTMANAGER);
	return _name;
}

const StringSet& FontManager::getDependencies() const
{
    static StringSet _dependencies
    {
        MODULE_VIRTUALFILESYSTEM,
        MODULE_XMLREGISTRY,
        MODULE_GAMEMANAGER,
        MODULE_SHADERSYSTEM,
    };

	return _dependencies;
}

void FontManager::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called" << std::endl;

    _loader = std::make_unique<FontLoader>(getFontPath(), getFontExtension(), *this);

	// Find installed fonts in a new thread
    _loader->start();
}

void FontManager::shutdownModule()
{
    _loader->reset();
    _fonts.clear();
}

const std::string& FontManager::getCurLanguage()
{
	return _curLanguage;
}

void FontManager::ensureFontsLoaded()
{
    _loader->ensureFinished();
}

std::size_t FontManager::getNumFonts()
{
    return _fonts.size();
}

std::string FontManager::getFontPath()
{
    auto nlBasePath = GlobalGameManager().currentGame()->getLocalXPath("/filesystem/fonts/basepath");

    if (nlBasePath.empty())
    {
        throw xml::MissingXMLNodeException(MISSING_BASEPATH_NODE);
    }

    // Load the DAT files from the VFS
    return os::standardPathWithSlash(nlBasePath[0].getContent()) + _curLanguage + "/";
}

std::string FontManager::getFontExtension()
{
    auto nlExt = GlobalGameManager().currentGame()->getLocalXPath("/filesystem/fonts/extension");

    if (nlExt.empty())
    {
        throw xml::MissingXMLNodeException(MISSING_EXTENSION_NODE);
    }

    return nlExt[0].getContent();
}

void FontManager::reloadFonts()
{
    _fonts.clear();

    _loader->reset();
    _loader->start();
}

IFontInfoPtr FontManager::findFontInfo(const std::string& name)
{
    ensureFontsLoaded();

	FontMap::const_iterator found = _fonts.find(name);

	return found != _fonts.end() ? found->second : FontInfoPtr();
}

FontInfoPtr FontManager::findOrCreateFontInfo(const std::string& name)
{
	FontMap::iterator i = _fonts.find(name);

	if (i == _fonts.end())
	{
		FontInfoPtr font(new FontInfo(name, _curLanguage));

		// Doesn't exist yet, create now
		std::pair<FontMap::iterator, bool> result = _fonts.insert(
			FontMap::value_type(name, font)
		);

		i = result.first;
	}

	return i->second;
}

// Static module instance
module::StaticModuleRegistration<FontManager> fontManagerModule;

} // namespace fonts
