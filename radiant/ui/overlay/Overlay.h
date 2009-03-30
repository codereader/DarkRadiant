#ifndef OVERLAY_H_
#define OVERLAY_H_

#include "iimage.h"
#include "iregistry.h"
#include "ishaders.h"
#include "iradiant.h"

namespace ui {

class Overlay;
typedef boost::shared_ptr<Overlay> OverlayPtr;

/**
 * The Overlay class allows a background image to be drawn in the XY window.
 * It is configured via the XMLRegistry, and its draw() method is called from
 * the XYWnd class to insert the image into the window.
 */
class Overlay : 
	public RegistryKeyObserver,
	public RadiantEventListener
{
private:
	
	// The imagename to be drawn
	std::string _imageName;
	
	// TRUE, if the overlay is visible
	bool _visible;
	
	// The transparency of the image (0 = completely transparent, 1 = opaque)
	float _transparency;
	
	// The image scale
	float _scale;
	
	// TRUE, if the image should be rescaled along with the orthoview scale
	bool _scaleWithXYView;
	
	// TRUE, if the image should be panned along
	bool _panWithXYView;
	
	// Set to TRUE, if the image should be displayed proportionally.
	bool _keepProportions;
	
	// The x,y translation of the texture center
	float _translationX;
	float _translationY;

	// The loaded texture
	TexturePtr _texture;

private:

	/* Main constructor */
	Overlay();
	
	// Capture the texture	
	void captureTexture();

	// Toggle image visibility
	void show(bool shown);
	
	static OverlayPtr& InstancePtr();
	
public:
	// Frees the static shared_ptr of the singleton instance
	static void destroyInstance();
	
	/**
	 * Static method to retrieve the singleton Overlay instance.
	 */
	static Overlay& Instance();

	// RadiantEventListener implementation
	virtual void onRadiantShutdown();
	
	// Sets the name of the image that should be loaded
	void setImage(const std::string& imageName);
	
	void setTransparency(const float& transparency);
	
	// Sets the image scale to the given float (1.0f is no scaling)
	void setImageScale(float scale);
	
	// Sets the image position in quasi texture coordinates
	void setImagePosition(const float& x, const float& y);

	// RegistryKeyObserver implementation, gets called upon key change
	void keyChanged(const std::string& key, const std::string& val);
	
	/**
	 * Public draw method, called from the XYWnd.
	 */
	void draw(float xbegin, float xend, 
			  float ybegin, float yend, float xyviewscale); 

private:

	// Helper method, constrains the <input> float to the given min/max values
	float constrainFloat(const float& input, const float& min, const float& max);

}; // class Overlay

} // namespace ui

#endif /*OVERLAY_H_*/
