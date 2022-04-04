#pragma once

#include "selectionlib.h"
#include "scene/Node.h"
#include "entitylib.h"

#include "../SpawnArgs.h"

#include "TargetKeyCollection.h"
#include "RenderableTargetLines.h"

namespace entity
{

class EntityNode;

class TargetLineNode;
typedef std::shared_ptr<TargetLineNode> TargetLineNodePtr;

/**
 * greebo: Each targetable entity (D3Group, Speaker, Lights, etc.) derives from
 *         this class.
 *
 * This registers itself with the contained SpawnArgs and observes its keys.
 * As soon as "name" keys are encountered, the TargetManager is notified about
 * the change, so that the name can be associated with a Target object.
 */
class TargetableNode :
	public Entity::Observer,
	public KeyObserver
{
	SpawnArgs& _d3entity;
	TargetKeyCollection _targetKeys;

	// The current name of this entity (used for comparison in "onKeyValueChanged")
	std::string _targetName;

	// The node we're associated with
	EntityNode& _node;

    // The targetmanager of the map we're in (is nullptr if not in the scene)
    ITargetManager* _targetManager;

    // The actual scene representation rendering the lines
    TargetLineNodePtr _targetLineNode;

public:
	TargetableNode(SpawnArgs& entity, EntityNode& node);

    // This might return nullptr if the node is not inserted in a scene
    ITargetManager* getTargetManager();

	// Connect this class with the SpawnArgs
	void construct();
	// Disconnect this class from the entity
	void destruct();

    TargetKeyCollection& getTargetKeys();

	// Gets called as soon as the "name" keyvalue changes
	void onKeyValueChanged(const std::string& name) override;

	// Entity::Observer implementation, gets called on key insert
	void onKeyInsert(const std::string& key, EntityKeyValue& value) override;
	void onKeyErase(const std::string& key, EntityKeyValue& value) override;
    void onKeyChange(const std::string& key, const std::string& value) override;

    // scene insert/remove handling
    void onInsertIntoScene(scene::IMapRootNode& root);
    void onRemoveFromScene(scene::IMapRootNode& root);

    void onVisibilityChanged(bool isVisibleNow);

    // Invoked by the TargetKeyCollection when the number of observed has changed
    void onTargetKeyCollectionChanged();

    void onTransformationChanged();
    void onRenderSystemChanged();
};

} // namespace entity
