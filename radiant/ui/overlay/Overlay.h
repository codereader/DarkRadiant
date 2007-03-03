#ifndef OVERLAY_H_
#define OVERLAY_H_

#include "iimage.h"
#include "iregistry.h"
#include "ishaders.h"

#include <string>

namespace ui {

/**
 * The Overlay class allows a background image to be drawn in the XY window.
 * It is configured via the XMLRegistry, and its draw() method is called from
 * the XYWnd class to insert the image into the window.
 */
class Overlay : 
	public RegistryKeyObserver
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
	
	// The instance of the GDKModule loader
	ImageLoaderModuleRef _imageGDKModule;

private:

	/* Main constructor */
	Overlay();

	// Capture the texture	
	void captureTexture() {
		if (_imageName != "") {
			_texture = GlobalShaderSystem().loadTextureFromFile(_imageName);
		}
	}

	// Toggle image visibility
	void show(bool shown) {
		_visible = shown;
	}
	
public:
	
	/**
	 * Static method to retrieve the singleton Overlay instance.
	 */
	static Overlay& getInstance();
	
	// Sets the name of the image that should be loaded
	void setImage(const std::string& imageName) {
		// Do nothing, if the current image is the same
		if (imageName == _imageName) {
			return;
		}
		
		_imageName = imageName;
		
		// Set the visibility flag to zero, if no imageName is specified
		if (_imageName == "") {
			_visible = false;
		}
		
		captureTexture();
	}
	
	void setTransparency(const float& transparency) {
		_transparency = transparency;
		
		// Check for valid bounds (0.0f ... 1.0f)
		if (_transparency > 1.0f) {
			_transparency = 1.0f;
		}
		else if (_transparency < 0.0f) {
			_transparency = 0.0f;
		}
	}
	
	// Helper method, constrains the <input> float to the given min/max values
	float constrainFloat(const float& input, const float& min, const float& max) {
		if (input < min) {
			return min;
		}
		else if (input > max) {
			return max;
		}
		else {
			return input;
		}
	}
	
	// Sets the image scale to the given float (1.0f is no scaling)
	void setImageScale(float scale);
	
	// Sets the image position in quasi texture coordinates (-0.5f .. 0.5f)
	void setImagePosition(const float& x, const float& y) {
		_translationX = constrainFloat(x, -1.0f, 1.0f);
		_translationY = constrainFloat(y, -1.0f, 1.0f);
	}

	// RegistryKeyObserver implementation, gets called upon key change
	void keyChanged();
	
	/**
	 * Public draw method, called from the XYWnd.
	 */
	void draw(float xbegin, float xend, 
			  float ybegin, float yend, float xyviewscale); 

}; // class Overlay

} // namespace ui

#endif /*OVERLAY_H_*/
