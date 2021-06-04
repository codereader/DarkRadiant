#include "RadiantTest.h"

#include "icommandsystem.h"
#include "itransformable.h"
#include "ibrush.h"
#include "imapresource.h"
#include "ipatch.h"
#include "icomparablenode.h"
#include "algorithm/Scene.h"
#include "registry/registry.h"
#include "scenelib.h"
#include "scene/merge/GraphComparer.h"
#include "scene/merge/MergeOperation.h"

namespace test
{

using MapMergeTest = RadiantTest;

TEST_F(MapMergeTest, BrushFingerprint)
{
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/fingerprinting.mapx"));

    auto originalMaterial = "textures/numbers/1";
    auto brush = std::dynamic_pointer_cast<IBrushNode>(algorithm::findFirstBrushWithMaterial(
        GlobalMapModule().findOrInsertWorldspawn(), originalMaterial));

    auto comparable = std::dynamic_pointer_cast<scene::IComparableNode>(brush);
    EXPECT_TRUE(comparable) << "BrushNode is not implementing IComparableNode";

    auto originalFingerprint = comparable->getFingerprint();

    EXPECT_FALSE(originalFingerprint.empty()); // shouldn't be empty

    // Calling it twice shouldn't change the value
    EXPECT_EQ(comparable->getFingerprint(), originalFingerprint);

    // Change the material of this brush, the fingerprint should change now
    brush->getIBrush().setShader("textures/somethingelse");
    EXPECT_NE(comparable->getFingerprint(), originalFingerprint);

    // Change it back
    brush->getIBrush().setShader(originalMaterial);
    EXPECT_EQ(comparable->getFingerprint(), originalFingerprint);

    // Change the contents flags, fingerprint should change
    brush->getIBrush().setDetailFlag(IBrush::Detail);
    EXPECT_NE(comparable->getFingerprint(), originalFingerprint);

    // Change it back
    brush->getIBrush().setDetailFlag(IBrush::Structural);
    EXPECT_EQ(comparable->getFingerprint(), originalFingerprint);

    // Get the first face and tinker with it
    auto& face = brush->getIBrush().getFace(0);

    auto lastFingerprint = comparable->getFingerprint();

    // Changing the texture should modify the fingerprint
    face.flipTexture(0);
    EXPECT_NE(comparable->getFingerprint(), lastFingerprint);
    lastFingerprint = comparable->getFingerprint();

    // Disable texture lock
    registry::setValue(RKEY_ENABLE_TEXTURE_LOCK, false);

    // Changing the planes
    auto transformable = std::dynamic_pointer_cast<ITransformable>(brush);
    transformable->setTranslation(Vector3(10,0,0));
    transformable->freezeTransform();

    EXPECT_NE(comparable->getFingerprint(), lastFingerprint);
    lastFingerprint = comparable->getFingerprint();
}

TEST_F(MapMergeTest, PatchFingerprint)
{
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/fingerprinting.mapx"));

    auto originalMaterial = "textures/numbers/1";
    auto patch = std::dynamic_pointer_cast<IPatchNode>(algorithm::findFirstPatchWithMaterial(
        GlobalMapModule().findOrInsertWorldspawn(), originalMaterial));

    auto comparable = std::dynamic_pointer_cast<scene::IComparableNode>(patch);
    EXPECT_TRUE(comparable) << "PatchNode is not implementing IComparableNode";

    auto originalFingerprint = comparable->getFingerprint();

    EXPECT_FALSE(originalFingerprint.empty()); // shouldn't be empty

    // Calling it twice shouldn't change the value
    EXPECT_EQ(comparable->getFingerprint(), originalFingerprint);

    // Change the material of this patch, the fingerprint should change now
    patch->getPatch().setShader("textures/somethingelse");
    EXPECT_NE(comparable->getFingerprint(), originalFingerprint);

    // Change it back
    patch->getPatch().setShader(originalMaterial);
    EXPECT_EQ(comparable->getFingerprint(), originalFingerprint);

    // Change a single control vertex
    auto& control = patch->getPatch().ctrlAt(0, 0);

    auto lastFingerprint = comparable->getFingerprint();

    // Change a 3D coordinate
    control.vertex.x() += 0.1;
    EXPECT_NE(comparable->getFingerprint(), lastFingerprint);
    lastFingerprint = comparable->getFingerprint();

    // Change a 2D component
    control.texcoord.x() += 0.1;
    EXPECT_NE(comparable->getFingerprint(), lastFingerprint);
    lastFingerprint = comparable->getFingerprint();

    // Append vertices
    patch->getPatch().appendPoints(true, false);
    EXPECT_NE(comparable->getFingerprint(), lastFingerprint);
}

TEST_F(MapMergeTest, EntityFingerprint)
{
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/fingerprinting.mapx"));

    auto entityNode = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_1");

    auto comparable = std::dynamic_pointer_cast<scene::IComparableNode>(entityNode);
    EXPECT_TRUE(comparable) << "EntityNode is not implementing IComparableNode";

    auto entity = std::dynamic_pointer_cast<IEntityNode>(entityNode);
    auto originalFingerprint = comparable->getFingerprint();

    EXPECT_FALSE(originalFingerprint.empty()); // shouldn't be empty

    // Calling it twice shouldn't change the value
    EXPECT_EQ(comparable->getFingerprint(), originalFingerprint);

    // Change a spawnarg slightly
    auto oldOrigin = entity->getEntity().getKeyValue("origin");

    entity->getEntity().setKeyValue("origin", "96 -32 53");
    EXPECT_NE(comparable->getFingerprint(), originalFingerprint);

    // Change it back
    entity->getEntity().setKeyValue("origin", oldOrigin);
    EXPECT_EQ(comparable->getFingerprint(), originalFingerprint);

    // Add a new spawnarg
    entity->getEntity().setKeyValue("origin22", "whatever");
    EXPECT_NE(comparable->getFingerprint(), originalFingerprint);

    // Change it back
    entity->getEntity().setKeyValue("origin22", "");
    EXPECT_EQ(comparable->getFingerprint(), originalFingerprint);

    // Remove a spawnarg, this alters the order of keyvalues
    auto originalValue = entity->getEntity().getKeyValue("dummyspawnarg");

    entity->getEntity().setKeyValue("dummyspawnarg", "");
    EXPECT_NE(comparable->getFingerprint(), originalFingerprint);

    // Change it back, even though the order is different, the fingerprint should be the same
    entity->getEntity().setKeyValue("dummyspawnarg", originalValue);
    EXPECT_EQ(comparable->getFingerprint(), originalFingerprint);
}

TEST_F(MapMergeTest, EntityFingerprintConsidersChildNodes)
{
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/fingerprinting.mapx"));

    auto entityNode = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_1");

    auto comparable = std::dynamic_pointer_cast<scene::IComparableNode>(entityNode);
    EXPECT_TRUE(comparable) << "EntityNode is not implementing IComparableNode";

    auto entity = std::dynamic_pointer_cast<IEntityNode>(entityNode);
    auto originalFingerprint = comparable->getFingerprint();

    // Add a child patch
    auto patchNode = GlobalPatchModule().createPatch(patch::PatchDefType::Def3);
    std::dynamic_pointer_cast<IPatchNode>(patchNode)->getPatch().setDims(3, 3);
    scene::addNodeToContainer(patchNode, entityNode);
    EXPECT_NE(comparable->getFingerprint(), originalFingerprint);

    // Remove the patch again
    scene::removeNodeFromParent(patchNode);
    EXPECT_EQ(comparable->getFingerprint(), originalFingerprint);
}

TEST_F(MapMergeTest, EntityFingerprintInsensitiveToChildOrder)
{
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/fingerprinting.mapx"));

    auto entityNode = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_1");

    auto comparable = std::dynamic_pointer_cast<scene::IComparableNode>(entityNode);
    EXPECT_TRUE(comparable) << "EntityNode is not implementing IComparableNode";

    auto entity = std::dynamic_pointer_cast<IEntityNode>(entityNode);
    auto originalFingerprint = comparable->getFingerprint();

    // Find the first brush of this func_static
    scene::INodePtr firstChild;
    std::size_t numChildren = 0;
    entity->foreachNode([&](const scene::INodePtr& node) 
    { 
        numChildren++;
        if (!firstChild)
        {
            firstChild = node;
        }

        return true;
    });
    EXPECT_TRUE(firstChild) << "func_static doesn't have any child nodes";
    EXPECT_GT(numChildren, 1) << "We need to have more than one child node in this func_static";

    // Remove it from the parent
    scene::removeNodeFromParent(firstChild);
    EXPECT_NE(comparable->getFingerprint(), originalFingerprint);

    // Adding it back to the entity will change the order of children,
    scene::addNodeToContainer(firstChild, entityNode);
    // Hash should stay the same
    EXPECT_EQ(comparable->getFingerprint(), originalFingerprint);
}

using namespace scene::merge;

inline ComparisonResult::Ptr performComparison(const std::string& targetMap, const std::string& sourceMapPath)
{
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument(targetMap));

