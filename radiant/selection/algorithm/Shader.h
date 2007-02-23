#ifndef SELECTION_ALGORITHM_SHADER_H_
#define SELECTION_ALGORITHM_SHADER_H_

#include <string>
#include "math/Vector2.h"

class TextureProjection;
class Face;

namespace selection {
	namespace algorithm {
	
	/** greebo: Retrieves the shader name from the current selection.
	 * 
	 * @returns: the name of the shader that is shared by every selected instance or
	 * the empty string "" otherwise.
	 */
	std::string getShaderFromSelection();
	
	/** greebo: Applies the given shader to the current selection.
	 */
	void applyShaderToSelection(const std::string& shaderName);
	
	/** greebo: Retrieves the texture projection from the current selection.
	 * 
	 * @returns: the TextureProjection of the last selected face/brush.
	 */
	TextureProjection getSelectedTextureProjection();
	
	/** greebo: Applies the given textureprojection to the selected
	 * brushes and brush faces.
	 */
	void applyTextureProjectionToFaces(TextureProjection& projection);
	
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
	
	/** greebo: Applies the texture "naturally" to the selected
	 * primitives. Natural makes use of the currently active default scale. 
	 */
	void naturalTexture();
	
	/** greebo: Shifts the texture of the current selection about
	 * the specified Vector2
	 */
	void shiftTexture(const Vector2& shift);
	
	/** greebo: These are the shortcut methods that scale/shift/rotate the 
	 * texture of the selected primitives about the values that are currently
	 * active in the Surface Inspector.
	 */
	void shiftTextureLeft();
	void shiftTextureRight();
	void shiftTextureUp();
	void shiftTextureDown();
	void scaleTextureLeft();
	void scaleTextureRight();
	void scaleTextureUp();
	void scaleTextureDown();
	void rotateTextureClock();
	void rotateTextureCounter();
	
	/** greebo: This translates the texture coordinates towards the origin
	 * 			in texture space without altering the appearance.
	 * 			The texture is translated in multiples of 1.0 
	 */
	void normaliseTexture();
	
	} // namespace algorithm
} // namespace selection

#endif /*SELECTION_ALGORITHM_SHADER_H_*/
