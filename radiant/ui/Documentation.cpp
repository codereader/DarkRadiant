#include "Documentation.h"
#include "registry/registry.h"

#include <wx/utils.h>
#include "imodule.h"

namespace ui
{

namespace
{
	const char* const RKEY_USER_GUIDE_URL = "user/ui/userGuideUrl";
	const char* const RKEY_FORUM_URL = "user/ui/forumUrl";
}

void Documentation::showUserGuide(const cmd::ArgumentList&)
{
    wxLaunchDefaultBrowser(registry::getValue<std::string>(RKEY_USER_GUIDE_URL));
}

void Documentation::OpenScriptReference(const cmd::ArgumentList&)
{
    const IApplicationContext& ctx = module::GlobalModuleRegistry().getApplicationContext();
    wxLaunchDefaultBrowser(registry::getValue<std::string>(RKEY_SCRIPT_REFERENCE_URL));
}

void Documentation::showOfflineUserGuide(const cmd::ArgumentList&)
{
    const IApplicationContext& ctx = module::GlobalModuleRegistry().getApplicationContext();
    wxLaunchDefaultBrowser(ctx.getHTMLPath() + "manual.html");
}

void Documentation::OpenForumUrl(const cmd::ArgumentList&)
{
    wxLaunchDefaultBrowser(registry::getValue<std::string>(RKEY_FORUM_URL));
}

}
