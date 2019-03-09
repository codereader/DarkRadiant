#include "Documentation.h"
#include "modulesystem/ModuleRegistry.h"

#include <wx/utils.h>

namespace ui
{

void Documentation::showUserGuide(const cmd::ArgumentList&)
{
    const ApplicationContext& ctx = module::ModuleRegistry::Instance().getApplicationContext();
    wxLaunchDefaultBrowser(ctx.getHTMLPath() + "manual.html");
}

}
