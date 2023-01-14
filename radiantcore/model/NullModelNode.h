#pragma once

#include "Bounded.h"
#include "iselectiontest.h"
#include "irenderable.h"

#include "NullModel.h"
#include "IndexedBoxSurface.h"
#include "ModelNodeBase.h"

namespace model
{

class NullModelNode final :
    public ModelNodeBase,
    public SelectionTestable,
    public ModelNode
{
private:
    NullModelPtr _nullModel;
    IndexedBoxSurface _boxSurface;

public:
    // Default constructor, allocates a new NullModel
    NullModelNode();

    // Alternative constructor, uses the given nullModel
    NullModelNode(const NullModelPtr& nullModel);

    std::string name() const override;

    void onInsertIntoScene(scene::IMapRootNode& root) override;

    const IModel& getIModel() const override;
    IModel& getIModel() override;
    bool hasModifiedScale() override;
    Vector3 getModelScale() override;

    void testSelect(Selector& selector, SelectionTest& test) override;

	void setRenderSystem(const RenderSystemPtr& renderSystem) override;

	// Bounded implementation
	const AABB& localAABB() const override;

protected:
    void createRenderableSurfaces() override;
};

} // namespace model