    auto resource = GlobalMapResourceManager().createFromPath(sourceMapPath);
    EXPECT_TRUE(resource->load()) << "Test map not found in path " << sourceMapPath;

    return GraphComparer::Compare(resource->getRootNode(), GlobalMapModule().getRoot());
}

inline std::size_t countPrimitiveDifference(const ComparisonResult::EntityDifference& diff, 
    const ComparisonResult::PrimitiveDifference::Type type)
{
    std::size_t count = 0;

    for (const auto& difference : diff.differingChildren)
    {
        if (difference.type == type)
        {
            ++count;
        }
    }

    return count;
}

inline bool hasKeyValueDifference(const ComparisonResult::EntityDifference& diff, 
    const std::string& key, const std::string& value, ComparisonResult::KeyValueDifference::Type type)
{
    for (const auto& difference : diff.differingKeyValues)
    {
        if (difference.type == type && difference.key == key && difference.value == value)
        {
            return true;
        }
    }

    return false;
}

inline ComparisonResult::EntityDifference getEntityDifference(const ComparisonResult::Ptr& result, const std::string& name)
{
    for (const auto& difference : result->differingEntities)
    {
        if (difference.entityName == name)
        {
            return difference;
        }
    }

    return ComparisonResult::EntityDifference();
}

TEST_F(MapMergeTest, DetectMissingEntities)
{
    auto result = performComparison("maps/fingerprinting.mapx", _context.getTestProjectPath() + "maps/fingerprinting_2.mapx");

    // The player start has been removed in the changed map
    auto diff = getEntityDifference(result, "info_player_start_1");
    
    EXPECT_EQ(diff.type, ComparisonResult::EntityDifference::Type::EntityMissingInSource);
    EXPECT_TRUE(diff.baseNode);
    EXPECT_FALSE(diff.sourceNode); // source node is missing, so it must be empty
}

