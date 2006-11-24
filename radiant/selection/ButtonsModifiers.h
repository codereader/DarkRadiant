#ifndef BUTTONSMODIFIERS_H_
#define BUTTONSMODIFIERS_H_

#include "windowobserver.h"

/* greebo: Here are the definitions of the modifier combinations and their meaning. To create
 * a customisable interface this should go into the XMLRegistry
 */

#if 1
// Buttons interpreted as the "select" button and "texture" button
const ButtonIdentifier c_button_select = c_buttonLeft;
const ButtonIdentifier c_button_texture = c_buttonMiddle;

// Which modifiers have to be held down for manipulator mode
const ModifierFlags c_modifier_manipulator = c_modifierNone;

// Which modifiers have to be held down for toggle mode (ordinary selection toggle)
const ModifierFlags c_modifier_toggle = c_modifierShift;

// The modifiers for the replace/cycle selection mode
const ModifierFlags c_modifier_replace = c_modifierShift | c_modifierAlt;

// The face modifier. 
const ModifierFlags c_modifier_face = c_modifierControl;

#else
// greebo: An artefact of older radiant versions?
const ButtonIdentifier c_button_select = c_buttonLeft;
const ModifierFlags c_modifier_manipulator = c_modifierNone;
const ModifierFlags c_modifier_toggle = c_modifierControl;
const ModifierFlags c_modifier_replace = c_modifierNone;
const ModifierFlags c_modifier_face = c_modifierShift;
#endif

// Toggle face is the combination of the toggle and the face modifier
const ModifierFlags c_modifier_toggle_face = c_modifier_toggle | c_modifier_face;
const ModifierFlags c_modifier_replace_face = c_modifier_replace | c_modifier_face;

// Modifiers for copy / apply texture
const ModifierFlags c_modifier_apply_texture = c_modifierControl | c_modifierShift;
const ModifierFlags c_modifier_copy_texture = c_modifierControl | c_modifierAlt;

#endif /*BUTTONSMODIFIERS_H_*/
