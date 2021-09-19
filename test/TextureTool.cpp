#include "RadiantTest.h"

#include <set>
#include <sigc++/connection.h>
#include "imap.h"
#include "ipatch.h"
#include "iselectable.h"
#include "itexturetoolmodel.h"
#include "icommandsystem.h"
#include "iselection.h"
#include "scenelib.h"
#include "algorithm/Primitives.h"
#include "render/TextureToolView.h"
#include "selection/SelectionVolume.h"
#include "Rectangle.h"

namespace test
{

using TextureToolTest = RadiantTest;

inline std::size_t getTextureToolNodeCount()
{
    std::size_t count = 0;

    GlobalTextureToolSceneGraph().foreachNode([&](const textool::INode::Ptr& node)
    {
        ++count;
        return true;
    });

    return count;
}

inline textool::INode::Ptr getFirstTextureToolNode()
{
    textool::INode::Ptr returnValue;

    GlobalTextureToolSceneGraph().foreachNode([&](const textool::INode::Ptr& node)
    {
        returnValue = node;
        return false;
    });

    return returnValue;
}

// Checks that changing the regular scene selection will have an effect on the tex tool scene
TEST_F(TextureToolTest, SceneGraphObservesSelection)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush1 = algorithm::createCubicBrush(worldspawn, Vector3(0,0,0), "textures/numbers/1");
    auto brush2 = algorithm::createCubicBrush(worldspawn, Vector3(0,256,256), "textures/numbers/1");

    // Empty tex tool scenegraph on empty scene selection
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 0) << "Non-empty selection at startup";
    EXPECT_EQ(getTextureToolNodeCount(), 0) << "There shouldn't be any textool nodes when the scene is empty";

    Node_setSelected(brush1, true);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1) << "1 Brush must be selected";

    // We don't know how many tex tool nodes there are, but it should be more than 0
    auto nodeCount = getTextureToolNodeCount();
    EXPECT_GT(nodeCount, 0) << "There should be some tex tool nodes now";

    Node_setSelected(brush2, true);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 2) << "2 Brushes must be selected";

    // Should be even more now
    auto nodeCount2 = getTextureToolNodeCount();
    EXPECT_GT(nodeCount2, nodeCount) << "There should be more tex tool nodes now";

    GlobalSelectionSystem().setSelectedAll(false);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 0) << "Non-empty selection at shutdown";
    EXPECT_EQ(getTextureToolNodeCount(), 0) << "There shouldn't be any textool nodes when the scene is empty";
}

TEST_F(TextureToolTest, SceneGraphNeedsUniqueShader)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush1 = algorithm::createCubicBrush(worldspawn, Vector3(0, 0, 0), "textures/numbers/1");
    auto brush2 = algorithm::createCubicBrush(worldspawn, Vector3(0, 256, 256), "textures/numbers/2");

    Node_setSelected(brush1, true);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1) << "1 Brush must be selected";

    // We don't know how many tex tool nodes there are, but it should be more than 0
    EXPECT_GT(getTextureToolNodeCount(), 0) << "There should be some tex tool nodes now";

    Node_setSelected(brush2, true);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 2) << "2 Brushes must be selected";

    EXPECT_EQ(getTextureToolNodeCount(), 0) << "There should be no nodes now, since the material is non unique";

    // Deselect brush 1, now only brush 2 is selected
    Node_setSelected(brush1, false);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1) << "1 Brush must be selected";
    EXPECT_GT(getTextureToolNodeCount(), 0) << "There should be some tex tool nodes again";
}

TEST_F(TextureToolTest, SceneGraphRecognisesBrushes)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush1 = algorithm::createCubicBrush(worldspawn, Vector3(0, 0, 0), "textures/numbers/1");

    Node_setSelected(brush1, true);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1) << "1 Brush must be selected";

    // We don't know how many tex tool nodes there are, but it should be more than 0
    EXPECT_GT(getTextureToolNodeCount(), 0) << "There should be some tex tool nodes now";
}

