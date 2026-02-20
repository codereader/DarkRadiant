#pragma once

#include "imousetool.h"
#include "math/Vector3.h"
#include <string>

namespace ui
{

/**
 * Camera mouse tool that places decal patches on brush faces.
 */
class DecalShooterTool :
    public MouseTool
{
private:
    static std::string _name;
    static std::string _displayName;

public:
    const std::string& getName() override;
    const std::string& getDisplayName() override;

    Result onMouseDown(Event& ev) override;
    Result onMouseMove(Event& ev) override;
    Result onMouseUp(Event& ev) override;

    static const std::string& NAME();

private:
    void createDecalAtFace(
        const Vector3& intersectionPoint,
        const Vector3& normal,
        double width,
        double height,
        double offset,
        double rotationDegrees,
        bool flip,
        const std::string& material
    );
};

} // namespace ui
