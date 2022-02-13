#include "RadiantTest.h"

#include "scene/BasicRootNode.h"
#include "scene/Node.h"
#include "scenelib.h"

namespace test
{

using SceneNodeTest = RadiantTest;

// Custom Node class to test the protected onVisibilityChanged method behaviour
class VisibilityTestNode :
    public scene::Node
{
public:
    VisibilityTestNode() :
        visibilityMethodCalled(false)
    {}

    Type getNodeType() const override
    {
        return Type::Unknown;
    }

    const AABB& localAABB() const
    {
        static AABB dummy;
        return dummy;
    }

    void onPreRender(const VolumeTest& volume) override
    {}

    void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume) override
    {}

    std::size_t getHighlightFlags() override
    {
        return 0;
    }

    // Wrapper to invoke the protected method
    void setProtectedForcedVisibility(bool isForced)
    {
        setForcedVisibility(isForced, false);
    }

    // Public test fields
    bool visibilityMethodCalled = false;
    bool passedVisibilityValue = false;

protected:
    void onVisibilityChanged(bool newState) override
    {
        visibilityMethodCalled = true;
        passedVisibilityValue = newState;
    }
};

auto allPossibleHideFlags = { scene::Node::eHidden, scene::Node::eFiltered, scene::Node::eExcluded, scene::Node::eLayered };

TEST_F(SceneNodeTest, InitialVisibility)
{
    auto node = std::make_shared<VisibilityTestNode>();
    scene::addNodeToContainer(node, GlobalMapModule().getRoot());

    EXPECT_TRUE(node->visible()) << "Should be visible after insertion as child node";
}

TEST_F(SceneNodeTest, InsertAndRemoveFromScene)
{
    auto node = std::make_shared<VisibilityTestNode>();
    scene::addNodeToContainer(node, GlobalMapModule().getRoot());

    EXPECT_TRUE(node->visible()) << "Should be visible after insertion as child node";

    node->visibilityMethodCalled = false;
    scene::removeNodeFromParent(node);
    EXPECT_FALSE(node->visible()) << "Should be invisible after removing as child node";
    EXPECT_TRUE(node->visibilityMethodCalled) << "Method should be called after removing from the scene";
    EXPECT_FALSE(node->passedVisibilityValue) << "Wrong argument passed to onVisibilityChanged";

    node->visibilityMethodCalled = false;
    scene::addNodeToContainer(node, GlobalMapModule().getRoot());
    EXPECT_TRUE(node->visible()) << "Should be visible after re-adding as child node";
    EXPECT_TRUE(node->visibilityMethodCalled) << "Method should be called after inserting into scene";
    EXPECT_TRUE(node->passedVisibilityValue) << "Wrong argument passed to onVisibilityChanged";
}

TEST_F(SceneNodeTest, SetVisibleFlag)
{
    auto node = std::make_shared<VisibilityTestNode>();
    scene::addNodeToContainer(node, GlobalMapModule().getRoot());

    node->visibilityMethodCalled = false;

    node->enable(scene::Node::eVisible); // does nothing

    EXPECT_FALSE(node->visibilityMethodCalled) << "Should still visible after setting eVisible";
    EXPECT_TRUE(node->visible()) << "Should still visible after setting eVisible";
}

TEST_F(SceneNodeTest, SetSingleHideFlag)
{
    for (auto flag : allPossibleHideFlags)
    {
        auto node = std::make_shared<VisibilityTestNode>();
        scene::addNodeToContainer(node, GlobalMapModule().getRoot());

        node->visibilityMethodCalled = false;
        node->enable(flag);

        EXPECT_TRUE(node->visibilityMethodCalled) << "Method should have been invoked";
        EXPECT_FALSE(node->visible()) << "Should be invisible after setting a flag";
        EXPECT_FALSE(node->passedVisibilityValue) << "Got the wrong visibility changed argument";

        // Set the same flag again
        node->visibilityMethodCalled = false;
        node->enable(flag);

        EXPECT_FALSE(node->visibilityMethodCalled) << "Method shouldn't have been invoked, no change";
        EXPECT_FALSE(node->visible()) << "Should still be invisible";

        // Clear the flag
        node->visibilityMethodCalled = false;
        node->disable(flag);

        EXPECT_TRUE(node->visibilityMethodCalled) << "Method should have been invoked";
        EXPECT_TRUE(node->visible()) << "Should be visible again after clearing the flag";
        EXPECT_TRUE(node->passedVisibilityValue) << "Got the wrong visibility changed argument";

        // Clear the flag a second time
        node->visibilityMethodCalled = false;
        node->disable(flag);

        EXPECT_FALSE(node->visibilityMethodCalled) << "Method shouldn't have been invoked, no change";
        EXPECT_TRUE(node->visible()) << "Should still be visible";

        scene::removeNodeFromParent(node);
    }
}

