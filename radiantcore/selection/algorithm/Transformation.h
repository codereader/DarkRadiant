#pragma once

#include "icommandsystem.h"
#include "iclipper.h"
#include "math/Vector3.h"

namespace selection
{

namespace algorithm
{

/**
 * Applies the given quaternion to the current selection.
 */
void rotateSelected(const Quaternion& rotation);

/** greebo: Rotates the current selection about the
	* 			specified rotation angles.
	*
	* @eulerXYZ: A three-component vector containing the three
	* 			  angles in degrees (vector[0] refers to x-axis rotation).
	*
	* Note: this is an undoable command.
	*/
void rotateSelected(const Vector3& eulerXYZ);

/**
 * Command adapter for rotateSelected(Vector3)
 */
void rotateSelectedEulerXYZ(const cmd::ArgumentList& args);

/** greebo: Scales the current selection with the given vector.
	* 			this emits an error if one of the vector's components
	* 			are zero.
	*
	* @scaleXYZ: A three-component vector (can be non-uniform) containing
	* 			  the three scale factors.
	*
	* Note: this is an undoable command.
	*/
void scaleSelected(const Vector3& scaleXYZ);

/**
 * Command adapter for scaleSelected(Vector3)
 */
void scaleSelectedCmd(const cmd::ArgumentList& args);

/**
 * greebo: This duplicates the current selection (that's what happening
 * when you hit the space bar).
 */
void cloneSelected(const cmd::ArgumentList& args);

/**
 * Moves the current selection by the given translation vector.
 */
void translateSelected(const Vector3& translation);

enum ENudgeDirection
{
	eNudgeUp		= 1,
	eNudgeDown	= 3,
	eNudgeLeft	= 0,
	eNudgeRight	= 2,
};

// "Nudges" (translates) the current selection in the specified direction
// The GlobalXYWnd's active viewtype decides how "left" / "right" / "up" / "down"
// are interpreted. The GlobalGrid is used for the amount
void nudgeSelected(ENudgeDirection direction);

// Overload for more control of what happens
void nudgeSelected(ENudgeDirection direction, float amount, OrthoOrientation viewtype);

/**
* Command target, interprets the first command as direction
*
* args[0]: String enum indicating the direction: "left", "right", "up" or "down"
*/
void nudgeSelectedCmd(const cmd::ArgumentList& args);

/**
* Generic move command, always moves the selection in the given direction.
* args[0]: Vector defining the translation
*/
void moveSelectedCmd(const cmd::ArgumentList& args);

/**
* Moves the selection along the z axis by the given amount.
*/
void moveSelectedAlongZ(float amount);

/**
* Vertical move command, always moves the selection along the z axis.
* args[0]: String indicating the direction: "up" or "down".
* Each move command is using the current grid size as amount.
*/
void moveSelectedVerticallyCmd(const cmd::ArgumentList& args);

/**
 * Rotates the current selection about one of the main axes.
 */
void rotateSelectionX(const cmd::ArgumentList& args);
void rotateSelectionY(const cmd::ArgumentList& args);
void rotateSelectionZ(const cmd::ArgumentList& args);

/**
 * Mirrors the current selection about one of the main axes.
 * Behind the scenes, a negative scale operation is performed.
 */
void mirrorSelectionX(const cmd::ArgumentList& args);
void mirrorSelectionY(const cmd::ArgumentList& args);
void mirrorSelectionZ(const cmd::ArgumentList& args);

} // namespace algorithm

} // namespace selection