TEST_F(TextureToolTest, SceneGraphRecognisesPatches)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto patch = GlobalPatchModule().createPatch(patch::PatchDefType::Def2);
    scene::addNodeToContainer(patch, worldspawn);

    Node_setSelected(patch, true);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1) << "1 patch must be selected";

    // We don't know how many tex tool nodes there are, but it should be more than 0
    EXPECT_GT(getTextureToolNodeCount(), 0) << "There should be some tex tool nodes now";
}

TEST_F(TextureToolTest, PatchNodeBounds)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto patchNode = GlobalPatchModule().createPatch(patch::PatchDefType::Def2);

    auto patch = Node_getIPatch(patchNode);
    patch->setDims(3, 3);

    auto origin = Vector2(5.4, -78.3);
    auto step = 0.45;
    // Accumulate the test bounds
    AABB checkedBounds;

    for (auto col = 0; col < 3; ++col)
    {
        for (auto row = 0; row < 3; ++row)
        {
            auto& ctrl = patch->ctrlAt(row, col);

            ctrl.texcoord = Vector2(origin.x() + step * col, origin.y() + step * row);
            checkedBounds.includePoint({ ctrl.texcoord.x(), ctrl.texcoord.y(), 0 });
        }
    }

    scene::addNodeToContainer(patchNode, worldspawn);
    Node_setSelected(patchNode, true);

    auto node = getFirstTextureToolNode();
    EXPECT_TRUE(node) << "No texture tool node here";

    EXPECT_TRUE(math::isNear(node->localAABB().getOrigin(), checkedBounds.getOrigin(), 0.01)) << 
        "Bounds mismatch, got " << node->localAABB().getOrigin() << " instead of " << checkedBounds.getOrigin();
    EXPECT_TRUE(math::isNear(node->localAABB().getExtents(), checkedBounds.getExtents(), 0.01)) <<
        "Bounds mismatch, got " << node->localAABB().getExtents() << " instead of " << checkedBounds.getExtents();
}

TEST_F(TextureToolTest, ForeachSelectedNode)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush1 = algorithm::createCubicBrush(worldspawn, Vector3(0, 0, 0), "textures/numbers/1");
    auto brush2 = algorithm::createCubicBrush(worldspawn, Vector3(0, 256, 256), "textures/numbers/1");
    auto patchNode = GlobalPatchModule().createPatch(patch::PatchDefType::Def2);
    Node_getIPatch(patchNode)->setDims(3, 3);
    Node_getIPatch(patchNode)->setShader("textures/numbers/1");

    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);
    Node_setSelected(patchNode, true);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 3) << "3 items must be selected";

    // We don't know how many tex tool nodes there are, but it should be more than 0
    EXPECT_GT(getTextureToolNodeCount(), 0) << "There should be some tex tool nodes now";

    std::set<textool::INode::Ptr> selectedNodes;
    std::size_t i = 0;

    // Selected every odd node
    GlobalTextureToolSceneGraph().foreachNode([&](const textool::INode::Ptr& node)
    {
        if (++i % 2 == 1)
        {
            node->setSelected(true);
            selectedNodes.emplace(node);
        }

        return true;
    });

    std::size_t selectedCount = 0;
    GlobalTextureToolSelectionSystem().foreachSelectedNode([&](const textool::INode::Ptr& node)
    {
        ++selectedCount;
        EXPECT_TRUE(selectedNodes.count(node) > 0) << "Node shouldn't be selected";
        return true;
    });

    EXPECT_EQ(selectedCount, selectedNodes.size()) << "Selection count didn't match";
}

inline void foreachPatchVertex(const IPatch& patch, const std::function<void(const PatchControl&)>& functor)
{
    for (std::size_t col = 0; col < patch.getWidth(); ++col)
    {
        for (std::size_t row = 0; row < patch.getHeight(); ++row)
        {
            functor(patch.ctrlAt(row, col));
        }
    }
}

