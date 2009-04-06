#ifndef SHADERUPDATEWALKER_H_
#define SHADERUPDATEWALKER_H_

#include "ishaders.h"
#include "ifilter.h"

namespace filters {

/**
 * Scenegraph walker to update filtered status of Instances based on the
 * status of their parent entity class.
 */
class ShaderUpdateWalker : 
	public shaders::ShaderVisitor
{
public:
	void visit(const MaterialPtr& shader) { 
		// Set the shader's visibility based on the current filter settings
		shader->setVisible(
			GlobalFilterSystem().isVisible("texture", shader->getName())
		);
	}
};

} // namespace filters

#endif /* SHADERUPDATEWALKER_H_ */