TEST_F(SceneNodeTest, SetMultipleHideFlags)
{
    auto node = std::make_shared<VisibilityTestNode>();
    scene::addNodeToContainer(node, GlobalMapModule().getRoot());

    std::vector allFlags(allPossibleHideFlags);

    // Set all possible flags to hide the node
    for (auto flag : allFlags)
    {
        node->enable(flag);
    }

    // Clear all but one flag, the node stays invisible throughout
    for (auto flag = allFlags.begin(); flag != allFlags.end() - 1; ++flag)
    {
        node->visibilityMethodCalled = false;
        node->disable(*flag);

        EXPECT_FALSE(node->visible()) << "Node should stay invisible since not all flags are cleared yet.";
        EXPECT_FALSE(node->visibilityMethodCalled) << "No event should have been fired";
    }

    node->disable(allFlags.back());

    EXPECT_TRUE(node->visible()) << "Should be visible after clearing all flags";
    EXPECT_TRUE(node->visibilityMethodCalled) << "The event should have been fired";
    EXPECT_TRUE(node->passedVisibilityValue) << "Got the wrong visibility changed argument";
}

TEST_F(SceneNodeTest, ForcedVisibility)
{
    std::vector allFlags(allPossibleHideFlags);
    
    // Try all possible hide flags
    for (auto flag : allFlags)
    {
        auto node = std::make_shared<VisibilityTestNode>();
        scene::addNodeToContainer(node, GlobalMapModule().getRoot());

        node->enable(flag);
        EXPECT_FALSE(node->visible()) << "Should be invisible after setting a flag";
        
        // Set the forced visibility flag
        node->visibilityMethodCalled = false;
        node->setProtectedForcedVisibility(true);

        EXPECT_TRUE(node->visible()) << "Node is now forced visible";
        EXPECT_TRUE(node->visibilityMethodCalled) << "The event should have been fired";
        EXPECT_TRUE(node->passedVisibilityValue) << "Got the wrong visibility changed argument";

        // Set the forced visibility flag again (should do nothing)
        node->visibilityMethodCalled = false;
        node->setProtectedForcedVisibility(true);

        EXPECT_TRUE(node->visible()) << "Node is still forced visible";
        EXPECT_FALSE(node->visibilityMethodCalled) << "The event shouldn't have been fired";

        // Clear the forced visibility flag again
        node->visibilityMethodCalled = false;
        node->setProtectedForcedVisibility(false);

        EXPECT_FALSE(node->visible()) << "Node is no longer forced visible";
        EXPECT_TRUE(node->visibilityMethodCalled) << "The event should have been fired";
        EXPECT_FALSE(node->passedVisibilityValue) << "Got the wrong visibility changed argument";

        // Clear the forced visibility flag a second time (should do nothing)
        node->visibilityMethodCalled = false;
        node->setProtectedForcedVisibility(false);

        EXPECT_FALSE(node->visible()) << "Node is still not forced visible";
        EXPECT_FALSE(node->visibilityMethodCalled) << "The event shouldn't have been fired";

        scene::removeNodeFromParent(node);
    }
}

TEST_F(SceneNodeTest, SetFilterStatus)
{
    auto node = std::make_shared<VisibilityTestNode>();
    scene::addNodeToContainer(node, GlobalMapModule().getRoot());

    node->visibilityMethodCalled = false;
    node->setFiltered(true);

    EXPECT_TRUE(node->visibilityMethodCalled) << "Method should have been invoked";
    EXPECT_FALSE(node->visible()) << "Should be invisible after filtering it";
    EXPECT_TRUE(node->isFiltered()) << "Node should report as filtered";
    EXPECT_FALSE(node->passedVisibilityValue) << "Got the wrong visibility changed argument";

    // Set the filter status again
    node->visibilityMethodCalled = false;
    node->setFiltered(true);

    EXPECT_FALSE(node->visibilityMethodCalled) << "Method shouldn't have been invoked, no change";
    EXPECT_FALSE(node->visible()) << "Should still be invisible";
    EXPECT_TRUE(node->isFiltered()) << "Node should report as filtered";

    // Clear the filter status
    node->visibilityMethodCalled = false;
    node->setFiltered(false);

    EXPECT_TRUE(node->visibilityMethodCalled) << "Method should have been invoked";
    EXPECT_TRUE(node->visible()) << "Should be visible again after un-filtering it";
    EXPECT_FALSE(node->isFiltered()) << "Node should report as unfiltered";
    EXPECT_TRUE(node->passedVisibilityValue) << "Got the wrong visibility changed argument";

    // Clear the flag a second time
    node->visibilityMethodCalled = false;
    node->setFiltered(false);

    EXPECT_FALSE(node->visibilityMethodCalled) << "Method shouldn't have been invoked, no change";
    EXPECT_TRUE(node->visible()) << "Should still be visible";
    EXPECT_FALSE(node->isFiltered()) << "Node should report as unfiltered";
}

}
