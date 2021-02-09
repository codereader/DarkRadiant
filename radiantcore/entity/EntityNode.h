#pragma once

#include <sigc++/connection.h>
#include "ientity.h"
#include "inamespace.h"
#include "Bounded.h"

#include "scene/SelectableNode.h"
#include "transformlib.h"

#include "NamespaceManager.h"
#include "target/TargetableNode.h"
#include "NameKey.h"
#include "ColourKey.h"
#include "ModelKey.h"
#include "ShaderParms.h"

#include "KeyObserverMap.h"

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
	public Transformable
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

	// A helper class observing the "name" keyvalue
	// Used for rendering the name and as Nameable implementation
	NameKey _nameKey;

	// The OpenGLRenderable, using the NameKey helper class to retrieve the name
	RenderableNameKey _renderableName;

	// The keyobserver watching over the "_color" keyvalue
	ColourKey _colourKey;

	// Model child node handling helper
	ModelKey _modelKey;
	KeyObserverDelegate _modelKeyObserver;
	KeyObserverDelegate _skinKeyObserver;

	// A helper class managing the collection of KeyObservers attached to the SpawnArgs
	KeyObserverMap _keyObservers;

	// Helper class observing the "shaderParmNN" spawnargs and caching their values
	ShaderParms _shaderParms;

	// This entity's main direction, usually determined by the angle/rotation keys
	Vector3 _direction;

	// The wireframe / solid shaders as determined by the entityclass
	ShaderPtr _fillShader;
	ShaderPtr _wireShader;

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
    using AttachedEntities = std::list<IEntityNodePtr>;
    AttachedEntities _attachedEnts;

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
    void transformChanged() override;

	// RenderEntity implementation
	virtual float getShaderParm(int parmNum) const override;
	virtual const Vector3& getDirection() const override;

    // IMatrixTransform implementation
    const Matrix4& localToParent() const override { return _localToParent; }
    Matrix4& localToParent() override { return _localToParent; }

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
	virtual void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const override;
	virtual void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const override;
	virtual void setRenderSystem(const RenderSystemPtr& renderSystem) override;
	virtual std::size_t getHighlightFlags() override;

	// Adds/removes the keyobserver to/from the KeyObserverMap
	void addKeyObserver(const std::string& key, KeyObserver& observer);
	void removeKeyObserver(const std::string& key, KeyObserver& observer);

	ModelKey& getModelKey(); // needed by the Doom3Group class, could be a fixme
    const ModelKey& getModelKey() const;

	const ShaderPtr& getWireShader() const override;
	const ShaderPtr& getFillShader() const;

	virtual void onPostUndo() override;
	virtual void onPostRedo() override;

protected:
	virtual void onModelKeyChanged(const std::string& value);

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

    void acquireShaders();
    void acquireShaders(const RenderSystemPtr& renderSystem);

    // Create entity nodes for all attached entities
    void createAttachedEntities();

    // Render all attached entities
    template <typename RenderFunc> void renderAttachments(RenderFunc func) const
    {
        for (const IEntityNodePtr& ent: _attachedEnts)
        {
            // Attached entities might themselves have child nodes (e.g. func_static
            // which has its model as a child node), so we must traverse() the
            // attached entities, not just render them alone
            struct ChildRenderer : public scene::NodeVisitor
            {
                RenderFunc _func;
                ChildRenderer(RenderFunc f): _func(f)
                {}

                bool pre(const scene::INodePtr& node)
                {
                    _func(node);
                    return true;
                }
            };

            ChildRenderer cr(func);
            ent->traverse(cr);
        }
    }

    };

} // namespace entity
