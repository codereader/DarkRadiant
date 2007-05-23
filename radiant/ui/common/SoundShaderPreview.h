#ifndef SOUNDSHADERPREVIEW_H_
#define SOUNDSHADERPREVIEW_H_

// Forward declaration
typedef struct _GtkWidget GtkWidget;

namespace ui {

/** greebo: This class provides the UI elements to inspect a given
 * 			sound shader with playback option.
 * 
 * 			Use the GtkWidget* cast operator to pack this into a
 * 			parent container. 
 */
class SoundShaderPreview
{
	// The main container widget of this preview
	GtkWidget* _preview;
	
public:
	SoundShaderPreview();

	/** greebo: Operator cast to GtkWidget to pack this into a 
	 * 			parent container widget.
	 */
	operator GtkWidget*();
};

} // namespace ui

#endif /*SOUNDSHADERPREVIEW_H_*/
