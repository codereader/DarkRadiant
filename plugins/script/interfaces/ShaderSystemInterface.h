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
	// The contained shader
	IShaderPtr _shader;
public:
	ScriptShader(const IShaderPtr& shader) :
		_shader(shader)
	{}

	operator const IShaderPtr&() const {
		return _shader;
	}

	std::string getName() {
		return (_shader != NULL) ? _shader->getName() : "";
	}

	std::string getShaderFileName() {
		return (_shader != NULL) ? _shader->getShaderFileName() : "";
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
    void visit(const IShaderPtr& shader) {
		// Wrap this method to python
		this->get_override("visit")(ScriptShader(shader));
	}
};

/**
 * greebo: This class provides the script interface for the GlobalShaderSystem module.
 */
class ShaderSystemInterface :
	public IScriptInterface
{
public:
	void foreachShader(shaders::ShaderVisitor& visitor);

	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace);
};
typedef boost::shared_ptr<ShaderSystemInterface> ShaderSystemInterfacePtr;

} // namespace script

#endif /* _SHADERSYSTEM_INTERFACE_H_ */
