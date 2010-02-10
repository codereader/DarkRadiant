#ifndef FontManager_h__
#define FontManager_h__

#include "ifonts.h"

namespace fonts
{

class FontManager :
	public IFontManager
{
public:
	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();
};
typedef boost::shared_ptr<FontManager> FontManagerPtr;

} // namespace fonts

#endif // FontManager_h__
