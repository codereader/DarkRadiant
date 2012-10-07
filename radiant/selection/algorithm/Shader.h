#pragma once

#include <string>
#include "icommandsystem.h"
#include "math/Vector2.h"

class TextureProjection;
class Face;
class SelectionTest;

enum EAlignType
{
	ALIGN_TOP,
	ALIGN_BOTTOM,
	ALIGN_LEFT,
	ALIGN_RIGHT,
};

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

	/** greebo: Applies the shader in the clipboard to the nearest
	 * 			texturable object (using the given SelectionTest)
	 *
	 * @test: the SelectionTest needed (usually a SelectionVolume).
	 *
	 * @projected: Set this to TRUE if the texture is projected onto patches using the
	 * 			   face in the shaderclipboard as reference plane
	 * 			   Set this to FALSE if a natural texturing of patches is attempted.
	 *
	 * @entireBrush: Set this to TRUE if all brush faces should be textured,
	 * 				 given the case that the SelectionTest is resulting in a brush.
	 */
	void pasteShader(SelectionTest& test, bool projected, bool entireBrush = false);

	/** greebo: Copies the texture coordinates from the source patch in the
	 * 			ShaderClipboard to the target patch defined by the SelectionTest.
	 * 			Tests are performed to ensure that the operation is valid,
	 * 			an error message is displayed otherwise.
	 *
	 * @test: the SelectionTest needed (usually a SelectionVolume).
	 */
	void pasteTextureCoords(SelectionTest& test);

	/** greebo: The command target of "CopyTexure". This tries to pick the shader
	 * 			from the current selection and copies it to the clipboard.
	 */
	void pickShaderFromSelection(const cmd::ArgumentList& args);

	/** greebo: The command target of "PasteTexture". This tries to get the Texturables
	 * 			from the current selection and pastes the clipboard shader onto them.
	 */
	void pasteShaderToSelection(const cmd::ArgumentList& args);

	/** greebo: The command target of "PasteTextureNatural". This tries to get
	 * 			the Texturables	from the current selection and
	 * 			pastes the clipboard shader "naturally" (undistorted) onto them.
	 */
	void pasteShaderNaturalToSelection(const cmd::ArgumentList& args);

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
	void fitTexture(const float repeatS, const float repeatT);

	/** greebo: Flips the texture about the given <flipAxis>
	 *
	 * @flipAxis: 0 = flip S, 1 = flip T
	 */
	void flipTexture(unsigned int flipAxis);

	/** greebo: The command Targets for flipping the textures about the
	 * 			S and T axes respectively.
	 * 			Passes the call to flipTexture() method above.
	 */
	void flipTextureS(const cmd::ArgumentList& args);
	void flipTextureT(const cmd::ArgumentList& args);

	void alignTexture(EAlignType align);

	// Aligns the texture of the current objets so that the image border aligns with a world edge
	void alignTextureCmd(const cmd::ArgumentList& args);

	/** greebo: Applies the texture "naturally" to the selected
	 * primitives. Natural makes use of the currently active default scale.
	 */
	void naturalTexture(const cmd::ArgumentList& args);

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

	/**
	 * Command target to rotate the texture.
	 *
	 * args[0]: Pass positive values to rotate clockwise,
	 *          negative values for counter-clockwise rotation.
	 */
	void rotateTexture(const cmd::ArgumentList& args);

	/**
	 * Command target to scale the texture.
	 *
	 * args[0]: Vector2 which contains the relative scale values
	 *          in the s and t axis (0 = 100%).
	 * args[0]: String: [up|down|left|right] which takes the values
	 *          from the SurfaceInspector.
	 *
	 *          Example: <0.05, 0> results in a 105% scale in the s direction.
	 */
	void scaleTexture(const cmd::ArgumentList& args);

	/**
	 * Command target to shift the texture.
	 *
	 * args[0]: Vector2 which contains the s/t shift values
	 * args[0]: String: [up|down|left|right] which takes the values
	 *          from the SurfaceInspector.
	 */
	void shiftTextureCmd(const cmd::ArgumentList& args);

	/** greebo: This translates the texture coordinates towards the origin
	 * 			in texture space without altering the appearance.
	 * 			The texture is translated in multiples of 1.0
	 */
	void normaliseTexture(const cmd::ArgumentList& args);

	/** greebo: Replaces all <find> shaders with <replace>.
	 *
	 * @find/replace: the full shadernames ("textures/darkmod/bleh")
	 * @selectedOnly: if TRUE, searches the current selection only.
	 *
	 * @returns: the number of replaced occurrences.
	 */
	int findAndReplaceShader(const std::string& find,
							 const std::string& replace,
							 bool selectedOnly);

	/**
	 * greebo: (De-)selects all map nodes that match the given shaderName.
	 */
	void selectItemsByShader(const std::string& shaderName);
	void deselectItemsByShader(const std::string& shaderName);

	// Command target to (de-)select items by shader name
	void selectItemsByShader(const cmd::ArgumentList& args);
	void deselectItemsByShader(const cmd::ArgumentList& args);

	} // namespace algorithm
} // namespace selection