inline AABB getTextureSpaceBounds(const IPatch& patch)
{
    AABB bounds;

    foreachPatchVertex(patch, [&](const PatchControl& control)
    {
        const auto& uv = control.texcoord;
        bounds.includePoint({ uv.x(), uv.y(), 0 });
    });

    return bounds;
}

inline AABB getTextureSpaceBounds(const IFace& face)
{
    AABB bounds;

    for (const auto& vertex : face.getWinding())
    {
        bounds.includePoint({ vertex.texcoord.x(), vertex.texcoord.y(), 0 });
    }

    return bounds;
}

constexpr int TEXTOOL_WIDTH = 500;
constexpr int TEXTOOL_HEIGHT = 400;

inline scene::INodePtr setupPatchNodeForTextureTool()
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto patchNode = algorithm::createPatchFromBounds(worldspawn, AABB(Vector3(4, 50, 60), Vector3(64, 128, 256)), "textures/numbers/1");

    auto patch = Node_getIPatch(patchNode);
    patch->scaleTextureNaturally();
    patch->controlPointsChanged();

    // Select this node in the scene, to make it available in the texture tool
    Node_setSelected(patchNode, true);

    return patchNode;
}

inline textool::IFaceNode::Ptr findTexToolFaceWithNormal(const Vector3& normal)
{
    textool::IFaceNode::Ptr result;

    GlobalTextureToolSceneGraph().foreachNode([&](const textool::INode::Ptr& node)
    {
        auto faceNode = std::dynamic_pointer_cast<textool::IFaceNode>(node);

        if (faceNode && math::isNear(faceNode->getFace().getPlane3().normal(), normal, 0.01))
        {
            result = faceNode;
        }

        return result == nullptr;
    });

    return result;
}

// Default manipulator mode should be "Drag"
TEST_F(TextureToolTest, DefaultManipulatorMode)
{
    EXPECT_EQ(GlobalTextureToolSelectionSystem().getActiveManipulatorType(), selection::IManipulator::Drag);
    EXPECT_EQ(GlobalTextureToolSelectionSystem().getActiveManipulator()->getType(), selection::IManipulator::Drag);
}

TEST_F(TextureToolTest, DefaultSelectionMode)
{
    EXPECT_EQ(GlobalTextureToolSelectionSystem().getMode(), textool::SelectionMode::Surface);
}

TEST_F(TextureToolTest, ToggleManipulatorModes)
{
    // We're starting in "Drag" mode, so toggling the default mode should do nothing
    GlobalCommandSystem().executeCommand("ToggleTextureToolManipulatorMode", { "Drag" });
    EXPECT_EQ(GlobalTextureToolSelectionSystem().getActiveManipulatorType(), selection::IManipulator::Drag);

    // Toggle to Rotate
    GlobalCommandSystem().executeCommand("ToggleTextureToolManipulatorMode", { "Rotate" });
    EXPECT_EQ(GlobalTextureToolSelectionSystem().getActiveManipulatorType(), selection::IManipulator::Rotate);

    // Toggle from Rotate back to Drag
    GlobalCommandSystem().executeCommand("ToggleTextureToolManipulatorMode", { "Rotate" });
    EXPECT_EQ(GlobalTextureToolSelectionSystem().getActiveManipulatorType(), selection::IManipulator::Drag);

    // Toggle to Rotate again
    GlobalCommandSystem().executeCommand("ToggleTextureToolManipulatorMode", { "Rotate" });
    EXPECT_EQ(GlobalTextureToolSelectionSystem().getActiveManipulatorType(), selection::IManipulator::Rotate);

    // Toggle Drag explicitly
    GlobalCommandSystem().executeCommand("ToggleTextureToolManipulatorMode", { "Drag" });
    EXPECT_EQ(GlobalTextureToolSelectionSystem().getActiveManipulatorType(), selection::IManipulator::Drag);
}

