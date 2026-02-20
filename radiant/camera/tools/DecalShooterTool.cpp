#include "DecalShooterTool.h"

#include "i18n.h"
#include "iundo.h"
#include "imap.h"
#include "ipatch.h"
#include "iscenegraph.h"
#include "iselection.h"

#include "scenelib.h"
#include "CameraMouseToolEvent.h"
#include "FaceIntersectionFinder.h"
#include "ui/decalshooter/DecalShooterPanel.h"

#include <cmath>
#include <random>

namespace ui
{

// Static members
std::string DecalShooterTool::_name = "DecalShooterTool";
std::string DecalShooterTool::_displayName;

const std::string& DecalShooterTool::NAME()
{
    return _name;
}

const std::string& DecalShooterTool::getName()
{
    return _name;
}

const std::string& DecalShooterTool::getDisplayName()
{
    if (_displayName.empty())
    {
        _displayName = _("Place Decal");
    }
    return _displayName;
}

MouseTool::Result DecalShooterTool::onMouseDown(Event& ev)
{
    try
    {
        CameraMouseToolEvent& camEvent = dynamic_cast<CameraMouseToolEvent&>(ev);

        SelectionTestPtr selectionTest = camEvent.getView().createSelectionTestForPoint(
            camEvent.getDevicePosition()
        );

        const Matrix4& viewProjection = selectionTest->getVolume().GetViewProjection();

        FaceIntersectionFinder finder(*selectionTest, viewProjection);
        GlobalSceneGraph().root()->traverse(finder);

        const FaceIntersection& intersection = finder.getResult();

        if (intersection.valid)
        {
            DecalShooterPanel* panel = DecalShooterPanel::getInstance();

            double width = panel ? panel->getDecalWidth() : 128.0;
            double height = panel ? panel->getDecalHeight() : 128.0;
            double offset = panel ? panel->getDecalOffset() : 0.125;

            bool randomRotation = panel ? panel->isRandomRotationEnabled() : false;
            double rotation = 0.0;
            if (randomRotation)
            {
                static std::random_device rd;
                static std::mt19937 gen(rd());
                static std::uniform_real_distribution<double> dist(-180.0, 180.0);
                rotation = dist(gen);
            }
            else
            {
                rotation = panel ? panel->getDecalRotation() : 0.0;
            }

            bool flip = panel ? panel->isFlipEnabled() : false;
            std::string material = panel ? panel->getDecalMaterial() : "textures/common/decal";

            createDecalAtFace(
                intersection.point,
                intersection.normal,
                width,
                height,
                offset,
                rotation,
                flip,
                material
            );
        }

        return Result::Finished;
    }
    catch (std::bad_cast&)
    {
    }

    return Result::Ignored;
}

MouseTool::Result DecalShooterTool::onMouseMove(Event& ev)
{
    return Result::Ignored;
}

MouseTool::Result DecalShooterTool::onMouseUp(Event& ev)
{
    return Result::Finished;
}

void DecalShooterTool::createDecalAtFace(
    const Vector3& intersectionPoint,
    const Vector3& normal,
    double width,
    double height,
    double offset,
    double rotationDegrees,
    bool flip,
    const std::string& material)
{
    UndoableCommand cmd("PlaceDecal");

    scene::INodePtr patchNode = GlobalPatchModule().createPatch(patch::PatchDefType::Def3);

    if (!patchNode)
    {
        return;
    }

    IPatchNodePtr patchNodePtr = std::dynamic_pointer_cast<IPatchNode>(patchNode);
    if (!patchNodePtr)
    {
        return;
    }

    IPatch& patch = patchNodePtr->getPatch();

    patch.setDims(3, 3);
    patch.setFixedSubdivisions(true, Subdivisions(1, 1));

    // Build orthonormal basis for the decal plane
    Vector3 up(0, 0, 1);
    if (std::abs(normal.dot(up)) > 0.9)
    {
        up = Vector3(0, 1, 0);
    }

    Vector3 tangent = normal.cross(up).getNormalised();
    Vector3 bitangent = tangent.cross(normal).getNormalised();

    if (std::abs(rotationDegrees) > 0.001)
    {
        double radians = rotationDegrees * (M_PI / 180.0);
        double cosAngle = std::cos(radians);
        double sinAngle = std::sin(radians);

        Vector3 rotatedTangent = tangent * cosAngle + bitangent * sinAngle;
        Vector3 rotatedBitangent = bitangent * cosAngle - tangent * sinAngle;

        tangent = rotatedTangent;
        bitangent = rotatedBitangent;
    }

    if (flip)
    {
        bitangent = -bitangent;
    }

    Vector3 center = intersectionPoint + normal * offset;

    double halfWidth = width * 0.5;
    double halfHeight = height * 0.5;

    Vector3 points[4];
    points[0] = center - tangent * halfWidth + bitangent * halfHeight;
    points[1] = center + tangent * halfWidth + bitangent * halfHeight;
    points[2] = center + tangent * halfWidth - bitangent * halfHeight;
    points[3] = center - tangent * halfWidth - bitangent * halfHeight;

    // Set up 3x3 control point grid
    patch.ctrlAt(0, 0).vertex = points[0];
    patch.ctrlAt(2, 0).vertex = points[1];
    patch.ctrlAt(1, 0).vertex = (patch.ctrlAt(0, 0).vertex + patch.ctrlAt(2, 0).vertex) / 2;

    patch.ctrlAt(0, 1).vertex = (points[0] + points[3]) / 2;
    patch.ctrlAt(2, 1).vertex = (points[1] + points[2]) / 2;
    patch.ctrlAt(1, 1).vertex = (patch.ctrlAt(0, 1).vertex + patch.ctrlAt(2, 1).vertex) / 2;

    patch.ctrlAt(0, 2).vertex = points[3];
    patch.ctrlAt(2, 2).vertex = points[2];
    patch.ctrlAt(1, 2).vertex = (patch.ctrlAt(0, 2).vertex + patch.ctrlAt(2, 2).vertex) / 2;

    patch.setShader(material);
    patch.fitTexture(1, 1);
    patch.controlPointsChanged();

    scene::INodePtr worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    if (worldspawn)
    {
        worldspawn->addChildNode(patchNode);
    }

    DecalShooterPanel* panel = DecalShooterPanel::getInstance();
    if (panel)
    {
        panel->onDecalCreated(patchNode);
    }

    GlobalSelectionSystem().setSelectedAll(false);
    Node_setSelected(patchNode, true);
}

} // namespace ui
