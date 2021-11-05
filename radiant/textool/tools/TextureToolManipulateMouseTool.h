#pragma once

#include "selection/ManipulateMouseTool.h"
#include "registry/CachedKey.h"

namespace ui
{

/**
 * Specialised manipulation operations for the texture tool
 */
class TextureToolManipulateMouseTool :
    public ManipulateMouseTool
{
private:
    registry::CachedKey<bool> _gridEnabled;

public:
    TextureToolManipulateMouseTool();

    const std::string& getName() override;
    const std::string& getDisplayName() override;

protected:
    selection::IManipulator::Ptr getActiveManipulator() override;

    void onManipulationStart() override;
    void onManipulationChanged() override;
    void onManipulationCancelled() override;
	void onManipulationFinished() override;

    bool manipulationIsPossible() override;
    Matrix4 getPivot2World() override;
    bool gridIsEnabled() override;
};

}