TEST_F(TextureToolTest, ManipulatorModeChangedSignal)
{
    bool signalFired = false;
    selection::IManipulator::Type signalArgument;

    // Subscribe to the changed signal
    sigc::connection conn = GlobalTextureToolSelectionSystem().signal_activeManipulatorChanged().connect(
        [&](selection::IManipulator::Type type)
    {
        signalFired = true;
        signalArgument = type;
    });

    // We're starting in drag mode, so no changed expected
    GlobalCommandSystem().executeCommand("ToggleTextureToolManipulatorMode", { "Drag" });
    EXPECT_FALSE(signalFired) << "Signal shouldn't have fired";
    signalFired = false;

    // Changing to Rotate should fire the signal
    GlobalCommandSystem().executeCommand("ToggleTextureToolManipulatorMode", { "Rotate" });
    EXPECT_TRUE(signalFired) << "Signal should have fired";
    EXPECT_EQ(signalArgument, selection::IManipulator::Rotate) << "Signal communicated wrong mode";
    signalFired = false;

    // Toggle Rotate, should switch back to Drag
    GlobalCommandSystem().executeCommand("ToggleTextureToolManipulatorMode", { "Rotate" });
    EXPECT_TRUE(signalFired) << "Signal should have fired";
    EXPECT_EQ(signalArgument, selection::IManipulator::Drag) << "Signal communicated wrong mode";
    signalFired = false;

    // Changing to Rotate (again) should fire the signal
    GlobalCommandSystem().executeCommand("ToggleTextureToolManipulatorMode", { "Rotate" });
    EXPECT_TRUE(signalFired) << "Signal should have fired";
    EXPECT_EQ(signalArgument, selection::IManipulator::Rotate) << "Signal communicated wrong mode";
    signalFired = false;

    // Directly toggle to Drag, should fire
    GlobalCommandSystem().executeCommand("ToggleTextureToolManipulatorMode", { "Drag" });
    EXPECT_TRUE(signalFired) << "Signal should have fired";
    EXPECT_EQ(signalArgument, selection::IManipulator::Drag) << "Signal communicated wrong mode";
    signalFired = false;

    conn.disconnect();
}

TEST_F(TextureToolTest, ToggleSelectionMode)
{
    bool signalFired = false;
    textool::SelectionMode signalArgument;

    // Subscribe to the changed signal
    sigc::connection conn = GlobalTextureToolSelectionSystem().signal_selectionModeChanged().connect(
        [&](textool::SelectionMode mode)
    {
        signalFired = true;
        signalArgument = mode;
    });

    // We're starting in Surface mode, toggle to Surface again
    GlobalCommandSystem().executeCommand("ToggleTextureToolSelectionMode", { "Surface" });
    EXPECT_EQ(GlobalTextureToolSelectionSystem().getMode(), textool::SelectionMode::Surface);
    EXPECT_FALSE(signalFired) << "Signal shouldn't have fired";
    signalFired = false;

    // Switch to vertex mode
    GlobalCommandSystem().executeCommand("ToggleTextureToolSelectionMode", { "Vertex" });
    EXPECT_EQ(GlobalTextureToolSelectionSystem().getMode(), textool::SelectionMode::Vertex);
    EXPECT_TRUE(signalFired) << "Signal should have fired";
    signalFired = false;

    // Toggle vertex mode again => back to surface mode
    GlobalCommandSystem().executeCommand("ToggleTextureToolSelectionMode", { "Vertex" });
    EXPECT_EQ(GlobalTextureToolSelectionSystem().getMode(), textool::SelectionMode::Surface);
    EXPECT_TRUE(signalFired) << "Signal should have fired";
    signalFired = false;

    // Switch to vertex mode (again)
    GlobalCommandSystem().executeCommand("ToggleTextureToolSelectionMode", { "Vertex" });
    EXPECT_EQ(GlobalTextureToolSelectionSystem().getMode(), textool::SelectionMode::Vertex);
    EXPECT_TRUE(signalFired) << "Signal should have fired";
    signalFired = false;

    // Directly toggle surface mode
    GlobalCommandSystem().executeCommand("ToggleTextureToolSelectionMode", { "Surface" });
    EXPECT_EQ(GlobalTextureToolSelectionSystem().getMode(), textool::SelectionMode::Surface);
    EXPECT_TRUE(signalFired) << "Signal should have fired";
    signalFired = false;
}

