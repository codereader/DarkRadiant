#pragma once

#include "inamespace.h"
#include "modelskin.h"
#include "ientity.h"
#include "iselection.h"
#include "editable.h"

#include "scene/TraversableNodeSet.h"
#include "transformlib.h"
#include "selectionlib.h"
#include "render/RenderablePivot.h"
#include "../target/TargetableNode.h"
#include "../EntityNode.h"
#include "../KeyObserverDelegate.h"
#include "../RotationKey.h"
#include "../OriginKey.h"

namespace entity
{

class EclassModelNode;
typedef std::shared_ptr<EclassModelNode> EclassModelNodePtr;

/**
 * @brief Entity node displaying a model defined in an entityDef.
 *
 * This is used for entities whose model is defined in the entity class, e.g.
 * guards, interactable objects etc.
 */
class EclassModelNode :
	public EntityNode,
	public Snappable
{
private:
    OriginKey _originKey;
    Vector3 _origin;

    RotationKey _rotationKey;
	RotationMatrix _rotation;

    AngleKey _angleKey;
	float _angle;

    render::RenderablePivot _renderOrigin;
    ShaderPtr _pivotShader;

	AABB _localAABB;

    bool _noShadowsLit;

private:
	// Constructor
	EclassModelNode(const IEntityClassPtr& eclass);
	// Copy Constructor
	EclassModelNode(const EclassModelNode& other);

public:
	static EclassModelNodePtr Create(const IEntityClassPtr& eclass);

    virtual ~EclassModelNode();

	// Snappable implementation
	virtual void snapto(float snap) override;

	// Bounded implementation
	virtual const AABB& localAABB() const override;

	scene::INodePtr clone() const override;

	// Renderable implementation
    void onPreRender(const VolumeTest& volume) override;
	void setRenderSystem(const RenderSystemPtr& renderSystem) override;

    // Returns the original "origin" value
    const Vector3& getUntransformedOrigin() override;
    
    const Vector3& getWorldPosition() const override;

    virtual bool isShadowCasting() const override;

protected:
	// Gets called by the Transformable implementation whenever
	// scale, rotation or translation is changed.
    void _onTransformationChanged() override;

	// Called by the Transformable implementation before freezing
	// or when reverting transformations.
    void _applyTransformation() override;

	// Override EntityNode::construct()
	void construct() override;

    void onSelectionStatusChange(bool changeGroupStatus) override;

private:
    void translate(const Vector3& translation);
	void rotate(const Quaternion& rotation);

    // Thes two should not be confused with the methods inherited from the Transformable class
	void _revertTransform();
	void _freezeTransform();

    void updateTransform();

    void originChanged();
    void rotationChanged();
    void angleChanged();
    void onNoshadowsLitChanged(const std::string& value);
};

} // namespace
