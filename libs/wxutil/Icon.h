#pragma once

#include <wx/defs.h>

#if wxCHECK_VERSION(3,1,6)
#define DR_WX_HAS_BITMAP_BUNDLE
#else
#undef DR_WX_HAS_BITMAP_BUNDLE
#endif

#ifdef DR_WX_HAS_BITMAP_BUNDLE
#include <wx/bmpbndl.h>
#else
#include <wx/icon.h>
#endif

namespace wxutil
{

/**
 * Adapter class to provide build compatibility in both wxWidgets 3.2 and 3.1.5- builds.
 * The type wxBitmapBundle doesn't exist in wxWidgets 3.1.5 and earlier.
 * In wxWidgets 3.1.6+ the wxDataViewIconText constructor is accepting wxBitmapBundle
 * whereas in earlier versions it's accepting a wxIcon only. This adapter class
 * provides conversion operators to the type that is supported in the active wx version.
 *
 * Note: The implicit constructor conversion between wxIcon and wxBitmapBundle provided
 * by wxWidgets 3.1.6+ has been causing trouble (crashes), hence this class.
 */
class Icon
{
private:
#ifdef DR_WX_HAS_BITMAP_BUNDLE
    wxBitmapBundle _bitmap;
#else
    wxIcon _bitmap;
#endif
public:
    Icon()
    {}

    Icon(const wxBitmap& bitmap)
#ifdef DR_WX_HAS_BITMAP_BUNDLE
        : _bitmap(bitmap)
#endif
    {
#ifndef DR_WX_HAS_BITMAP_BUNDLE
        _bitmap.CopyFromBitmap(bitmap);
#endif
    }

    Icon(const Icon& other) = default;
    Icon(Icon&& other) = default;
    Icon& operator=(const Icon& other) = default;

    // Implicit conversion operators
#ifdef DR_WX_HAS_BITMAP_BUNDLE
    operator const wxBitmapBundle&() const
    {
        return _bitmap;
    }
#else
    operator const wxIcon&() const
    {
        return _bitmap;
    }
#endif
};

}