TEST_F(TextureToolTest, SelectionModeChangedSignal)
{
    bool signalFired = false;
    textool::SelectionMode signalArgument;

    // Subscribe to the changed signal
    sigc::connection conn = GlobalTextureToolSelectionSystem().signal_selectionModeChanged().connect(
        [&] (textool::SelectionMode mode)
        {
            signalFired = true;
            signalArgument = mode;
        });

    // We're starting in Surface mode, so no changed expected
    GlobalTextureToolSelectionSystem().setMode(textool::SelectionMode::Surface);
    EXPECT_FALSE(signalFired) << "Signal shouldn't have fired";
    signalFired = false;

    GlobalTextureToolSelectionSystem().setMode(textool::SelectionMode::Vertex);
    EXPECT_TRUE(signalFired) << "Signal should have fired";
    EXPECT_EQ(signalArgument, textool::SelectionMode::Vertex) << "Signal communicated wrong mode";
    signalFired = false;

    // Switch to the same mode again => no signal expected
    GlobalTextureToolSelectionSystem().setMode(textool::SelectionMode::Vertex);
    EXPECT_FALSE(signalFired) << "Signal shouldn't have fired";
    signalFired = false;

    // Back to surface mode
    GlobalTextureToolSelectionSystem().setMode(textool::SelectionMode::Surface);
    EXPECT_TRUE(signalFired) << "Signal should have fired";
    EXPECT_EQ(signalArgument, textool::SelectionMode::Surface) << "Signal communicated wrong mode";
    signalFired = false;

    conn.disconnect();
}

TEST_F(TextureToolTest, TestSelectPatchByPoint)
{
    auto patchNode = setupPatchNodeForTextureTool();
    auto patch = Node_getIPatch(patchNode);

    // Get the texture space bounds of this patch
    auto bounds = getTextureSpaceBounds(*patch);

    // Construct a view that includes the patch UV bounds
    bounds.extents *= 1.2f;

    render::TextureToolView view;
    view.constructFromTextureSpaceBounds(bounds, TEXTOOL_WIDTH, TEXTOOL_HEIGHT);

    // Check the device coords of the patch centroid
    auto centroid = bounds.origin;
    auto centroidTransformed = view.GetViewProjection().transformPoint(Vector3(centroid.x(), centroid.y(), 0));
    Vector2 devicePoint(centroidTransformed.x(), centroidTransformed.y());

    // Use the device point we calculated for this vertex and use it to construct a selection test
    ConstructSelectionTest(view, selection::Rectangle::ConstructFromPoint(devicePoint, Vector2(0.02f, 0.02f)));

    SelectionVolume test(view);
    GlobalTextureToolSelectionSystem().selectPoint(test, SelectionSystem::eToggle);

    // Check if the node was selected
    std::vector<textool::INode::Ptr> selectedNodes;
    GlobalTextureToolSelectionSystem().foreachSelectedNode([&](const textool::INode::Ptr& node)
    {
        selectedNodes.push_back(node);
        return true;
    });

    EXPECT_EQ(selectedNodes.size(), 1) << "Only one patch should be selected";
}

