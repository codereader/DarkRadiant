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
	public Transformable,
	public MatrixTransform,	// influences local2world of child nodes
	public scene::Cloneable // all entities are cloneable, to be implemented in subclasses
{
protected:
	// The entity class
	IEntityClassPtr _eclass;

	// The actual entity (which contains the key/value pairs)
	// TODO: Rename this to "spawnargs"?
	Doom3Entity _entity;

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

	// A helper class managing the collection of KeyObservers attached to the Doom3Entity
	KeyObserverMap _keyObservers;

	// Helper class observing the "shaderParmNN" spawnargs and caching their values
	ShaderParms _shaderParms;

	// This entity's main direction, usually determined by the angle/rotation keys
	Vector3 _direction;

	// The wireframe / solid shaders as determined by the entityclass
	ShaderPtr _fillShader;
	ShaderPtr _wireShader;

	sigc::connection _eclassChangedConn;

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

	// RenderEntity implementation
	virtual float getShaderParm(int parmNum) const override;
	virtual const Vector3& getDirection() const override;

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

	// Returns the colour as defined in the _color spawnarg
	const Vector3& getColour() const;
	const ShaderPtr& getColourShader() const;

	ModelKey& getModelKey(); // needed by the Doom3Group class, could be a fixme

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

	// Signal method to be overridden by subclasses.
	// Don't forget to call the base class implementation as this will
	// reload the entity key values and notify observers
	virtual void onEntityClassChanged();

private:
	// Routine used by the destructor, should be non-virtual
	void destruct();

	// Private function target - wraps to virtual protected signal
	void _modelKeyChanged(const std::string& value);
};

} // namespace entity
