#include "Documentation.h"
#include "registry/registry.h"

#include <wx/utils.h>

namespace ui
{

namespace
{
	const char* const RKEY_USER_GUIDE_URL = "user/ui/userGuideUrl";
}

void Documentation::showUserGuide(const cmd::ArgumentList&)
{
    wxLaunchDefaultBrowser(registry::getValue<std::string>(RKEY_USER_GUIDE_URL));
}

}
