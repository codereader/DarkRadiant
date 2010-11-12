#ifndef _BRUSH_INTERFACE_H_
#define _BRUSH_INTERFACE_H_

#include <boost/python.hpp>
#include "iscript.h"

#include "SceneGraphInterface.h"

namespace script {

class BrushInterface :
	public IScriptInterface
{
public:
	ScriptSceneNode createBrush();

	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace);
};
typedef boost::shared_ptr<BrushInterface> BrushInterfacePtr;

} // namespace script

#endif /* _BRUSH_INTERFACE_H_ */
