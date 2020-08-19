#include "MapObserver.h"
#include "inode.h"
#include "ientity.h"


namespace gameconn {

static std::vector<IEntityNodePtr> getEntitiesInNode(const scene::INodePtr &node) {
    struct MyVisitor : scene::NodeVisitor {
        std::set<IEntityNodePtr> v;
        virtual bool pre(const scene::INodePtr& node) override {
            if (IEntityNodePtr ptr = std::dynamic_pointer_cast<IEntityNode, scene::INode>(node)) {
                v.insert(ptr);
                return false;
            }
            return true;
        }
    };
    MyVisitor visitor;
    node->traverse(visitor);
    return std::vector<IEntityNodePtr>(visitor.v.begin(), visitor.v.end());
}

class MapObserver_EntityObserver : public Entity::Observer {
    MapObserver &_owner;
    std::string _entityName;
    bool _enabled = false;

public:
    MapObserver_EntityObserver(MapObserver &owner) : _owner(owner) {}
    void enable() {
        _enabled = true;
    }
    virtual void onKeyInsert(const std::string& key, EntityKeyValue& value) override {
        if (key == "name")
            _entityName = value.get();      //happens when installing observer
        if (_enabled)
            _owner.entityUpdated(_entityName, 0);
    }
    virtual void onKeyChange(const std::string& key, const std::string& val) override {
        if (_enabled) {
            if (key == "name") {
                //renaming is equivalent to deleting old entity and adding new
                _owner.entityUpdated(_entityName, -1);
                _owner.entityUpdated(val, 1);
            }
            else {
                _owner.entityUpdated(_entityName, 0);
            }
        }
    }
    virtual void onKeyErase(const std::string& key, EntityKeyValue& value) override {
        if (_enabled)
            _owner.entityUpdated(_entityName, 0);
    }
};

class MapObserver_SceneObserver : public scene::Graph::Observer {
    MapObserver &_owner;

public:
    MapObserver_SceneObserver(MapObserver &owner) : _owner(owner) {}
    virtual void onSceneNodeInsert(const scene::INodePtr& node) override {
        auto entityNodes = getEntitiesInNode(node);
        for (const IEntityNodePtr &entNode : entityNodes)
            _owner.entityUpdated(entNode->name(), 1);
        _owner.setEntityObservers(entityNodes, true);
    }
    virtual void onSceneNodeErase(const scene::INodePtr& node) override {
        auto entityNodes = getEntitiesInNode(node);
        _owner.setEntityObservers(entityNodes, false);
        for (const IEntityNodePtr &entNode : entityNodes)
            _owner.entityUpdated(entNode->name(), -1);
    }
};

void MapObserver::setEntityObservers(const std::vector<IEntityNodePtr> &entityNodes, bool enable) {
    if (enable) {
        for (auto entNode : entityNodes) {
            if (_entityObservers.count(entNode.get()))
                continue;   //already tracked
            MapObserver_EntityObserver *observer = new MapObserver_EntityObserver(*this);
            entNode->getEntity().attachObserver(observer);
            _entityObservers[entNode.get()] = observer;
            observer->enable();
        }
    }
    else {
        for (auto entNode : entityNodes) {
            if (!_entityObservers.count(entNode.get()))
                continue;   //not tracked
            Entity::Observer* observer = _entityObservers[entNode.get()];
            entNode->getEntity().detachObserver(observer);
            delete observer;
            _entityObservers.erase(entNode.get());
        }
    }
}

void MapObserver::setEnabled(bool enable) {
    auto entityNodes = getEntitiesInNode(GlobalSceneGraph().root());
    if (enable) {
        setEntityObservers(entityNodes, true);
        if (!_sceneObserver)
            _sceneObserver.reset(new MapObserver_SceneObserver(*this));
        GlobalSceneGraph().addSceneObserver(_sceneObserver.get());
    }
    else {
        if (_sceneObserver) {
            GlobalSceneGraph().removeSceneObserver(_sceneObserver.get());
            _sceneObserver.reset();
        }
        setEntityObservers(entityNodes, false);
        assert(_entityObservers.empty());
        _entityChanges.clear();
    }
}

bool MapObserver::isEnabled() const {
    return _sceneObserver != nullptr;
}

void MapObserver::clear() {
    _entityChanges.clear();
}

MapObserver::~MapObserver() {
    setEnabled(false);
}

void MapObserver::entityUpdated(const std::string &name, int type) {
    int &status = _entityChanges[name];
    status += type;
    if (std::abs(status) > 1) {
        //added an already added entity, or removed an already removed one: should not happen
        assert(false);
        status = (status < 0 ? -1 : 1);
    }
}

const MapObserver::ChangeSet &MapObserver::getChanges() const {
    return _entityChanges;
}

}