TEST_F(MapMergeTest, DetectAddedEntities)
{
    auto result = performComparison("maps/fingerprinting.mapx", _context.getTestProjectPath() + "maps/fingerprinting_2.mapx");

    // light_3 start has been added to the changed map
    auto diff = getEntityDifference(result, "light_3");

    EXPECT_EQ(diff.type, ComparisonResult::EntityDifference::Type::EntityMissingInBase);
    EXPECT_TRUE(diff.sourceNode);
    EXPECT_FALSE(diff.baseNode); // base node is missing, so it must be empty

    diff = getEntityDifference(result, "func_static_2");
    EXPECT_EQ(diff.type, ComparisonResult::EntityDifference::Type::EntityMissingInBase);
    EXPECT_TRUE(diff.sourceNode);
    EXPECT_FALSE(diff.baseNode); // base node is missing, so it must be empty
}

TEST_F(MapMergeTest, DetectAddedKeyValues)
{
    auto result = performComparison("maps/fingerprinting.mapx", _context.getTestProjectPath() + "maps/fingerprinting_2.mapx");

    // light_1 has different key values, dist_check_period = 40 has been added
    auto diff = getEntityDifference(result, "light_1");

    EXPECT_EQ(diff.type, ComparisonResult::EntityDifference::Type::EntityPresentButDifferent);
    EXPECT_TRUE(hasKeyValueDifference(diff, "dist_check_period", "40", ComparisonResult::KeyValueDifference::Type::KeyValueAdded));
}

TEST_F(MapMergeTest, DetectRemovedKeyValues)
{
    auto result = performComparison("maps/fingerprinting.mapx", _context.getTestProjectPath() + "maps/fingerprinting_2.mapx");

    // light_1 has different key values, break = 1 has been removed
    auto diff = getEntityDifference(result, "light_1");

    EXPECT_EQ(diff.type, ComparisonResult::EntityDifference::Type::EntityPresentButDifferent);
    EXPECT_TRUE(hasKeyValueDifference(diff, "break", "1", ComparisonResult::KeyValueDifference::Type::KeyValueRemoved));
}

TEST_F(MapMergeTest, DetectChangedKeyValues)
{
    auto result = performComparison("maps/fingerprinting.mapx", _context.getTestProjectPath() + "maps/fingerprinting_2.mapx");

    // light_1 has different key values, "ai_see" has been changed to "1"
    auto diff = getEntityDifference(result, "light_1");

    EXPECT_EQ(diff.type, ComparisonResult::EntityDifference::Type::EntityPresentButDifferent);
    EXPECT_TRUE(hasKeyValueDifference(diff, "ai_see", "1", ComparisonResult::KeyValueDifference::Type::KeyValueChanged));

    // light_2 has a different origin
    diff = getEntityDifference(result, "light_2");
    EXPECT_EQ(diff.type, ComparisonResult::EntityDifference::Type::EntityPresentButDifferent);
    EXPECT_TRUE(hasKeyValueDifference(diff, "origin", "280 160 0", ComparisonResult::KeyValueDifference::Type::KeyValueChanged));
}

TEST_F(MapMergeTest, DetectChildPrimitiveChanges)
{
    auto result = performComparison("maps/fingerprinting.mapx", _context.getTestProjectPath() + "maps/fingerprinting_2.mapx");

    // func_static_30 has changed primitives
    auto diff = getEntityDifference(result, "func_static_30");

    EXPECT_EQ(diff.type, ComparisonResult::EntityDifference::Type::EntityPresentButDifferent);
    EXPECT_EQ(diff.differingChildren.size(), 1);
    EXPECT_EQ(diff.differingChildren.front().type, ComparisonResult::PrimitiveDifference::Type::PrimitiveAdded);
    EXPECT_EQ(diff.differingChildren.front().node->getNodeType(), scene::INode::Type::Brush);

    // func_static_1 has 2 additions and 1 removal (== 1 addition, 1 replacement)
    diff = getEntityDifference(result, "func_static_1");

    EXPECT_EQ(diff.type, ComparisonResult::EntityDifference::Type::EntityPresentButDifferent);
    EXPECT_EQ(countPrimitiveDifference(diff, ComparisonResult::PrimitiveDifference::Type::PrimitiveAdded), 2);
    EXPECT_EQ(countPrimitiveDifference(diff, ComparisonResult::PrimitiveDifference::Type::PrimitiveRemoved), 1);

    // worldspawn has a couple of changes
    diff = getEntityDifference(result, "worldspawn");

    EXPECT_EQ(diff.type, ComparisonResult::EntityDifference::Type::EntityPresentButDifferent);
    EXPECT_EQ(countPrimitiveDifference(diff, ComparisonResult::PrimitiveDifference::Type::PrimitiveAdded), 3);
    EXPECT_EQ(countPrimitiveDifference(diff, ComparisonResult::PrimitiveDifference::Type::PrimitiveRemoved), 3);
}

