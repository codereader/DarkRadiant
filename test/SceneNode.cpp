#include "RadiantTest.h"

#include "scene/BasicRootNode.h"
#include "scene/Node.h"
#include "scenelib.h"
#include "algorithm/Entity.h"

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

// Custom Node class to test the protected onRenderStateChanged method behaviour
class RenderStateTestNode :
    public scene::Node
{
public:
    RenderStateTestNode() :
        changedMethodCalled(false)
    {}

    // Default copy ctor for testing
    RenderStateTestNode(const RenderStateTestNode& other) = default;

    Type getNodeType() const override
    {
        return Type::Unknown;
    }

    const AABB& localAABB() const
    {
        static AABB dummy;
        return dummy;
    }

    void onPreRender(const VolumeTest& volume) override {}
    void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume) override {}
    std::size_t getHighlightFlags() override
    {
        return 0;
    }

    // Public test fields
    bool changedMethodCalled = false;

protected:
    void onRenderStateChanged() override
    {
        Node::onRenderStateChanged();

        changedMethodCalled = true;
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

TEST_F(SceneNodeTest, InitialRenderState)
{
    auto node = std::make_shared<RenderStateTestNode>();
    EXPECT_EQ(node->getRenderState(), scene::INode::RenderState::Active) << "Should be active after construction";
    EXPECT_FALSE(node->changedMethodCalled) << "Changed method should not have been called";
}

TEST_F(SceneNodeTest, RenderStateOfCopiedNode)
{
    auto node = std::make_shared<RenderStateTestNode>();
    auto copy = std::make_shared<RenderStateTestNode>(*node);

    EXPECT_EQ(copy->getRenderState(), scene::INode::RenderState::Active) << "Copied node should inherit the initial value";

    node->setRenderState(scene::INode::RenderState::Inactive);
    copy = std::make_shared<RenderStateTestNode>(*node);
    EXPECT_EQ(copy->getRenderState(), scene::INode::RenderState::Inactive) << "Copied node should inherit the value";
}

TEST_F(SceneNodeTest, OnRenderStateChanged)
{
    auto node = std::make_shared<RenderStateTestNode>();
    EXPECT_FALSE(node->changedMethodCalled) << "Changed method should not have been called";

    // Setting to active again shouldn't do anything
    node->setRenderState(scene::INode::RenderState::Active);
    EXPECT_FALSE(node->changedMethodCalled) << "Changed method should not have been called";

    // To Inactive
    node->setRenderState(scene::INode::RenderState::Inactive);
    EXPECT_TRUE(node->changedMethodCalled) << "Changed method should have been called";

    // To inactive a second time
    node->changedMethodCalled = false;
    node->setRenderState(scene::INode::RenderState::Inactive);
    EXPECT_FALSE(node->changedMethodCalled) << "Changed method should not have been called";

    // Back to active
    node->changedMethodCalled = false;
    node->setRenderState(scene::INode::RenderState::Active);
    EXPECT_TRUE(node->changedMethodCalled) << "Changed method should not have been called";
}

TEST_F(SceneNodeTest, SetRenderStateWorksNonrecursively)
{
    auto node = std::make_shared<RenderStateTestNode>();
    auto child = std::make_shared<RenderStateTestNode>();
    scene::addNodeToContainer(child, node);

    EXPECT_EQ(node->getRenderState(), scene::INode::RenderState::Active) << "Should be active after construction";
    EXPECT_EQ(child->getRenderState(), scene::INode::RenderState::Active) << "Should be active after construction";

    node->setRenderState(scene::INode::RenderState::Inactive);
    EXPECT_EQ(node->getRenderState(), scene::INode::RenderState::Inactive) << "Parent node should have been changed";
    EXPECT_EQ(child->getRenderState(), scene::INode::RenderState::Active) << "setRenderState should not affect children";
}

TEST_F(SceneNodeTest, SetRenderStateAffectsAttachments)
{
    auto torch = algorithm::createEntityByClassName("atdm:torch_brazier");
    scene::addNodeToContainer(torch, GlobalMapModule().getRoot());

    bool hasAttachments = false;
    // All attachments should be there and active
    torch->foreachAttachment([&](const IEntityNodePtr& attachment)
    {
        hasAttachments = true;
        EXPECT_EQ(attachment->getRenderState(), scene::INode::RenderState::Active) << "Should be active after construction";
    });
    EXPECT_TRUE(hasAttachments) << "Unit test setup is wrong, node should have attachments";

    // Set the state of the host entity to inactive
    torch->setRenderState(scene::INode::RenderState::Inactive);
    torch->foreachAttachment([&](const IEntityNodePtr& attachment)
    {
        EXPECT_EQ(attachment->getRenderState(), scene::INode::RenderState::Inactive) << "Attachments not set to inactive";
    });

    // Back to active
    torch->setRenderState(scene::INode::RenderState::Active);
    torch->foreachAttachment([&](const IEntityNodePtr& attachment)
    {
        EXPECT_EQ(attachment->getRenderState(), scene::INode::RenderState::Active) << "Attachments not set to active";
    });
}

}
