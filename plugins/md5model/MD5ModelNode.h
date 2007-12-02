#ifndef MD5MODELNODE_H_
#define MD5MODELNODE_H_

#include "scenelib.h"
#include "instancelib.h"
#include "MD5Model.h"

namespace md5 {

class MD5ModelNode : 
	public scene::Node, 
	public scene::Instantiable,
	public Nameable
{
	InstanceSet _instances;
	MD5Model _model;
	
public:
	// returns the contained model
	MD5Model& model();

	// Instantiable implementation
	scene::Instance* create(const scene::Path& path, scene::Instance* parent);
	void forEachInstance(const scene::Instantiable::Visitor& visitor);
	void insert(const scene::Path& path, scene::Instance* instance);
	scene::Instance* erase(const scene::Path& path);

	// Nameable implementation
	virtual std::string name() const;
};
typedef boost::shared_ptr<MD5ModelNode> MD5ModelNodePtr;

} // namespace md5

#endif /*MD5MODELNODE_H_*/
