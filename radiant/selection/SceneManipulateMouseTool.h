#pragma once

#include "selection/ManipulateMouseTool.h"

namespace ui
{

/**
 * Specialised manipulation operations targeting the selection as
 * currently active in the global selection system.
 */
class SceneManipulateMouseTool :
    public ManipulateMouseTool
{
public:
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
private:
    bool nothingSelected() const;
};

}