template<typename T>
std::shared_ptr<T> findAction(const MergeOperation::Ptr& operation, const std::function<bool(const std::shared_ptr<T>&)>& predicate)
{
    std::shared_ptr<T> foundAction;

    operation->foreachAction([&](const IMergeAction::Ptr& action)
    {
        if (foundAction) return;

        auto derivedAction = std::dynamic_pointer_cast<T>(action);

        if (!derivedAction || !predicate(derivedAction)) return;

        foundAction = derivedAction;
    });

    return foundAction;
}

template<typename T>
std::size_t countActions(const MergeOperation::Ptr& operation, const std::function<bool(const std::shared_ptr<T>&)>& predicate)
{
    std::size_t count = 0;

    operation->foreachAction([&](const IMergeAction::Ptr& action)
    {
        auto derivedAction = std::dynamic_pointer_cast<T>(action);

        if (!derivedAction || !predicate(derivedAction)) return;

        ++count;
    });

    return count;
}

TEST_F(MapMergeTest, MergeActionsForMissingEntities)
{
    auto result = performComparison("maps/fingerprinting.mapx", _context.getTestProjectPath() + "maps/fingerprinting_2.mapx");
    auto operation = MergeOperation::CreateFromComparisonResult(*result);

    // light_3
    auto action = findAction<AddEntityAction>(operation, [](const std::shared_ptr<AddEntityAction>& action)
    {
        auto sourceEntity = Node_getEntity(action->getSourceNodeToAdd());
        return sourceEntity->getKeyValue("name") == "light_3";
    });

    EXPECT_TRUE(action) << "No merge action found for missing entity";

    // Check pre-requisites and apply the action
    auto entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "light_3");
    EXPECT_FALSE(entityNode);

    action->applyChanges();

    entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "light_3");
    EXPECT_TRUE(entityNode);

    // func_static_2
    action = findAction<AddEntityAction>(operation, [](const std::shared_ptr<AddEntityAction>& action)
    {
        auto sourceEntity = Node_getEntity(action->getSourceNodeToAdd());
        return sourceEntity->getKeyValue("name") == "func_static_2";
    });

    EXPECT_TRUE(action) << "No merge action found for missing entity";

    // Check pre-requisites and apply the action
    entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "func_static_2");
    EXPECT_FALSE(entityNode);

    action->applyChanges();

    entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "func_static_2");
    EXPECT_TRUE(entityNode);

    // Assume the child primitives have been added too
    EXPECT_TRUE(scene::hasChildPrimitives(entityNode));
}

TEST_F(MapMergeTest, MergeActionsForRemovedEntities)
{
    auto result = performComparison("maps/fingerprinting.mapx", _context.getTestProjectPath() + "maps/fingerprinting_2.mapx");
    auto operation = MergeOperation::CreateFromComparisonResult(*result);

    auto action = findAction<RemoveEntityAction>(operation, [](const std::shared_ptr<RemoveEntityAction>& action)
    {
        auto entity = Node_getEntity(action->getNodeToRemove());
        return entity->getKeyValue("name") == "info_player_start_1";
    });

    EXPECT_TRUE(action) << "No merge action found for removed entity";

    // Check pre-requisites and apply the action
    auto entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "info_player_start_1");
    EXPECT_TRUE(entityNode);

    action->applyChanges();

    entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "info_player_start_1");
    EXPECT_FALSE(entityNode);
}

TEST_F(MapMergeTest, MergeActionsForAddedKeyValues)
{
    auto result = performComparison("maps/fingerprinting.mapx", _context.getTestProjectPath() + "maps/fingerprinting_2.mapx");
    auto operation = MergeOperation::CreateFromComparisonResult(*result);

    auto action = findAction<AddEntityKeyValueAction>(operation, [](const std::shared_ptr<AddEntityKeyValueAction>& action)
    {
        auto entity = Node_getEntity(action->getEntityNode());
        return entity->getKeyValue("name") == "light_1" && action->getKey() == "dist_check_period" && action->getValue() == "40";
    });

    EXPECT_TRUE(action) << "No merge action found for added key value";

    // Check pre-requisites and apply the action
    auto entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "light_1");
    EXPECT_EQ(Node_getEntity(entityNode)->getKeyValue("dist_check_period"), "");

    action->applyChanges();

    entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "light_1");
    EXPECT_EQ(Node_getEntity(entityNode)->getKeyValue("dist_check_period"), "40");
}

TEST_F(MapMergeTest, MergeActionsForRemovedKeyValues)
{
    auto result = performComparison("maps/fingerprinting.mapx", _context.getTestProjectPath() + "maps/fingerprinting_2.mapx");
    auto operation = MergeOperation::CreateFromComparisonResult(*result);

    auto action = findAction<RemoveEntityKeyValueAction>(operation, [](const std::shared_ptr<RemoveEntityKeyValueAction>& action)
    {
        auto entity = Node_getEntity(action->getEntityNode());
        return entity->getKeyValue("name") == "light_1" && action->getKey() == "break" && action->getValue().empty();
    });

    EXPECT_TRUE(action) << "No merge action found for removed key value";

    // Check pre-requisites and apply the action
    auto entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "light_1");
    EXPECT_EQ(Node_getEntity(entityNode)->getKeyValue("break"), "1");

    action->applyChanges();

    entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "light_1");
    EXPECT_EQ(Node_getEntity(entityNode)->getKeyValue("break"), "");
}

