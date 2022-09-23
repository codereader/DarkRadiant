#include "FxChooser.h"

#include "debugging/ScopedDebugTimer.h"
#include "os/path.h"
#include "wxutil/dataview/ThreadedDeclarationTreePopulator.h"
#include "wxutil/dataview/VFSTreePopulator.h"
#include "wxutil/decl/DeclarationSelector.h"

namespace ui
{

// Local class for loading FX declarations in a separate thread
class ThreadedFxLoader :
    public wxutil::ThreadedDeclarationTreePopulator
{
private:
    constexpr static const char* const FX_ICON = "icon_fx.png";

public:
    ThreadedFxLoader(const wxutil::DeclarationTreeView::Columns& columns) :
        ThreadedDeclarationTreePopulator(decl::Type::Fx, columns, FX_ICON)
    {}

    ~ThreadedFxLoader()
    {
        EnsureStopped();
    }

    void PopulateModel(const wxutil::TreeModel::Ptr& model) override
    {
        ScopedDebugTimer timer("ThreadedFxLoader::run()");

        wxutil::VFSTreePopulator populator(model);
        
        // Visit all sound shaders and collect them for later insertion
        GlobalDeclarationManager().foreachDeclaration(decl::Type::Fx, [&](const decl::IDeclaration::Ptr& decl)
        {
            ThrowIfCancellationRequested();

            // Some shaders contain backslashes, sort them in the tree by replacing the backslashes
            auto shaderNameForwardSlashes = os::standardPath(decl->getDeclName());

            auto fullPath = decl->getModName() + "/" + shaderNameForwardSlashes;

            // Sort the shader into the tree and set the values
            populator.addPath(fullPath, [&](wxutil::TreeModel::Row& row, 
                const std::string& path, const std::string& leafName, bool isFolder)
            {
                AssignValuesToRow(row, path, isFolder ? path : decl->getDeclName(), leafName, isFolder);
            });
        });
    }
};

class FxSelector :
    public wxutil::DeclarationSelector
{
public:
    FxSelector(wxWindow* parent) :
        DeclarationSelector(parent, decl::Type::Fx)
    {
        Populate();
    }

    void Populate() override
    {
        PopulateTreeView(std::make_shared<ThreadedFxLoader>(GetColumns()));
    }
};

FxChooser::FxChooser(wxWindow* parent) :
    DeclarationSelectorDialog(decl::Type::SoundShader, _("Choose FX Declaration"), "FxChooser", parent)
{
    SetSelector(new FxSelector(this));
}

std::string FxChooser::ChooseDeclaration(const std::string& preselected)
{
    FxChooser instance;

    if (!preselected.empty())
    {
        instance.SetSelectedDeclName(preselected);
    }

    if (instance.ShowModal() == wxID_OK)
    {
        return instance.GetSelectedDeclName();
    }

    return ""; // Empty selection on cancel
}

}
