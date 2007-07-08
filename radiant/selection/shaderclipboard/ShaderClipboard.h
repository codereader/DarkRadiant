#ifndef SHADERCLIPBOARD_H_
#define SHADERCLIPBOARD_H_

#include "Texturable.h"

namespace selection {

class ShaderClipboard
{
	// The source and target Texturables
	Texturable _source;

	bool _updatesDisabled;

public:
	ShaderClipboard();

	/** greebo: Tries to find the best source Texturable object
	 * 			with the given SelectionTest (SelectionVolume)
	 */
	void setSource(SelectionTest& test);
	
	/** greebo: Sets the source patch to the given <sourcePatch>
	 */
	void setSource(Patch& sourcePatch);
	
	/** greebo: Sets the source face to the given <sourceFace>
	 */
	void setSource(Face& sourceFace);
	
	/** greebo: Sets the source face to the given <shader>
	 */
	void setSource(std::string shader);

	/** greebo: Retrieves the current source Texturable
	 */
	Texturable& getSource();

	/** greebo: Clears both the source and target texturables.
	 * 			Call this as soon as the objects might be deleted
	 * 			or the map is changed.
	 */
	void clear();

private:

	/** greebo: Updates the shader information in the status bar.
	 */
	void updateStatusText();

	/** greebo: Sets the media browser / texwindow focus to the
	 * 			new source shader.
	 */
	void updateMediaBrowsers();

	/** greebo: Retrieves the best texturable object from the
	 * 			given SelectionTest.
	 */
	Texturable getTexturable(SelectionTest& test);
};

} // namespace selection

selection::ShaderClipboard& GlobalShaderClipboard();

#endif /*SHADERCLIPBOARD_H_*/
