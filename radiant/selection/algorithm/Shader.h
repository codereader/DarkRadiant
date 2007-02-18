#ifndef SELECTION_ALGORITHM_SHADER_H_
#define SELECTION_ALGORITHM_SHADER_H_

#include <string>
#include "math/Vector2.h"

class TextureProjection;

namespace selection {
	namespace algorithm {
	
	/** greebo: Returns the number of the selected face instances.
	 */
	int selectedFaceCount();
	
	/** greebo: Retrieves the shader name from the current selection.
	 * 
	 * @returns: the name of the shader that is shared by every selected instance or
	 * the empty string "" otherwise.
	 */
	std::string getShaderFromSelection();
	
	/** greebo: Retrieves the texture projection from the current selection.
	 * 
	 * @returns: the TextureProjection of the last selected face/brush.
	 */
	TextureProjection getSelectedTextureProjection();
	
	/** greebo: Get the width/height of the shader of the last selected
	 * face instance.
	 * 
	 * @returns: A Vector2 with <width, height> of the shader or <0,0> if empty.
	 */
	Vector2 getSelectedFaceShaderSize();
	
	/** greebo: Rescales the texture of the selected primitives to fit
	 * <repeatX> times horizontally and <repeatY> times vertically 
	 */
	void fitTexture(const float& repeatS, const float& repeatT);
	
	} // namespace algorithm
} // namespace selection

#endif /*SELECTION_ALGORITHM_SHADER_H_*/
