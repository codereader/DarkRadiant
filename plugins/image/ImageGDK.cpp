#include "ImageGDK.h"

#include <gdk/gdkpixbuf.h>
#include "iarchive.h"
#include "imagelib.h"
#include "stream/textstream.h"

/* greebo: This loads a file from the disk using GDKPixbuf routines.
 * 
 * The image is loaded and its alpha channel is set uniformly to 1, the
 * according Image class is instantiated and the pointer is returned.
 * 
 * Note: returns NULL if the file could not be loaded.
 */

ImagePtr LoadImageGDK(ArchiveFile& file) {
	
	// Allocate a new GdkPixBuf and create an alpha-channel with alpha=1.0
	GdkPixbuf* rawPixbuf = gdk_pixbuf_new_from_file(file.getName().c_str(), NULL);
	
	// Only create an alpha channel if the other rawPixbuf could be loaded
	GdkPixbuf* img = (rawPixbuf != NULL) ? gdk_pixbuf_add_alpha(rawPixbuf, TRUE, 255, 0, 255) : NULL;
	
	if (img != NULL) {
		// Allocate a new image
		RGBAImagePtr image (new RGBAImage(gdk_pixbuf_get_width(img), gdk_pixbuf_get_height(img)));
		
		// Initialise the source buffer pointers
		guchar* gdkStart = gdk_pixbuf_get_pixels(img);
		int rowstride = gdk_pixbuf_get_rowstride(img);
		int numChannels = gdk_pixbuf_get_n_channels(img);
		
		// Set the target buffer pointer to the first RGBAPixel
		RGBAPixel* targetPixel = image->pixels;
		
		// Now do an unelegant cycle over all the pixels and move them into the target
		for (unsigned int y = 0; y < image->height; y++) {
			for (unsigned int x = 0; x < image->width; x++) {
				guchar* gdkPixel = gdkStart + y*rowstride + x*numChannels;
				
				// Copy the values from the GdkPixel
				targetPixel->red = gdkPixel[0];
				targetPixel->green = gdkPixel[1];
				targetPixel->blue = gdkPixel[2];
				targetPixel->alpha = gdkPixel[3];
				
				// Increase the pointer
				targetPixel++;
			}
		}
		
		// Free the GdkPixbufs from the memory
		g_object_unref(G_OBJECT(img)); 
		g_object_unref(G_OBJECT(rawPixbuf));

		return image;
	}
	
	// No image could be loaded, return NULL	
	return ImagePtr();
}
