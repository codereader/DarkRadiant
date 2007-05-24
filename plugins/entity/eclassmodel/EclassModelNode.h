#ifndef ECLASSMODELNODE_H_
#define ECLASSMODELNODE_H_

#include "nameable.h"
#include "inamespace.h"
#include "modelskin.h"
#include "ientity.h"

#include "scenelib.h"
#include "transformlib.h"
#include "instancelib.h"

#include "EclassModel.h"

namespace entity {

class EclassModelNode :
	public scene::Node,
	public scene::Instantiable,
	public scene::Cloneable,
	public scene::Traversable::Observer,
	public Nameable,
	public Snappable,
	public TransformNode,
	public scene::Traversable,
	public EntityNode,
	public Namespaced,
	public ModelSkin
{
	InstanceSet m_instances;
	EclassModel m_contained;

public:
	// Constructor
	EclassModelNode(IEntityClassPtr eclass);
	// Copy Constructor
	EclassModelNode(const EclassModelNode& other);

	~EclassModelNode();

	// scene::Traversable Implementation
	virtual void insert(Node& node);
	virtual void erase(Node& node);
	virtual void traverse(const Walker& walker);
	virtual bool empty() const;

	// Snappable implementation
	virtual void snapto(float snap);

	// TransformNode implementation
	virtual const Matrix4& localToParent() const;

	// EntityNode implementation
	virtual Entity& getEntity();

	// Namespaced implementation
	virtual void setNamespace(Namespace& space);

	virtual void attach(ModuleObserver& observer);
	virtual void detach(ModuleObserver& observer);

	virtual std::string getRemap(const std::string& name) const;

	// scene::Traversable::Observer implementation
	void insertChild(scene::Node& child);
	void eraseChild(scene::Node& child);

	scene::Node& clone() const;

	scene::Instance* create(const scene::Path& path, scene::Instance* parent);
	void forEachInstance(const scene::Instantiable::Visitor& visitor);
	void insert(scene::Instantiable::Observer* observer, const scene::Path& path, scene::Instance* instance);
	scene::Instance* erase(scene::Instantiable::Observer* observer, const scene::Path& path);

	// Nameable implementation
	virtual std::string name() const;
	virtual void attach(const NameCallback& callback);
	virtual void detach(const NameCallback& callback);

private:
	void construct();
	void destroy();
};

} // namespace entity

#endif /*ECLASSMODELNODE_H_*/
