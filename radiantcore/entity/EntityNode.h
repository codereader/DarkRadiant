#pragma once

#include <sigc++/connection.h>
#include "ientity.h"
#include "inamespace.h"
#include "icomparablenode.h"
#include "Bounded.h"

#include "scene/SelectableNode.h"
#include "transformlib.h"

#include "NamespaceManager.h"
#include "target/TargetableNode.h"
#include "NameKey.h"
#include "ColourKey.h"
#include "ModelKey.h"
#include "ShaderParms.h"
#include "OriginKey.h"

#include "KeyObserverMap.h"
#include "RenderableEntityName.h"
#include "RenderableObjectCollection.h"

namespace entity
{

class EntityNode;
typedef std::shared_ptr<EntityNode> EntityNodePtr;

/**
 * greebo: This is the common base class of all map entities.
 */
class EntityNode :
	public IEntityNode,
	public scene::SelectableNode, // derives from scene::Node
	public SelectionTestable,
	public Namespaced,
	public TargetableNode,
	public Transformable,
    public scene::IComparableNode
{
protected:
	// The entity class
	IEntityClassPtr _eclass;

	// The actual entity (which contains the key/value pairs)
	SpawnArgs _spawnArgs;

    // Transformation applied to this node and its children
    Matrix4 _localToParent = Matrix4::getIdentity();

	// The class taking care of all the namespace-relevant stuff
	NamespaceManager _namespaceManager;

    // Observes the "origin" keyvalue
    OriginKey _originKey;

    // Points to the origin value of this entity, is also kept up to date
    // during transformations. Used to render the entity name at the correct position.
    Vector3 _originTransformed;

	// A helper class observing the "name" keyvalue
	// Used for rendering the name and as Nameable implementation
	NameKey _nameKey;

	// The renderable, using the NameKey helper class to retrieve the name
	RenderableEntityName _renderableName;

	// The keyobserver watching over the "_color" keyvalue
	ColourKey _colourKey;

	// Model child node handling helper
	ModelKey _modelKey;

	// A helper class managing the collection of KeyObservers attached to the SpawnArgs
	KeyObserverMap _keyObservers;

	// Helper class observing the "shaderParmNN" spawnargs and caching their values
	ShaderParms _shaderParms;

	// This entity's main direction, usually determined by the angle/rotation keys
	Vector3 _direction;

	// The coloured shaders as determined by the entityclass

	ShaderPtr _fillShader; // cam only
	ShaderPtr _wireShader; // ortho only
	ShaderPtr _colourShader; // cam+ortho view
	ITextRenderer::Ptr _textRenderer; // for name rendering

	sigc::connection _eclassChangedConn;

    // List of attached sub-entities that we will submit for rendering (but are
    // otherwise non-interactable).
    //
    // Although scene::Node already has the ability to store children, this is a
    // separate list of entity nodes for two reasons: (1) there is a lot of
    // other code which walks the scene graph for various reasons (e.g. map
    // saving), and I don't want to have to audit the entire codebase to make
    // sure that everything will play nicely with entities as children of other
    // entities, and (2) storing entity node pointers instead of generic node
    // pointers avoids some extra dynamic_casting.
    using AttachedEntity = std::pair<IEntityNodePtr, Vector3 /* offset */>;
    using AttachedEntities = std::list<AttachedEntity>;
    AttachedEntities _attachedEnts;

    // Whether this entity has registered itself to a render system
    bool _isAttachedToRenderSystem;

    // The list of renderable objects attached to this IRenderEntity
    // Used in lighting render mode to enumerate surfaces by entity
    RenderableObjectCollection _renderObjects;

    bool _isShadowCasting;

protected:
	// The Constructor needs the eclass
	EntityNode(const IEntityClassPtr& eclass);

	// Copy constructor
	EntityNode(const EntityNode& other);

public:
	virtual ~EntityNode();

	// IEntityNode implementation
	Entity& getEntity() override;
	virtual void refreshModel() override;
    virtual void transformChanged() override;

	// RenderEntity implementation
    virtual std::string getEntityName() const override;
	virtual float getShaderParm(int parmNum) const override;
	virtual const Vector3& getDirection() const override;

    virtual void addRenderable(const render::IRenderableObject::Ptr& object, Shader* shader) override;
    virtual void removeRenderable(const render::IRenderableObject::Ptr& object) override;
    virtual void foreachRenderable(const ObjectVisitFunction& functor) override;
    virtual void foreachRenderableTouchingBounds(const AABB& bounds,
        const ObjectVisitFunction& functor) override;
    virtual bool isShadowCasting() const override;

    // IMatrixTransform implementation
    Matrix4 localToParent() const override { return _localToParent; }
    void setLocalToParent(const Matrix4& localToParent) override
    {
        _localToParent = localToParent;
    }

    // IComparableNode implementation
    std::string getFingerprint() override;

	// SelectionTestable implementation
	virtual void testSelect(Selector& selector, SelectionTest& test) override;

	// Namespaced implementation
	// Gets/sets the namespace of this named object
	std::string getName() const override;
	void setNamespace(INamespace* space) override;
	INamespace* getNamespace() const override;
	void connectNameObservers() override;
	void disconnectNameObservers() override;
	void changeName(const std::string& newName) override;

	void attachNames() override;
	void detachNames() override;

	virtual void onInsertIntoScene(scene::IMapRootNode& root) override;
	virtual void onRemoveFromScene(scene::IMapRootNode& root) override;

	// Sets/clears render entity references on child nodes
	virtual void onChildAdded(const scene::INodePtr& child) override;
	virtual void onChildRemoved(const scene::INodePtr& child) override;

	virtual std::string name() const override;
	Type getNodeType() const override;

	// Renderable implementation, can be overridden by subclasses
	virtual void onPreRender(const VolumeTest& volume) override;
	virtual void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume) override;
	virtual void setRenderSystem(const RenderSystemPtr& renderSystem) override;
	virtual std::size_t getHighlightFlags() override;

