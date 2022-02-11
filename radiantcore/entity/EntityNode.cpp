#include "EntityNode.h"

#include "i18n.h"
#include "itextstream.h"
#include "icounter.h"
#include "imodel.h"
#include "imap.h"
#include "itransformable.h"
#include "math/Hash.h"
#include "string/case_conv.h"

#include "EntitySettings.h"

namespace entity
{

EntityNode::EntityNode(const IEntityClassPtr& eclass) :
	TargetableNode(_spawnArgs, *this),
	_eclass(eclass),
	_spawnArgs(_eclass),
	_namespaceManager(_spawnArgs),
    _originKey(std::bind(&EntityNode::_originKeyChanged, this)),
	_nameKey(_spawnArgs),
	_renderableName(*this, _nameKey),
	_modelKey(*this),
	_keyObservers(_spawnArgs),
	_shaderParms(_keyObservers, _colourKey),
	_direction(1,0,0),
    _isAttachedToRenderSystem(false)
{
}

EntityNode::EntityNode(const EntityNode& other) :
	IEntityNode(other),
	SelectableNode(other),
	SelectionTestable(other),
	Namespaced(other),
	TargetableNode(_spawnArgs, *this),
	Transformable(other),
	_eclass(other._eclass),
	_spawnArgs(other._spawnArgs),
    _localToParent(other._localToParent),
	_namespaceManager(_spawnArgs),
    _originKey(std::bind(&EntityNode::_originKeyChanged, this)),
	_nameKey(_spawnArgs),
	_renderableName(*this, _nameKey),
	_modelKey(*this),
	_keyObservers(_spawnArgs),
	_shaderParms(_keyObservers, _colourKey),
	_direction(1,0,0),
    _isAttachedToRenderSystem(false)
{
}

EntityNode::~EntityNode()
{
	destruct();
}

void EntityNode::construct()
{
	_eclassChangedConn = _eclass->changedSignal().connect([this]()
	{
		this->onEntityClassChanged();
	});

	TargetableNode::construct();

    // Observe basic keys
    static_assert(std::is_base_of_v<sigc::trackable, NameKey>);
    static_assert(std::is_base_of_v<sigc::trackable, ColourKey>);
    static_assert(std::is_base_of_v<sigc::trackable, OriginKey>);
    observeKey("origin", sigc::mem_fun(_originKey, &OriginKey::onKeyValueChanged));
	observeKey("name", sigc::mem_fun(_nameKey, &NameKey::onKeyValueChanged));
	observeKey("_color", sigc::mem_fun(_colourKey, &ColourKey::onKeyValueChanged));

    // Observe model-related keys
    static_assert(std::is_base_of_v<sigc::trackable, EntityNode>);
    static_assert(std::is_base_of_v<sigc::trackable, ModelKey>);
	observeKey("model", sigc::mem_fun(this, &EntityNode::_modelKeyChanged));
	observeKey("skin", sigc::mem_fun(_modelKey, &ModelKey::skinChanged));

	_shaderParms.addKeyObservers();

    // Construct all attached entities
    createAttachedEntities();
}

void EntityNode::constructClone(const EntityNode& original)
{
    // We just got cloned, it's possible that this node is the parent of a scaled model node
    auto originalChildModel = original.getModelKey().getNode();

    if (originalChildModel)
    {
        model::ModelNodePtr originalModel = Node_getModel(originalChildModel);

        // Check if the original model node is scaled
        if (originalModel && originalModel->hasModifiedScale())
        {
            assert(getModelKey().getNode()); // clone should have a child model like the original
            auto transformable = scene::node_cast<ITransformable>(getModelKey().getNode());

            if (transformable)
            {
                transformable->setType(TRANSFORM_PRIMITIVE);
                transformable->setScale(originalModel->getModelScale());
                transformable->freezeTransform();
            }
        }
    }
}

void EntityNode::destruct()
{
	_modelKey.setActive(false); // disable callbacks during destruction

	_eclassChangedConn.disconnect();

	TargetableNode::destruct();
}

void EntityNode::createAttachedEntities()
{
    _spawnArgs.forEachAttachment(
        [this](const Entity::Attachment& a)
        {
            // Since we can't yet handle joint positions, ignore this attachment
            // if it is attached to a joint
            if (!a.joint.empty())
                return;

            // Check this is a valid entity class
            auto cls = GlobalEntityClassManager().findClass(a.eclass);
            if (!cls)
            {
                rWarning() << "EntityNode [" << _eclass->getName()
                           << "]: cannot attach non-existent entity class '"
                           << a.eclass << "'\n";
                return;
            }

            // Construct and store the attached entity
            auto attachedEnt = GlobalEntityModule().createEntity(cls);
            assert(attachedEnt);
            _attachedEnts.push_back(attachedEnt);

            // Set ourselves as the parent of the attached entity (for
            // localToParent transforms)
            attachedEnt->setParent(shared_from_this());

            // Set the attached entity's transform matrix according to the
            // required offset
            attachedEnt->localToParent() = Matrix4::getTranslation(a.offset);
        }
    );
}

void EntityNode::transformChanged()
{
    Node::transformChanged();

    // Broadcast transformChanged to all attached entities so they can update
    // their position
    for (auto attached: _attachedEnts)
        attached->transformChanged();
}

void EntityNode::onEntityClassChanged()
{
	// By default, we notify the KeyObservers attached to this entity
	_keyObservers.refreshObservers();

    // The colour might have changed too, so re-acquire the shaders if possible
    acquireShaders();
}

void EntityNode::observeKey(const std::string& key, KeyObserverFunc func)
{
    _keyObservers.observeKey(key, func);
}

Entity& EntityNode::getEntity()
{
	return _spawnArgs;
}

void EntityNode::refreshModel()
{
	_modelKey.refreshModel();
}

std::string EntityNode::getEntityName() const
{
    return getName();
}

float EntityNode::getShaderParm(int parmNum) const
{
	return _shaderParms.getParmValue(parmNum);
}

const Vector3& EntityNode::getDirection() const
{
	return _direction;
}

void EntityNode::addRenderable(const render::IRenderableObject::Ptr& object, Shader* shader)
{
    _renderObjects.addRenderable(object, shader);
}

void EntityNode::removeRenderable(const render::IRenderableObject::Ptr& object)
{
    _renderObjects.removeRenderable(object);
}

void EntityNode::foreachRenderableTouchingBounds(const AABB& bounds,
    const IRenderEntity::ObjectVisitFunction& functor)
{
    _renderObjects.foreachRenderableTouchingBounds(bounds, functor);
}

std::string EntityNode::getFingerprint()
{
    std::map<std::string, std::string> sortedKeyValues;

    // Entities are just a collection of key/value pairs,
    // use them in lower case form, ignore inherited keys, sort before hashing
    _spawnArgs.forEachKeyValue([&](const std::string& key, const std::string& value)
    {
        sortedKeyValues.emplace(string::to_lower_copy(key), string::to_lower_copy(value));
    }, false);

    math::Hash hash;

    for (const auto& pair : sortedKeyValues)
    {
        hash.addString(pair.first);
        hash.addString(pair.second);
    }

    // Entities need to include any child hashes, but be insensitive to their order
    std::set<std::string> childFingerprints;

    foreachNode([&](const scene::INodePtr& child)
    {
        auto comparable = std::dynamic_pointer_cast<scene::IComparableNode>(child);

        if (comparable)
        {
            childFingerprints.insert(comparable->getFingerprint());
        }

        return true;
    });

    for (auto childFingerprint : childFingerprints)
    {
        hash.addString(childFingerprint);
    }

    return hash;
}

void EntityNode::testSelect(Selector& selector, SelectionTest& test)
{
	test.BeginMesh(localToWorld());

	// Pass the call down to the model node, if applicable
	SelectionTestablePtr selectionTestable = Node_getSelectionTestable(_modelKey.getNode());

    if (selectionTestable)
	{
		selectionTestable->testSelect(selector, test);
    }
}

std::string EntityNode::getName() const {
	return _namespaceManager.getName();
}

void EntityNode::setNamespace(INamespace* space) {
	_namespaceManager.setNamespace(space);
}

INamespace* EntityNode::getNamespace() const {
	return _namespaceManager.getNamespace();
}

void EntityNode::connectNameObservers() {
	_namespaceManager.connectNameObservers();
}

void EntityNode::disconnectNameObservers() {
	_namespaceManager.disconnectNameObservers();
}

void EntityNode::attachNames() {
	_namespaceManager.attachNames();
}

void EntityNode::detachNames() {
	_namespaceManager.detachNames();
}

void EntityNode::changeName(const std::string& newName) {
	_namespaceManager.changeName(newName);
}

void EntityNode::onInsertIntoScene(scene::IMapRootNode& root)
{
    GlobalCounters().getCounter(counterEntities).increment();

	_spawnArgs.connectUndoSystem(root.getUndoSystem());
	_modelKey.connectUndoSystem(root.getUndoSystem());

    attachToRenderSystem();

	SelectableNode::onInsertIntoScene(root);
    TargetableNode::onInsertIntoScene(root);
}

void EntityNode::onRemoveFromScene(scene::IMapRootNode& root)
{
    TargetableNode::onRemoveFromScene(root);
	SelectableNode::onRemoveFromScene(root);

    detachFromRenderSystem();

	_modelKey.disconnectUndoSystem(root.getUndoSystem());
	_spawnArgs.disconnectUndoSystem(root.getUndoSystem());

    GlobalCounters().getCounter(counterEntities).decrement();
}

void EntityNode::onChildAdded(const scene::INodePtr& child)
{
	// Let the child know which renderEntity it has - this has to happen before onChildAdded()
	child->setRenderEntity(this);

	Node::onChildAdded(child);
}

void EntityNode::onChildRemoved(const scene::INodePtr& child)
{
	Node::onChildRemoved(child);

	// Leave the renderEntity on the child until this point - this has to happen after onChildRemoved()

	// greebo: Double-check that we're the currently assigned renderentity - in some cases nodes on the undostack
	// keep references to child nodes - we should never NULLify renderentities of nodes that are not assigned to us
	IRenderEntity* curRenderEntity = child->getRenderEntity();

	if (curRenderEntity && curRenderEntity == this)
	{
		child->setRenderEntity(nullptr);
	}
	else
	{
		rWarning() << "[EntityNode] the child being removed is already assigned to a different render entity." << std::endl;
	}
}

std::string EntityNode::name() const
{
	return _nameKey.getName();
}

scene::INode::Type EntityNode::getNodeType() const
{
	return Type::Entity;
}

void EntityNode::onPreRender(const VolumeTest& volume)
{
    if (EntitySettings::InstancePtr()->getRenderEntityNames())
    {
        _renderableName.update(_textRenderer);
    }
    else
    {
        _renderableName.clear();
    }

    // Render any attached entities
    renderAttachments(
        [&](const scene::INodePtr& n) { n->onPreRender(volume); }
    );
}

void EntityNode::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
    renderAttachments(
        [&](const scene::INodePtr& n) { n->renderHighlights(collector, volume); }
    );
}

