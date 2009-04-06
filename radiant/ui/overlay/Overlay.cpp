#include "Overlay.h"

#include "math/Vector3.h"
#include "math/matrix.h"
#include "texturelib.h"

#include "OverlayRegistryKeys.h"

namespace ui {

/* CONSTANTS */

namespace {
	const float MIN_SCALE = 0.001f;
	const float MAX_SCALE = 20.0f;
	const float TRANSLATION_MIN = -20.0f;
	const float TRANSLATION_MAX = 20.0f;
}

// Main constructor
Overlay::Overlay()
:	_imageName(GlobalRegistry().get(RKEY_OVERLAY_IMAGE)),
	_visible(GlobalRegistry().get(RKEY_OVERLAY_VISIBLE) == "1"),
	_transparency(GlobalRegistry().getFloat(RKEY_OVERLAY_TRANSPARENCY)),
	_scale(GlobalRegistry().getFloat(RKEY_OVERLAY_SCALE)),
	_scaleWithXYView(GlobalRegistry().get(RKEY_OVERLAY_SCALE_WITH_XY) == "1"),
	_panWithXYView(GlobalRegistry().get(RKEY_OVERLAY_PAN_WITH_XY) == "1"),
	_keepProportions(GlobalRegistry().get(RKEY_OVERLAY_PROPORTIONAL) == "1"),
	_translationX(GlobalRegistry().getFloat(RKEY_OVERLAY_TRANSLATIONX)),
	_translationY(GlobalRegistry().getFloat(RKEY_OVERLAY_TRANSLATIONY))
{
	// Watch the relevant registry keys		
	GlobalRegistry().addKeyObserver(this, RKEY_OVERLAY_VISIBLE);
	GlobalRegistry().addKeyObserver(this, RKEY_OVERLAY_TRANSPARENCY);
	GlobalRegistry().addKeyObserver(this, RKEY_OVERLAY_IMAGE);
	GlobalRegistry().addKeyObserver(this, RKEY_OVERLAY_SCALE);
	GlobalRegistry().addKeyObserver(this, RKEY_OVERLAY_TRANSLATIONX);
	GlobalRegistry().addKeyObserver(this, RKEY_OVERLAY_TRANSLATIONY);
	GlobalRegistry().addKeyObserver(this, RKEY_OVERLAY_PROPORTIONAL);
	GlobalRegistry().addKeyObserver(this, RKEY_OVERLAY_SCALE_WITH_XY);
	GlobalRegistry().addKeyObserver(this, RKEY_OVERLAY_PAN_WITH_XY);
}

void Overlay::onRadiantShutdown() {
	_texture = TexturePtr();
	GlobalRegistry().removeKeyObserver(this);
}

void Overlay::destroyInstance() {
	InstancePtr() = OverlayPtr();
}

OverlayPtr& Overlay::InstancePtr() {
	static OverlayPtr _instancePtr;
	
	if (_instancePtr == NULL) {
		// Not yet instantiated, do it now
		_instancePtr = OverlayPtr(new Overlay);
		
		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().addEventListener(_instancePtr);
	}
	
	return _instancePtr;
}

// Static instance owner
Overlay& Overlay::Instance() {
	return *InstancePtr();	
}

void Overlay::show(bool shown) {
	_visible = shown;
}

void Overlay::setTransparency(const float& transparency) {
	_transparency = transparency;
	
	// Check for valid bounds (0.0f ... 1.0f)
	if (_transparency > 1.0f) {
		_transparency = 1.0f;
	}
	else if (_transparency < 0.0f) {
		_transparency = 0.0f;
	}
}

void Overlay::setImage(const std::string& imageName) {
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

void Overlay::captureTexture() {
	if (_imageName != "") {
		// Load the image using the GDK image module
		_texture = GlobalMaterialManager().loadTextureFromFile(_imageName, "GDK");
	}
}

// Main draw function
void Overlay::draw(float xbegin, float xend, float ybegin, float yend, 
				   float xyviewscale)
{
	if (!_visible) {
		return;
	}
	
	// Check if the texture is realised
	if (_texture == NULL) {
		// Try to realise it
		captureTexture();
	}
	
	// Return without drawing anything if the texture pointer is invalid
	if (!_texture)
		return;
	
	// The two corners of the window (default: stretches to window borders)
	Vector3 windowUpperLeft(xbegin, ybegin, 0);
	Vector3 windowLowerRight(xend, yend, 0);
	
	if (_keepProportions) {
		float aspectRatio = static_cast<float>(_texture->getWidth())/_texture->getHeight();
		
		// Calculate the proportionally stretched yEnd coordinate
		float newYend = ybegin + (xend - xbegin) / aspectRatio;
		windowLowerRight = Vector3(xend, newYend, 0);
		
		// Now calculate how far the center went off due to this stretch
		float deltaCenter = (newYend - yend)/2;
		
		// Correct the y coordinates with the delta, so that the image gets centered again
		windowLowerRight.y() -= deltaCenter;
		windowUpperLeft.y() -= deltaCenter;
	}
	
	// Calculate the (virtual) window center
	Vector3 windowOrigin((xend + xbegin)/2, (yend + ybegin)/2, 0);
	
	windowUpperLeft -= windowOrigin;
	windowLowerRight -= windowOrigin;
	
	// The translation vector
	Vector3 translation(_translationX * (xend - xbegin) * xyviewscale, _translationY * (yend - ybegin) * xyviewscale, 0);
	
	// Create a translation matrix
	Matrix4 scaleTranslation = Matrix4::getTranslation(translation);
		
	// Store the scale into the matrix
	scaleTranslation.xx() = _scale;
	scaleTranslation.yy() = _scale;
	
	if (_scaleWithXYView) {
		// Scale once again with the xyviewscale, if enabled
		scaleTranslation.xx() *= xyviewscale;
		scaleTranslation.yy() *= xyviewscale;
	}
	
	// Apply the transformations onto the window corners
	windowUpperLeft = scaleTranslation.transform(windowUpperLeft).getProjected();
	windowLowerRight = scaleTranslation.transform(windowLowerRight).getProjected();
	
	if (!_panWithXYView) {
		windowUpperLeft += windowOrigin;
		windowLowerRight += windowOrigin;
	}
	
	// Enable the blend functions and textures
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	
	// Define the blend function for transparency
	glBlendColor(0,0,0, _transparency);
	glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);
	
	// Define the texture (get the ID from the texture object)
	glBindTexture (GL_TEXTURE_2D, _texture->getGLTexNum());
	glColor3f (1,1,1);
	
	// Draw the rectangle with the texture on it
	glBegin(GL_QUADS);
	glTexCoord2i(0,1);
	glVertex3f(windowUpperLeft.x(), windowUpperLeft.y(), 0.0f);
	
	glTexCoord2i(1,1);
	glVertex3f(windowLowerRight.x(), windowUpperLeft.y(), 0.0f);
	
	glTexCoord2i(1,0);
	glVertex3f(windowLowerRight.x(), windowLowerRight.y(), 0.0f);
	
	glTexCoord2i(0,0);
	glVertex3f(windowUpperLeft.x(), windowLowerRight.y(), 0.0f);
	glEnd();
	
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

// Sets the image scale to the given float (1.0f is no scaling)
void Overlay::setImageScale(float scale) {
	_scale = constrainFloat(scale, MIN_SCALE, MAX_SCALE);
}

// RegistryKeyObserver implementation, gets called upon key change
void Overlay::keyChanged(const std::string& key, const std::string& val) 
{
	show(GlobalRegistry().get(RKEY_OVERLAY_VISIBLE) == "1");
	_keepProportions = (GlobalRegistry().get(RKEY_OVERLAY_PROPORTIONAL) == "1");
	_scaleWithXYView = (GlobalRegistry().get(RKEY_OVERLAY_SCALE_WITH_XY) == "1"),
	_panWithXYView = (GlobalRegistry().get(RKEY_OVERLAY_PAN_WITH_XY) == "1"),
	setImage(GlobalRegistry().get(RKEY_OVERLAY_IMAGE));
	setTransparency(GlobalRegistry().getFloat(RKEY_OVERLAY_TRANSPARENCY));
	setImageScale(GlobalRegistry().getFloat(RKEY_OVERLAY_SCALE));
	setImagePosition( GlobalRegistry().getFloat(RKEY_OVERLAY_TRANSLATIONX),
					  GlobalRegistry().getFloat(RKEY_OVERLAY_TRANSLATIONY) );
}

// Helper method, constrains the <input> float to the given min/max values
float Overlay::constrainFloat(const float& input, const float& min, const float& max) {
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

// Sets the image position in quasi texture coordinates (-0.5f .. 0.5f)
void Overlay::setImagePosition(const float& x, const float& y) {
	_translationX = constrainFloat(x, TRANSLATION_MIN, TRANSLATION_MAX);
	_translationY = constrainFloat(y, TRANSLATION_MIN, TRANSLATION_MAX);
}

} // namespace ui