TEST_F(TextureToolTest, TestSelectFaceByPoint)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush = algorithm::createCubicBrush(worldspawn, Vector3(0, 256, 256), "textures/numbers/1");
    scene::addNodeToContainer(brush, worldspawn);

    // Put all faces into the tex tool scene
    Node_setSelected(brush, true);

    auto faceUp = algorithm::findBrushFaceWithNormal(Node_getIBrush(brush), Vector3(0, 0, 1));

    // Check the face
    auto textoolFace = findTexToolFaceWithNormal(faceUp->getPlane3().normal());
    EXPECT_FALSE(textoolFace->isSelected()) << "Face should be unselected at start";

    // Get the texture space bounds of this face
    // Construct a view that includes the patch UV bounds
    auto bounds = getTextureSpaceBounds(*faceUp);
    bounds.extents *= 1.2f;

    render::TextureToolView view;
    view.constructFromTextureSpaceBounds(bounds, TEXTOOL_WIDTH, TEXTOOL_HEIGHT);

    // Check the device coords of the face centroid
    auto centroid = algorithm::getFaceCentroid(faceUp);
    auto centroidTransformed = view.GetViewProjection().transformPoint(Vector3(centroid.x(), centroid.y(), 0));
    Vector2 devicePoint(centroidTransformed.x(), centroidTransformed.y());

    // Use the device point we calculated for this vertex and use it to construct a selection test
    ConstructSelectionTest(view, selection::Rectangle::ConstructFromPoint(devicePoint, Vector2(0.02f, 0.02f)));

    SelectionVolume test(view);
    GlobalTextureToolSelectionSystem().selectPoint(test, SelectionSystem::eToggle);

    // Check if the node was selected
    std::vector<textool::INode::Ptr> selectedNodes;
    GlobalTextureToolSelectionSystem().foreachSelectedNode([&](const textool::INode::Ptr& node)
    {
        selectedNodes.push_back(node);
        return true;
    });

    EXPECT_EQ(selectedNodes.size(), 1) << "Only one item should be selected";
    EXPECT_EQ(selectedNodes.front(), textoolFace) << "The face should be selected";
}

TEST_F(TextureToolTest, TestSelectPatchByArea)
{
    auto patchNode = setupPatchNodeForTextureTool();
    auto patch = Node_getIPatch(patchNode);

    // Get the texture space bounds of this patch
    auto bounds = getTextureSpaceBounds(*patch);

    // Construct a view that includes the patch UV bounds
    bounds.extents *= 1.2f;

    render::TextureToolView view;
    view.constructFromTextureSpaceBounds(bounds, TEXTOOL_WIDTH, TEXTOOL_HEIGHT);

    // Use the device point we calculated for this vertex and use it to construct a selection test
    ConstructSelectionTest(view, selection::Rectangle::ConstructFromArea(Vector2(-0.95f, -0.95f), Vector2(0.95f*2, 0.95f*2)));

    SelectionVolume test(view);
    GlobalTextureToolSelectionSystem().selectArea(test, SelectionSystem::eToggle);

    // Check if the node was selected
    std::vector<textool::INode::Ptr> selectedNodes;
    GlobalTextureToolSelectionSystem().foreachSelectedNode([&](const textool::INode::Ptr& node)
    {
        selectedNodes.push_back(node);
        return true;
    });

    EXPECT_EQ(selectedNodes.size(), 1) << "Only one patch should be selected";
}

inline std::vector<Vector2> getTexcoords(const IFace* face)
{
    std::vector<Vector2> uvs;

    for (const auto& vertex : face->getWinding())
    {
        uvs.push_back(vertex.texcoord);
    }

    return uvs;
}

void dragManipulateSelectionTowardsLowerRight(const Vector2& startTexcoord, const render::View& view)
{
    auto centroid = startTexcoord;
    auto centroidTransformed = view.GetViewProjection().transformPoint(Vector3(centroid.x(), centroid.y(), 0));
    Vector2 devicePoint(centroidTransformed.x(), centroidTransformed.y());

    GlobalTextureToolSelectionSystem().onManipulationStart();

    // Simulate a transformation by click-and-drag
    auto manipulator = GlobalTextureToolSelectionSystem().getActiveManipulator();
    EXPECT_EQ(manipulator->getType(), selection::IManipulator::Drag) << "Wrong manipulator";

    render::View scissored(view);
    ConstructSelectionTest(scissored, selection::Rectangle::ConstructFromPoint(devicePoint, Vector2(0.05, 0.05)));

    auto manipComponent = manipulator->getActiveComponent();
    auto pivot2World = GlobalTextureToolSelectionSystem().getPivot2World();
    manipComponent->beginTransformation(pivot2World, scissored, devicePoint);

    // Move the device point a bit to the lower right
    auto secondDevicePoint = devicePoint + (Vector2(1, -1) - devicePoint) / 2;

    render::View scissored2(view);
    ConstructSelectionTest(scissored2, selection::Rectangle::ConstructFromPoint(secondDevicePoint, Vector2(0.05, 0.05)));

    manipComponent->transform(pivot2World, scissored2, secondDevicePoint, selection::IManipulator::Component::Constraint::Unconstrained);

    GlobalTextureToolSelectionSystem().onManipulationFinished();
}

