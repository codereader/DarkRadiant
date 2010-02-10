#ifndef ifonts_h__
#define ifonts_h__

#include "imodule.h"

namespace fonts
{

class IFontManager :
	public RegisterableModule
{
public:
	// TODO: Public methods
};

}

const std::string MODULE_FONTMANAGER("FontManager");

inline fonts::IFontManager& GlobalFontManager()
{
	// Cache the reference locally
	static fonts::IFontManager& _fontManager(
		*boost::static_pointer_cast<fonts::IFontManager>(
			module::GlobalModuleRegistry().getModule(MODULE_FONTMANAGER)
		)
	);
	return _fontManager;
}

#endif // ifonts_h__