TEST_F(MapMergeTest, MergeActionsForChangedKeyValues)
{
    auto result = performComparison("maps/fingerprinting.mapx", _context.getTestProjectPath() + "maps/fingerprinting_2.mapx");
    auto operation = MergeOperation::CreateFromComparisonResult(*result);

    auto action = findAction<ChangeEntityKeyValueAction>(operation, [](const std::shared_ptr<ChangeEntityKeyValueAction>& action)
    {
        auto entity = Node_getEntity(action->getEntityNode());
        return entity->getKeyValue("name") == "light_1" && action->getKey() == "ai_see" && action->getValue() == "1";
    });

    EXPECT_TRUE(action) << "No merge action found for changed key value";

    // Check pre-requisites and apply the action
    auto entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "light_1");
    EXPECT_EQ(Node_getEntity(entityNode)->getKeyValue("ai_see"), "0");

    action->applyChanges();

    entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "light_1");
    EXPECT_EQ(Node_getEntity(entityNode)->getKeyValue("ai_see"), "1");

    action = findAction<ChangeEntityKeyValueAction>(operation, [](const std::shared_ptr<ChangeEntityKeyValueAction>& action)
    {
        auto entity = Node_getEntity(action->getEntityNode());
        return entity->getKeyValue("name") == "light_2" && action->getKey() == "origin" && action->getValue() == "280 160 0";
    });

    EXPECT_TRUE(action) << "No merge action found for changed key value";

    // Check pre-requisites and apply the action
    entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "light_2");
    EXPECT_NE(Node_getEntity(entityNode)->getKeyValue("origin"), "280 160 0");

    action->applyChanges();

    entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "light_2");
    EXPECT_EQ(Node_getEntity(entityNode)->getKeyValue("origin"), "280 160 0");
}

inline std::size_t getChildPrimitiveCount(const scene::INodePtr& node)
{
    std::size_t count = 0;
    node->foreachNode([&](const scene::INodePtr& node)
    {
        if (Node_isPrimitive(node))
        {
            ++count;
        }

        return true;
    });

    return count;
}

TEST_F(MapMergeTest, MergeActionsForChangedPrimitives)
{
    auto result = performComparison("maps/fingerprinting.mapx", _context.getTestProjectPath() + "maps/fingerprinting_2.mapx");
    auto operation = MergeOperation::CreateFromComparisonResult(*result);

    auto addAction = findAction<AddChildAction>(operation, [](const std::shared_ptr<AddChildAction>& action)
    {
        auto entity = Node_getEntity(action->getParent());
        return entity->getKeyValue("name") == "func_static_30" && action->getSourceNodeToAdd()->getNodeType() == scene::INode::Type::Brush;
    });

    EXPECT_TRUE(addAction) << "No merge action found for added child node";

    // func_static_1 has 2 additions and 1 removal (== 1 addition, 1 replacement)
    auto addActionCount = countActions<AddChildAction>(operation, [](const std::shared_ptr<AddChildAction>& action)
    {
        auto entity = Node_getEntity(action->getParent());
        return entity->getKeyValue("name") == "func_static_1";
    });
    auto removeActionCount = countActions<RemoveChildAction>(operation, [](const std::shared_ptr<RemoveChildAction>& action)
    {
        auto entity = Node_getEntity(action->getNodeToRemove()->getParent());
        return entity->getKeyValue("name") == "func_static_1";
    });

    EXPECT_EQ(addActionCount, 2) << "No merge action found for added child node";
    EXPECT_EQ(removeActionCount, 1) << "No merge action found for removed child node";

    addActionCount = countActions<AddChildAction>(operation, [](const std::shared_ptr<AddChildAction>& action)
    {
        auto entity = Node_getEntity(action->getParent());
        return entity->isWorldspawn();
    });
    removeActionCount = countActions<RemoveChildAction>(operation, [](const std::shared_ptr<RemoveChildAction>& action)
    {
        auto entity = Node_getEntity(action->getNodeToRemove()->getParent());
        return entity->isWorldspawn();
    });

    EXPECT_EQ(addActionCount, 3) << "No merge action found for added child node";
    EXPECT_EQ(removeActionCount, 3) << "No merge action found for removed child node";

    // func_static_1 should have 3 brushes after execution of the actions
    // Check prerequisites and execute action
    auto entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "func_static_1");
    EXPECT_EQ(getChildPrimitiveCount(entityNode), 2);

    operation->applyActions();

    entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "func_static_1");
    EXPECT_EQ(getChildPrimitiveCount(entityNode), 3);
}

