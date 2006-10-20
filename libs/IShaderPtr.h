#ifndef ISHADERPTR_H_
#define ISHADERPTR_H_

#include "ishaders.h"

namespace shaders
{

/** Reference to an IShader object. This reference object takes ownership
 * of the given IShader* and calls IShader::DecRef on destruction.
 */
 
class IShaderPtr {

	// Owned IShader*
	IShader* _shader;
	
public:

	/** Construct an IShaderPtr owning the given IShader*. It is
	 * assumed that the IShader's reference count is automatically
	 * incremented on creation, thus there is no call to IncRef().
	 */
	IShaderPtr(IShader* ptr)
	: _shader(ptr) {}
	
	/** Destructor automatically calls DecRef on the owned IShader.
	 */
	~IShaderPtr() {
		_shader->DecRef();
	}
	
	/** Dereference operators. Allow access to the owned IShader 
	 * object.
	 */
	IShader* operator* () {
		return _shader;	
	}
	
	IShader* operator-> () {
		return _shader;
	}
	
}; 


}

#endif /*ISHADERPTR_H_*/
