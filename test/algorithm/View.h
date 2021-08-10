#pragma once

#include "render/View.h"
#include "registry/registry.h"
#include "selection/SelectionVolume.h"

namespace test
{

namespace algorithm
{

constexpr std::size_t DeviceWidth = 640;
constexpr std::size_t DeviceHeight = 640;
constexpr const char* const RKEY_SELECT_EPSILON = "user/ui/selectionEpsilon";

inline void constructCenteredOrthoview(render::View& view, const Vector3& origin)
{
    // Move the orthoview exactly to the center of this object
    double scale = 1.0;

    Matrix4 projection;

    projection[0] = 1.0 / static_cast<double>(DeviceWidth / 2);
    projection[5] = 1.0 / static_cast<double>(DeviceHeight / 2);
    projection[10] = 1.0 / (32768 * scale);

    projection[12] = 0.0;
    projection[13] = 0.0;
    projection[14] = -1.0;

    projection[1] = projection[2] = projection[3] =
        projection[4] = projection[6] = projection[7] =
        projection[8] = projection[9] = projection[11] = 0.0;

    projection[15] = 1.0f;

    // Modelview
    Matrix4 modelView;

    // Translate the view to the center of the brush
    modelView[12] = -origin.x() * scale;
    modelView[13] = -origin.y() * scale;
    modelView[14] = 32768 * scale;

    // axis base
    modelView[0] = scale;
    modelView[1] = 0;
    modelView[2] = 0;

    modelView[4] = 0;
    modelView[5] = scale;
    modelView[6] = 0;

    modelView[8] = 0;
    modelView[9] = 0;
    modelView[10] = -scale;

    modelView[3] = modelView[7] = modelView[11] = 0;
    modelView[15] = 1;

    view.construct(projection, modelView, DeviceWidth, DeviceHeight);
}

inline SelectionVolume constructOrthoviewSelectionTest(const render::View& orthoView)
{
    render::View scissored(orthoView);

    auto epsilon = registry::getValue<float>(RKEY_SELECT_EPSILON);
    Vector2 deviceEpsilon(epsilon / DeviceWidth, epsilon / DeviceHeight);
    ConstructSelectionTest(scissored, selection::Rectangle::ConstructFromPoint(Vector2(0, 0), deviceEpsilon));

    return SelectionVolume(scissored);
}

}

}