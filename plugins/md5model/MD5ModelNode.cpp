#include "MD5ModelNode.h"

#include "MD5ModelInstance.h"

namespace md5 {

MD5Model& MD5ModelNode::model() {
	return _model;
}

scene::Instance* MD5ModelNode::create(const scene::Path& path, scene::Instance* parent) {
	return new MD5ModelInstance(path, parent, _model);
}

void MD5ModelNode::forEachInstance(const scene::Instantiable::Visitor& visitor) {
	_instances.forEachInstance(visitor);
}

void MD5ModelNode::insert(const scene::Path& path, scene::Instance* instance) {
	_instances.insert(path, instance);
}

scene::Instance* MD5ModelNode::erase(const scene::Path& path) {
	return _instances.erase(path);
}

std::string MD5ModelNode::name() const {
	return _model.getFilename();
}

} // namespace md5
