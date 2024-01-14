#include "Algorithm.h"

#include "ieclass.h"
#include "icommandsystem.h"

#include "wxutil/sourceview/DeclarationSourceView.h"

namespace ui
{

namespace algorithm
{

void showEntityClassDefinition(wxWindow* parent, const std::string& entityClassName)
{
    if (auto eclass = GlobalEntityClassManager().findClass(entityClassName); eclass)
    {
        auto view = new wxutil::DeclarationSourceView(parent);
        view->setDeclaration(eclass);

        view->ShowModal();
        view->Destroy();
    }
}

void showEntityClassInTree(const std::string& entityClassName)
{
    if (auto eclass = GlobalEntityClassManager().findClass(entityClassName); eclass)
    {
        GlobalCommandSystem().executeCommand("EntityClassTree", cmd::ArgumentList{ eclass->getDeclName() });
    }
}

}

}
