#pragma once

#include "isound.h"
#include "wxutil/decl/DeclarationSelector.h"
#include "SoundShaderPreview.h"

#include "wxutil/dataview/ThreadedDeclarationTreePopulator.h"
#include "wxutil/dataview/VFSTreePopulator.h"

#include "debugging/ScopedDebugTimer.h"
#include "os/path.h"

namespace ui
{

// Local class for loading sound shader definitions in a separate thread
class ThreadedSoundShaderLoader :
    public wxutil::ThreadedDeclarationTreePopulator
{
private:
    constexpr static const char* const SHADER_ICON = "icon_sound.png";

public:
    ThreadedSoundShaderLoader(const wxutil::DeclarationTreeView::Columns& columns) :
        ThreadedDeclarationTreePopulator(decl::Type::SoundShader, columns, SHADER_ICON)
    {}

    ~ThreadedSoundShaderLoader()
    {
        EnsureStopped();
    }

    void PopulateModel(const wxutil::TreeModel::Ptr& model) override
    {
        ScopedDebugTimer timer("ThreadedSoundShaderLoader::run()");

        wxutil::VFSTreePopulator populator(model);
        
        // Visit all sound shaders and collect them for later insertion
        GlobalSoundManager().forEachShader([&](const ISoundShader::Ptr& shader)
        {
            ThrowIfCancellationRequested();

            // Construct a "path" into the sound shader tree,
            // using the mod name as first folder level
            // angua: if there is a displayFolder present, put it between the mod name and the shader name
            auto displayFolder = shader->getDisplayFolder();

            // Some shaders contain backslashes, sort them in the tree by replacing the backslashes
            auto shaderNameForwardSlashes = os::standardPath(shader->getDeclName());

            auto fullPath = !displayFolder.empty() ?
                shader->getModName() + "/" + displayFolder + "/" + shaderNameForwardSlashes :
                shader->getModName() + "/" + shaderNameForwardSlashes;

            // Sort the shader into the tree and set the values
            populator.addPath(fullPath, [&](wxutil::TreeModel::Row& row, 
                const std::string& path, const std::string& leafName, bool isFolder)
            {
                AssignValuesToRow(row, path, isFolder ? path : shader->getDeclName(), leafName, isFolder);
            });
        });
    }
};

class SoundShaderSelector :
    public wxutil::DeclarationSelector
{
private:
    // The preview widget group
    SoundShaderPreview* _preview;

public:
    SoundShaderSelector(wxWindow* parent) :
        DeclarationSelector(parent, decl::Type::SoundShader),
        _preview(new SoundShaderPreview(this))
    {
        GetTreeView()->SetExpandTopLevelItemsAfterPopulation(true);

        Populate();

        AddPreviewToBottom(_preview);
    }

    void Populate() override
    {
        PopulateTreeView(std::make_shared<ThreadedSoundShaderLoader>(GetColumns()));
    }

protected:
    bool onTreeViewItemActivated() override
    {
        auto selectedItem = GetSelectedDeclName();

        // Ctrl-Double-Click plays back a random file
        if (wxGetKeyState(WXK_CONTROL))
        {
            _preview->playRandomSoundFile();
            return true;
        }

        return false; // not processed
    }
};

}
