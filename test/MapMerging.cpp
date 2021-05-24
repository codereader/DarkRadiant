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
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/fingerprinting.mapx"));

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

    operation->foreachAction([&](const MergeAction::Ptr& action)
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

    operation->foreachAction([&](const MergeAction::Ptr& action)
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

    action = findAction<ChangeEntityKeyValueAction>(operation, [](const std::shared_ptr<ChangeEntityKeyValueAction>& action)
    {
        auto entity = Node_getEntity(action->getEntityNode());
        return entity->getKeyValue("name") == "light_2" && action->getKey() == "origin" && action->getValue() == "280 160 0";
    });

    EXPECT_TRUE(action) << "No merge action found for changed key value";
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
}

}
