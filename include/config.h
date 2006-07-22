#ifndef CONFIG_H_
#define CONFIG_H_

/****************************************************************************
 * ENTITY INSPECTOR
 ****************************************************************************/

/* This option specifies that the new Entity Inspector should be used instead of
 * the legacy GtkRadiant dialog
 */
 
#define USE_NEW_ENTITY_INSPECTOR


/* Enable this option to force the Entity Inspector to use Text property editors
 * for all key types
 */

//#define ALL_PROPERTY_EDITORS_ARE_TEXT


/* Enable this option to allow the use of ClassnamePropertyEditor for the "classname"
 * property. A corresponding change must be made in the .game file to select this
 * editor for the property.
 */

//#define USE_CLASSNAME_PROPERTY_EDITOR


/* Specify whether an "Undo" button should be displayed on the PropertyEditor.
 */
 
//#define PROPERTY_EDITOR_HAS_UNDO_BUTTON


/*************************************************************************
 * ORTHO WINDOW
 *************************************************************************/
 
/* Use the new context menu, with links to Eclass browser and model selector,
 * rather than the old-style entity menu.
 */
 
#define USE_NEW_ORTHO_CONTEXT_MENU

#endif /*CONFIG_H_*/