void EntityNode::acquireShaders()
{
    acquireShaders(getRenderSystem());
}

void EntityNode::acquireShaders(const RenderSystemPtr& renderSystem)
{
    if (renderSystem)
    {
        const auto& colour = _spawnArgs.getEntityClass()->getColour();
        _fillShader = renderSystem->capture(ColourShaderType::CameraSolid, colour);
        _wireShader = renderSystem->capture(ColourShaderType::OrthoviewSolid, colour);
        _colourShader = renderSystem->capture(ColourShaderType::CameraAndOrthoview, colour);
        _textRenderer = renderSystem->captureTextRenderer(IGLFont::Style::Sans, 14);
    }
    else
    {
        _fillShader.reset();
        _wireShader.reset();
        _colourShader.reset();
        _textRenderer.reset();
    }
}

void EntityNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
    // Detach from any existing render system first
    detachFromRenderSystem();

	SelectableNode::setRenderSystem(renderSystem);

    if (renderSystem)
    {
        attachToRenderSystem();
    }

    acquireShaders(renderSystem);

	// The colour key is maintaining a shader object as well
	_colourKey.setRenderSystem(renderSystem);

    // Make sure any attached entities have a render system too
    for (IEntityNodePtr node: _attachedEnts)
        node->setRenderSystem(renderSystem);

	TargetableNode::onRenderSystemChanged();
}