TEST_F(MapMergeTest, ApplyMergeOperation)
{
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/fingerprinting.mapx"));

    auto changedMap = _context.getTestProjectPath() + "maps/fingerprinting_2.mapx";
    auto resource = GlobalMapResourceManager().createFromPath(changedMap);
    EXPECT_TRUE(resource->load()) << "Test map not found in path " << changedMap;

    auto result = GraphComparer::Compare(resource->getRootNode(), GlobalMapModule().getRoot());

    // The result should have differences
    EXPECT_FALSE(result->differingEntities.empty());

    auto operation = MergeOperation::CreateFromComparisonResult(*result);
    operation->applyActions();

    auto resultAfterExecution = GraphComparer::Compare(resource->getRootNode(), GlobalMapModule().getRoot());

    // The result should not list any differences anymore
    EXPECT_TRUE(resultAfterExecution->differingEntities.empty());
}

TEST_F(MapMergeTest, DeactivatedAddEntityAction)
{
    auto result = performComparison("maps/fingerprinting.mapx", _context.getTestProjectPath() + "maps/fingerprinting_2.mapx");
    auto operation = MergeOperation::CreateFromComparisonResult(*result);

    auto action = findAction<AddEntityAction>(operation, [](const std::shared_ptr<AddEntityAction>& action)
    {
        auto sourceEntity = Node_getEntity(action->getSourceNodeToAdd());
        return sourceEntity->getKeyValue("name") == "light_3";
    });

    // Check pre-requisites and apply the action
    auto entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "light_3");
    EXPECT_FALSE(entityNode);

    action->deactivate();
    action->applyChanges();

    entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "light_3");
    EXPECT_FALSE(entityNode); // must still be missing
}

TEST_F(MapMergeTest, DeactivatedRemoveEntityAction)
{
    auto result = performComparison("maps/fingerprinting.mapx", _context.getTestProjectPath() + "maps/fingerprinting_2.mapx");
    auto operation = MergeOperation::CreateFromComparisonResult(*result);

    auto action = findAction<RemoveEntityAction>(operation, [](const std::shared_ptr<RemoveEntityAction>& action)
    {
        auto entity = Node_getEntity(action->getNodeToRemove());
        return entity->getKeyValue("name") == "info_player_start_1";
    });

    // Check pre-requisites and apply the action
    auto entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "info_player_start_1");
    EXPECT_TRUE(entityNode);

    action->deactivate();
    action->applyChanges();

    entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "info_player_start_1");
    EXPECT_TRUE(entityNode); // must still be here
}

TEST_F(MapMergeTest, DeactivatedAddEntityKeyValueAction)
{
    auto result = performComparison("maps/fingerprinting.mapx", _context.getTestProjectPath() + "maps/fingerprinting_2.mapx");
    auto operation = MergeOperation::CreateFromComparisonResult(*result);

    auto action = findAction<AddEntityKeyValueAction>(operation, [](const std::shared_ptr<AddEntityKeyValueAction>& action)
    {
        auto entity = Node_getEntity(action->getEntityNode());
        return entity->getKeyValue("name") == "light_1" && action->getKey() == "dist_check_period" && action->getValue() == "40";
    });

    // Check pre-requisites and apply the action
    auto entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "light_1");
    EXPECT_EQ(Node_getEntity(entityNode)->getKeyValue("dist_check_period"), "");

    action->deactivate();
    action->applyChanges();

    entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "light_1");
    EXPECT_EQ(Node_getEntity(entityNode)->getKeyValue("dist_check_period"), ""); // must still be unchanged
}

TEST_F(MapMergeTest, DeactivatedRemoveEntityKeyValueAction)
{
    auto result = performComparison("maps/fingerprinting.mapx", _context.getTestProjectPath() + "maps/fingerprinting_2.mapx");
    auto operation = MergeOperation::CreateFromComparisonResult(*result);

    auto action = findAction<RemoveEntityKeyValueAction>(operation, [](const std::shared_ptr<RemoveEntityKeyValueAction>& action)
    {
        auto entity = Node_getEntity(action->getEntityNode());
        return entity->getKeyValue("name") == "light_1" && action->getKey() == "break" && action->getValue().empty();
    });

    // Check pre-requisites and apply the action
    auto entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "light_1");
    EXPECT_EQ(Node_getEntity(entityNode)->getKeyValue("break"), "1");

    action->deactivate();
    action->applyChanges();

    entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "light_1");
    EXPECT_EQ(Node_getEntity(entityNode)->getKeyValue("break"), "1"); // must be unchanged
}

TEST_F(MapMergeTest, DeactivatedChangeEntityKeyValueAction)
{
    auto result = performComparison("maps/fingerprinting.mapx", _context.getTestProjectPath() + "maps/fingerprinting_2.mapx");
    auto operation = MergeOperation::CreateFromComparisonResult(*result);

    auto action = findAction<ChangeEntityKeyValueAction>(operation, [](const std::shared_ptr<ChangeEntityKeyValueAction>& action)
    {
        auto entity = Node_getEntity(action->getEntityNode());
        return entity->getKeyValue("name") == "light_1" && action->getKey() == "ai_see" && action->getValue() == "1";
    });

    // Check pre-requisites and apply the action
    auto entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "light_1");
    EXPECT_EQ(Node_getEntity(entityNode)->getKeyValue("ai_see"), "0");

    action->deactivate();
    action->applyChanges();

    entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "light_1");
    EXPECT_EQ(Node_getEntity(entityNode)->getKeyValue("ai_see"), "0");
}