TEST_F(TextureToolTest, DragManipulateFace)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush = algorithm::createCubicBrush(worldspawn, Vector3(0, 256, 256), "textures/numbers/1");

    // Put all faces into the tex tool scene
    Node_setSelected(brush, true);

    auto faceUp = algorithm::findBrushFaceWithNormal(Node_getIBrush(brush), Vector3(0, 0, 1));
    auto faceDown = algorithm::findBrushFaceWithNormal(Node_getIBrush(brush), Vector3(0, 0, -1));

    // Remember the texcoords of this face
    auto oldFaceUpUvs = getTexcoords(faceUp);
    auto oldFaceDownUvs = getTexcoords(faceDown);

    // Select the face
    auto textoolFace = findTexToolFaceWithNormal(faceUp->getPlane3().normal());
    textoolFace->setSelected(true);

    // Get the texture space bounds of this face
    // Construct a view that includes the patch UV bounds
    auto bounds = getTextureSpaceBounds(*faceUp);
    bounds.extents *= 1.2f;

    render::TextureToolView view;
    view.constructFromTextureSpaceBounds(bounds, TEXTOOL_WIDTH, TEXTOOL_HEIGHT);

    // Check the device coords of the face centroid and manipulate from that point
    auto centroid = algorithm::getFaceCentroid(faceUp);
    dragManipulateSelectionTowardsLowerRight(centroid, view);

    // All the texcoords should have been moved to the lower right (U increased, V increased)
    auto oldUv = oldFaceUpUvs.begin();
    for (const auto& vertex : faceUp->getWinding())
    {
        EXPECT_LT(oldUv->x(), vertex.texcoord.x());
        EXPECT_LT(oldUv->y(), vertex.texcoord.y());
        ++oldUv;
    }

    // The texcoords of the other face should not have been changed
    oldUv = oldFaceDownUvs.begin();
    for (const auto& vertex : faceDown->getWinding())
    {
        EXPECT_EQ(oldUv->x(), vertex.texcoord.x());
        EXPECT_EQ(oldUv->y(), vertex.texcoord.y());
        ++oldUv;
    }
}

TEST_F(TextureToolTest, DragManipulatePatch)
{
    auto patchNode = setupPatchNodeForTextureTool();
    auto patch = Node_getIPatch(patchNode);

    // Remember the texcoords before manipulation
    std::vector<Vector2> oldTexcoords;
    foreachPatchVertex(*patch, [&](const PatchControl& control) { oldTexcoords.push_back(control.texcoord); });

    auto texToolPatch = getFirstTextureToolNode();
    texToolPatch->setSelected(true);

    // Get the texture space bounds of this patch
    auto bounds = getTextureSpaceBounds(*patch);
    bounds.extents *= 1.2f;

    render::TextureToolView view;
    view.constructFromTextureSpaceBounds(bounds, TEXTOOL_WIDTH, TEXTOOL_HEIGHT);

    // Check the device coords of the face centroid
    auto centroid = Vector2(bounds.origin.x(), bounds.origin.y());
    dragManipulateSelectionTowardsLowerRight(centroid, view);

    // All the texcoords should have been moved to the lower right (U increased, V increased)
    auto oldUv = oldTexcoords.begin();
    foreachPatchVertex(*patch, [&](const PatchControl& control)
    {
        EXPECT_LT(oldUv->x(), control.texcoord.x());
        EXPECT_LT(oldUv->y(), control.texcoord.y());
        ++oldUv;
    });
}

}
