#ifndef _PATCH_INTERFACE_H_
#define _PATCH_INTERFACE_H_

#include <boost/python.hpp>
#include "iscript.h"

#include "SceneGraphInterface.h"

namespace script {

class PatchInterface :
	public IScriptInterface
{
public:
	ScriptSceneNode createPatchDef2();
	ScriptSceneNode createPatchDef3();

	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace);
};
typedef boost::shared_ptr<PatchInterface> PatchInterfacePtr;

} // namespace script

#endif /* _PATCH_INTERFACE_H_ */