TEST_F(MapMergeTest, DeactivatedChangePrimitiveActions)
{
    auto result = performComparison("maps/fingerprinting.mapx", _context.getTestProjectPath() + "maps/fingerprinting_2.mapx");
    auto operation = MergeOperation::CreateFromComparisonResult(*result);

    // func_static_1 has 2 additions and 1 removal (== 1 addition, 1 replacement)
    operation->foreachAction([&](const IMergeAction::Ptr& action)
    {
        auto addChildAction = std::dynamic_pointer_cast<AddChildAction>(action);

        if (addChildAction && Node_isEntity(addChildAction->getParent()) && 
            Node_getEntity(addChildAction->getParent())->getKeyValue("name") == "func_static_1")
        {
            addChildAction->deactivate();
        }

        auto removeChildAction = std::dynamic_pointer_cast<RemoveChildAction>(action);

        if (removeChildAction && Node_isEntity(removeChildAction->getNodeToRemove()->getParent()) &&
            Node_getEntity(removeChildAction->getNodeToRemove()->getParent())->getKeyValue("name") == "func_static_1")
        {
            removeChildAction->deactivate();
        }
    });
    
    // func_static_1 has 2 brushes, this should stay the same
    // Check prerequisites and execute action
    auto entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "func_static_1");
    EXPECT_EQ(getChildPrimitiveCount(entityNode), 2);

    operation->applyActions();

    entityNode = algorithm::getEntityByName(result->getBaseRootNode(), "func_static_1");
    EXPECT_EQ(getChildPrimitiveCount(entityNode), 2);
}

TEST_F(MapMergeTest, GroupDifferenceSameMap)
{
    auto result = performComparison("maps/merging_groups_1.mapx", _context.getTestProjectPath() + "maps/merging_groups_1.mapx");

    // Make sure we do have groups in the map
    std::size_t groupCount = 0;
    result->getSourceRootNode()->getSelectionGroupManager().foreachSelectionGroup([&](const selection::ISelectionGroup& group)
    {
        ++groupCount;
    });
    EXPECT_NE(groupCount, 0) << "We don't have any groups in the test map, this test is useless.";
    EXPECT_TRUE(result->selectionGroupDifferences.empty()) << "Comparing a map with itself shouldn't produce a group diff";
}

TEST_F(MapMergeTest, GroupMemberOrdering)
{
    auto originalResource = GlobalMapResourceManager().createFromPath("maps/merging_groups_1.mapx");
    EXPECT_TRUE(originalResource->load()) << "Test map not found";

    auto changedResource = GlobalMapResourceManager().createFromPath("maps/merging_groups_1.mapx");
    EXPECT_TRUE(changedResource->load()) << "Test map not found";

    auto result = GraphComparer::Compare(changedResource->getRootNode(), originalResource->getRootNode());
    EXPECT_TRUE(result->selectionGroupDifferences.empty()) << "Unchanged resource should be the same as the original";

    // Create a group from 3 defined brushes, in a specific order
    auto& originalGroupManager = originalResource->getRootNode()->getSelectionGroupManager();
    auto originalGroup = originalGroupManager.createSelectionGroup();

    // Find the three defined brushes
    auto brush11 = algorithm::findFirstBrushWithMaterial(algorithm::findWorldspawn(originalResource->getRootNode()), "textures/numbers/11");
    auto brush12 = algorithm::findFirstBrushWithMaterial(algorithm::findWorldspawn(originalResource->getRootNode()), "textures/numbers/12");
    auto brush13 = algorithm::findFirstBrushWithMaterial(algorithm::findWorldspawn(originalResource->getRootNode()), "textures/numbers/13");
    
    originalGroup->addNode(brush11);
    originalGroup->addNode(brush12);
    originalGroup->addNode(brush13);

    // Do the same in the other map, but in a different order
    auto& changedGroupManager = changedResource->getRootNode()->getSelectionGroupManager();
    auto changedGroup = changedGroupManager.createSelectionGroup();

    // Find the three defined brushes
    brush11 = algorithm::findFirstBrushWithMaterial(algorithm::findWorldspawn(changedResource->getRootNode()), "textures/numbers/11");
    brush12 = algorithm::findFirstBrushWithMaterial(algorithm::findWorldspawn(changedResource->getRootNode()), "textures/numbers/12");
    brush13 = algorithm::findFirstBrushWithMaterial(algorithm::findWorldspawn(changedResource->getRootNode()), "textures/numbers/13");

    changedGroup->addNode(brush11);
    changedGroup->addNode(brush13);
    changedGroup->addNode(brush12);

    result = GraphComparer::Compare(changedResource->getRootNode(), originalResource->getRootNode());
    EXPECT_TRUE(result->selectionGroupDifferences.empty()) << "Group ordering shouldn't make a difference";
}