	// IEntityNode implementation
    void observeKey(const std::string& key, KeyObserverFunc func) override;
    void foreachAttachment(const std::function<void(const IEntityNodePtr&)>& functor) override;

	ModelKey& getModelKey(); // needed by the Doom3Group class, could be a fixme
    const ModelKey& getModelKey() const;

	const ShaderPtr& getWireShader() const override;
	const ShaderPtr& getColourShader() const override;
	const ShaderPtr& getFillShader() const;
    virtual Vector4 getEntityColour() const override;

	virtual void onPostUndo() override;
	virtual void onPostRedo() override;

    // Optional implementation: gets invoked by the EntityModule when the settings are changing
    virtual void onEntitySettingsChanged();

    void onVisibilityChanged(bool isVisibleNow) override;

    // Returns the current world origin of this entity (also up to date during transformations)
    virtual const Vector3& getWorldPosition() const = 0;

protected:
    virtual void onModelKeyChanged(const std::string& value);

    // Invoked when the colour key has changed its value
    virtual void onColourKeyChanged(const std::string& value)
    {}

	/**
	 * greebo: construct() does the necessary setup, connects keyobservers, etc.
	 * This is called by the static constructors - it seems awkward but is necessary
	 * since the std::shared_from_this() is not working when the object is not fully
	 * constructed yet.
	 * Subclasses must make sure to have this base method called if they override this.
	 */
	virtual void construct();

    // Called after cloning and construct to perform additional setup
    virtual void constructClone(const EntityNode& original);

	// Signal method to be overridden by subclasses.
	// Don't forget to call the base class implementation as this will
	// reload the entity key values and notify observers
	virtual void onEntityClassChanged();

private:
	// Routine used by the destructor, should be non-virtual
	void destruct();

	// Private function target - wraps to virtual protected signal
	void _modelKeyChanged(const std::string& value);
    void _originKeyChanged();
    void _colourKeyChanged(const std::string& value);
    void _onNoShadowsSettingsChanged(const std::string& value);

    void acquireShaders();
    void acquireShaders(const RenderSystemPtr& renderSystem);

    // Create entity nodes for all attached entities
    void createAttachedEntities();

    // Render all attached entities
    template <typename RenderFunc> void renderAttachments(const RenderFunc& func) const
    {
        for (auto& [entityNode, offset]: _attachedEnts)
        {
            // Before rendering the attached entity, ensure its offset is correct
            entityNode->setLocalToParent(Matrix4::getTranslation(offset));

            // Attached entities might themselves have child nodes (e.g. func_static
            // which has its model as a child node), so we must traverse() the
            // attached entities, not just render them alone
            struct ChildRenderer : public scene::NodeVisitor
            {
                RenderFunc _func;
                ChildRenderer(const RenderFunc& f): _func(f)
                {}

                bool pre(const scene::INodePtr& node)
                {
                    _func(node);
                    return true;
                }
            };
            ChildRenderer cr(func);
            entityNode->traverse(cr);
        }
    }

    void attachToRenderSystem();
    void detachFromRenderSystem();
};

} // namespace entity
