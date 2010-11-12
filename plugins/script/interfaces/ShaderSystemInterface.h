#ifndef _SHADERSYSTEM_INTERFACE_H_
#define _SHADERSYSTEM_INTERFACE_H_

#include <boost/python.hpp>

#include "ishaders.h"
#include "iscript.h"

namespace script {

/**
 * This class represents a single Shader as seen by the Python script.
 */
class ScriptShader
{
	// The contained shader (can be NULL)
	MaterialPtr _shader;
public:
	ScriptShader(const MaterialPtr& shader) :
		_shader(shader)
	{}

	operator const MaterialPtr&() const {
		return _shader;
	}

	std::string getName() {
		return (_shader != NULL) ? _shader->getName() : "";
	}

	std::string getShaderFileName() {
		return (_shader != NULL) ? _shader->getShaderFileName() : "";
	}

	std::string getDescription() {
		return (_shader != NULL) ? _shader->getDescription() : "";
	}

	std::string getDefinition() {
		return (_shader != NULL) ? _shader->getDefinition() : "";
	}

	bool isVisible() {
		return (_shader != NULL) ? _shader->isVisible() : false;
	}

	bool isAmbientLight() {
		return (_shader != NULL) ? _shader->isAmbientLight() : false;
	}

	bool isBlendLight() {
		return (_shader != NULL) ? _shader->isBlendLight() : false;
	}

	bool isFogLight() {
		return (_shader != NULL) ? _shader->isFogLight() : false;
	}

	bool isNull() const {
		return _shader == NULL;
	}
};

// Wrap around the EntityClassVisitor interface
class ShaderVisitorWrapper :
	public shaders::ShaderVisitor,
	public boost::python::wrapper<shaders::ShaderVisitor>
{
public:
    void visit(const MaterialPtr& shader) {
		// Wrap this method to python
		this->get_override("visit")(ScriptShader(shader));
	}
};

/**
 * greebo: This class provides the script interface for the GlobalMaterialManager module.
 */
class ShaderSystemInterface :
	public IScriptInterface
{
public:
	void foreachShader(shaders::ShaderVisitor& visitor);
	ScriptShader getMaterialForName(const std::string& name);

	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace);
};
typedef boost::shared_ptr<ShaderSystemInterface> ShaderSystemInterfacePtr;

} // namespace script

#endif /* _SHADERSYSTEM_INTERFACE_H_ */