// Entity with no changed geometry, but with changed group membership in its children
TEST_F(MapMergeTest, GroupDifferenceInMatchingEntity)
{
    auto originalResource = GlobalMapResourceManager().createFromPath("maps/merging_groups_1.mapx");
    EXPECT_TRUE(originalResource->load()) << "Test map not found";

    auto changedResource = GlobalMapResourceManager().createFromPath("maps/merging_groups_1.mapx");
    EXPECT_TRUE(changedResource->load()) << "Test map not found";

    auto result = GraphComparer::Compare(changedResource->getRootNode(), originalResource->getRootNode());
    EXPECT_TRUE(result->selectionGroupDifferences.empty()) << "Unchanged resource should be the same as the original";

    // Create a group from 3 defined brushes in the one map
    auto& originalGroupManager = originalResource->getRootNode()->getSelectionGroupManager();
    auto originalGroup = originalGroupManager.createSelectionGroup();

    auto brush11 = algorithm::findFirstBrushWithMaterial(algorithm::findWorldspawn(originalResource->getRootNode()), "textures/numbers/11");
    auto brush12 = algorithm::findFirstBrushWithMaterial(algorithm::findWorldspawn(originalResource->getRootNode()), "textures/numbers/12");
    auto brush13 = algorithm::findFirstBrushWithMaterial(algorithm::findWorldspawn(originalResource->getRootNode()), "textures/numbers/13");

    originalGroup->addNode(brush11);
    originalGroup->addNode(brush12);
    originalGroup->addNode(brush13);

    // Do the same in the other map, but with one more brush
    auto& changedGroupManager = changedResource->getRootNode()->getSelectionGroupManager();
    auto changedGroup = changedGroupManager.createSelectionGroup();

    // Find the three defined brushes
    brush11 = algorithm::findFirstBrushWithMaterial(algorithm::findWorldspawn(changedResource->getRootNode()), "textures/numbers/11");
    brush12 = algorithm::findFirstBrushWithMaterial(algorithm::findWorldspawn(changedResource->getRootNode()), "textures/numbers/12");
    brush13 = algorithm::findFirstBrushWithMaterial(algorithm::findWorldspawn(changedResource->getRootNode()), "textures/numbers/13");
    auto brush14 = algorithm::findFirstBrushWithMaterial(algorithm::findWorldspawn(changedResource->getRootNode()), "textures/numbers/14");

    changedGroup->addNode(brush11);
    changedGroup->addNode(brush12);
    changedGroup->addNode(brush13);
    changedGroup->addNode(brush14); // one additional brush

    result = GraphComparer::Compare(changedResource->getRootNode(), originalResource->getRootNode());

    // We should get 4 differences: one for each brush (brush11/12/13 and the additional brush14)
    EXPECT_EQ(result->selectionGroupDifferences.size(), 4) << "Group difference not detected";

    auto groupMemberMismatches = std::count_if(result->selectionGroupDifferences.begin(), result->selectionGroupDifferences.end(),
        [&](const ComparisonResult::GroupDifference& diff) { return diff.type == ComparisonResult::GroupDifference::Type::GroupMemberMismatch; });
    EXPECT_EQ(groupMemberMismatches, 3);

    auto membershipCountMismatches = std::count_if(result->selectionGroupDifferences.begin(), result->selectionGroupDifferences.end(),
        [&](const ComparisonResult::GroupDifference& diff) { return diff.type == ComparisonResult::GroupDifference::Type::MembershipCountMismatch; });
    EXPECT_EQ(membershipCountMismatches, 1);
}

// Entity with no changed geometry, entity itself has different group membership
TEST_F(MapMergeTest, GroupDifferenceOfMatchingEntity)
{
    auto originalResource = GlobalMapResourceManager().createFromPath("maps/merging_groups_1.mapx");
    EXPECT_TRUE(originalResource->load()) << "Test map not found";

    auto changedResource = GlobalMapResourceManager().createFromPath("maps/merging_groups_1.mapx");
    EXPECT_TRUE(changedResource->load()) << "Test map not found";

    auto result = GraphComparer::Compare(changedResource->getRootNode(), originalResource->getRootNode());
    EXPECT_TRUE(result->selectionGroupDifferences.empty()) << "Unchanged resource should be the same as the original";

    // Create a group from 3 defined brushes in the one map
    auto& changedGroupManager = changedResource->getRootNode()->getSelectionGroupManager();
    auto changedGroup = changedGroupManager.createSelectionGroup();

    // This entity already forms a group with light, add another group on top of that
    auto funcStatic = algorithm::getEntityByName(changedResource->getRootNode(), "expandable");
    auto brush11 = algorithm::findFirstBrushWithMaterial(algorithm::findWorldspawn(originalResource->getRootNode()), "textures/numbers/11");

    changedGroup->addNode(funcStatic);
    changedGroup->addNode(brush11);

    // Compare the unchanged map to the one with the new group
    result = GraphComparer::Compare(changedResource->getRootNode(), originalResource->getRootNode());

    // We should get 2 differences: one for the entity node itself and one for brush 11 (both have different group membership count)
    EXPECT_EQ(result->selectionGroupDifferences.size(), 2) << "Group difference not detected";

    auto membershipCountMismatches = std::count_if(result->selectionGroupDifferences.begin(), result->selectionGroupDifferences.end(),
        [&](const ComparisonResult::GroupDifference& diff) { return diff.type == ComparisonResult::GroupDifference::Type::MembershipCountMismatch; });
    EXPECT_EQ(membershipCountMismatches, 2);
}

}