std::size_t EntityNode::getHighlightFlags()
{
	if (!isSelected()) return Highlight::NoHighlight;

	return isGroupMember() ? (Highlight::Selected | Highlight::GroupMember) : Highlight::Selected;
}

ModelKey& EntityNode::getModelKey()
{
	return _modelKey;
}

const ModelKey& EntityNode::getModelKey() const
{
    return _modelKey;
}

void EntityNode::onModelKeyChanged(const std::string& value)
{
	// Default implementation suitable for Light, Generic and EClassModel
	// Dispatch the call to the key observer, which will create the child node
	_modelKey.modelChanged(value);
}

void EntityNode::_modelKeyChanged(const std::string& value)
{
	// Wrap the call to the virtual event
	onModelKeyChanged(value);
}

void EntityNode::_originKeyChanged()
{
    // TODO: add virtual callout for subclasses
}

const ShaderPtr& EntityNode::getWireShader() const
{
	return _wireShader;
}

const ShaderPtr& EntityNode::getColourShader() const
{
	return _colourShader;
}

const ShaderPtr& EntityNode::getFillShader() const
{
	return _fillShader;
}

Vector4 EntityNode::getEntityColour() const
{
    return _spawnArgs.getEntityClass()->getColour();
}

void EntityNode::onPostUndo()
{
	// After undo operations there might remain some child nodes
	// without renderentity, rectify that
	foreachNode([&] (const scene::INodePtr& child)->bool
	{
		child->setRenderEntity(this);
		return true;
	});
}

void EntityNode::onPostRedo()
{
	// After redo operations there might remain some child nodes
	// without renderentity, rectify that
	foreachNode([&] (const scene::INodePtr& child)->bool
	{
		child->setRenderEntity(this);
		return true;
	});
}

void EntityNode::onEntitySettingsChanged()
{
    if (!EntitySettings::InstancePtr()->getRenderEntityNames())
    {
        _renderableName.clear();
    }
}

void EntityNode::attachToRenderSystem()
{
    if (_isAttachedToRenderSystem) return;

    auto renderSystem = getRenderSystem();

    if (renderSystem)
    {
        renderSystem->addEntity(std::dynamic_pointer_cast<IRenderEntity>(shared_from_this()));
        _isAttachedToRenderSystem = true;
    }
}

void EntityNode::detachFromRenderSystem()
{
    if (!_isAttachedToRenderSystem) return;

    auto renderSystem = getRenderSystem();

    if (renderSystem)
    {
        renderSystem->removeEntity(std::dynamic_pointer_cast<IRenderEntity>(shared_from_this()));
        _isAttachedToRenderSystem = false;
    }
}

} // namespace entity
