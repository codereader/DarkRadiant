#pragma once

#include "ifonts.h"
#include "iimage.h"

#include "string/string.h"
#include <map>

#include "FontInfo.h"
#include "ThreadedDefLoader.h"

namespace fonts
{

class FontManager :
	public IFontManager
{
private:
	typedef std::map<std::string, FontInfoPtr> FontMap;
	FontMap _fonts;

    util::ThreadedDefLoader<void> _loader;

	std::string _curLanguage;

public:
	FontManager();

	// RegisterableModule implementation
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const ApplicationContext& ctx) override;
    void shutdownModule() override;

	// Returns the info structure of a specific font (current language),
	// returns NULL if no font info is available yet
	IFontInfoPtr findFontInfo(const std::string& name) override;

	// Returns the info structure of a specific font (current language),
	// always returns non-NULL, non-existent fonts are created on the fly
	FontInfoPtr findOrCreateFontInfo(const std::string& name);

	// Returns the current language (e.g. "english")
	const std::string& getCurLanguage();

private:
    void ensureFontsLoaded();

    void loadFonts();
	void reloadFonts();
};
typedef std::shared_ptr<FontManager> FontManagerPtr;

} // namespace fonts
