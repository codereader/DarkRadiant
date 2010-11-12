#ifndef FontManager_h__
#define FontManager_h__

#include "ifonts.h"
#include "iimage.h"

#include "string/string.h"
#include <map>
#include <boost/shared_array.hpp>

#include "FontInfo.h"

namespace fonts
{

class FontManager :
	public IFontManager
{
private:
	typedef std::map<std::string, FontInfoPtr> FontMap;
	FontMap _fonts;

	std::string _curLanguage;

public:
	FontManager();

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();

	// Returns the info structure of a specific font (current language),
	// returns NULL if no font info is available yet
	IFontInfoPtr findFontInfo(const std::string& name);

	// Returns the info structure of a specific font (current language),
	// always returns non-NULL, non-existent fonts are created on the fly
	FontInfoPtr findOrCreateFontInfo(const std::string& name);

	// Returns the current language (e.g. "english")
	const std::string& getCurLanguage();

private:
	void reloadFonts();
};
typedef boost::shared_ptr<FontManager> FontManagerPtr;

} // namespace fonts

#endif // FontManager_h__
