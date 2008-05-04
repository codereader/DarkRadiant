#ifndef _GTKUTIL_ICONV_H
#define _GTKUTIL_ICONV_H

#include <glib/gconvert.h>
#include <glib/gmessages.h>
#include <glib/gunicode.h>
#include <glib/gmem.h>
#include <string>

#include <cassert>

namespace gtkutil {

/**
 * greebo: This is a wrapper class around the iconv functions.
 *         Many codeparts have been taken from the GLib::IConv 
 *         class in the glibmm 2.14.2 library.
 */
class IConv
{
private:
	GIConv gobject_;

public:
	/** Open new conversion descriptor.
	 * @param to_codeset Destination codeset.
	 * @param from_codeset %Source codeset.
	 */
	IConv(const std::string& to_codeset, const std::string& from_codeset) :
		gobject_(g_iconv_open(to_codeset.c_str(), from_codeset.c_str()))
	{
		if (gobject_ == reinterpret_cast<GIConv>(-1)) {
			GError* gerror = 0;

			// Abuse g_convert() to create a GError object.  This may seem a weird
			// thing to do, but it gives us consistently translated error messages
			// at no further cost.
			g_convert("", 0, to_codeset.c_str(), from_codeset.c_str(), 0, 0, &gerror);

			// If this should ever fail we're fucked.
			assert(gerror != 0);
		}
	}

	/** Close conversion descriptor.
	 */
	~IConv() {
		g_iconv_close(gobject_);
	}

	/** Same as the standard UNIX routine %iconv(), but may be implemented
	 * via libiconv on UNIX flavors that lack a native implementation.  glibmm
	 * provides Glib::convert() and Glib::locale_to_utf8() which are likely
	 * more convenient than the raw iconv wrappers.
	 * @param inbuf Bytes to convert.
	 * @param inbytes_left In/out parameter, bytes remaining to convert in @a inbuf.
	 * @param outbuf Converted output bytes.
	 * @param outbytes_left In/out parameter, bytes available to fill in @a outbuf.
	 * @return Count of non-reversible conversions, or <tt>static_cast<size_t>(-1)</tt> on error.
	 */
	size_t iconv(char** inbuf, gsize* inbytes_left, char** outbuf, gsize* outbytes_left) {
		return g_iconv(gobject_, inbuf, inbytes_left, outbuf, outbytes_left);
	}

	/** Reset conversion descriptor to initial state.
	 * Same as <tt>iconv(0, 0, 0, 0)</tt>, but implemented slightly differently
	 * in order to work on Sun Solaris <= 7.  It's also more obvious so you're
	 * encouraged to use it.
	 */
	void reset() {
		// Apparently iconv() on Solaris <= 7 segfaults if you pass in
		// NULL for anything but inbuf; work around that. (NULL outbuf
		// or NULL *outbuf is allowed by Unix98.)

		char* outbuf        = 0;
		gsize inbytes_left  = 0;
		gsize outbytes_left = 0;

		g_iconv(gobject_, 0, &inbytes_left, &outbuf, &outbytes_left);
	}

	/** Convert from one encoding to another.
	 * @param str The string to convert.
	 * @return The converted string.
	 * @throw Glib::ConvertError
	 */
	std::string convert(const std::string& str) {
		gsize bytes_written = 0;
		GError* gerror = 0;

		char *const buf = g_convert_with_iconv(str.data(), static_cast<gssize>(str.size()), gobject_, 0, &bytes_written, &gerror);

		if (gerror) {
			return "";
		}

		std::string returnValue(buf, bytes_written);
		g_free(buf);

		return returnValue;
	}

	GIConv gobj() { 
		return gobject_;
	}

	/** 
	 * greebo: Converts the given string to UTF-8 encoding. 
	 * Returns an empty string if the conversion fails.
	 */
	static std::string localeToUTF8(const std::string& input) {
		gsize bytes_written = 0;
		GError* gerror = 0;

		char *const buf = g_locale_to_utf8(input.data(), static_cast<gssize>(input.size()), 0, &bytes_written, &gerror);

		if (gerror) return "";
		
		std::string returnValue(buf, bytes_written);
		g_free(buf);

		return returnValue;
	}

	/** 
	 * greebo: Converts the given string from UTF-8 to the current locale. 
	 * Returns an empty string if the conversion fails.
	 */	
	static std::string localeFromUTF8(const std::string& input) {
		gsize bytes_written = 0;
		GError* gerror = 0;

		char *const buf = g_locale_from_utf8(input.data(), static_cast<gssize>(input.size()), 0, &bytes_written, &gerror);

		if (gerror) return "";
		
		std::string returnValue(buf, bytes_written);
		g_free(buf);

		return returnValue;
	}
};

} // namespace gtkutil

#endif /* _GTKUTIL_ICONV_H */
