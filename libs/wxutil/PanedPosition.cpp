#include "PanedPosition.h"

#include "string/convert.h"
#include "iregistry.h"
#include <wx/splitter.h>

namespace
{
    const int DEFAULT_POSITION = 200;
}

namespace wxutil
{

PanedPosition::PanedPosition() :
    _position(DEFAULT_POSITION)
{}

PanedPosition::~PanedPosition()
{
    disconnect();
}

void PanedPosition::connect(wxSplitterWindow* paned)
{
    wxASSERT(_paned == NULL); // detect weirdness

    _paned = paned;

    _paned->Bind(wxEVT_SPLITTER_SASH_POS_CHANGED,
                 &PanedPosition::onPositionChange, this);
}

void PanedPosition::disconnect()
{
    if (_paned)
    {
        _paned->Unbind(wxEVT_SPLITTER_SASH_POS_CHANGED,
                       &PanedPosition::onPositionChange, this);

        _paned.Release();
    }
}

void PanedPosition::setPosition(int position)
{
    _position = position;

    if (_paned)
    {
        _paned->SetSashPosition(_position, true);
    }
}

void PanedPosition::saveToPath(const std::string& path)
{
    GlobalRegistry().setAttribute(path, "position", string::to_string(_position));
}

void PanedPosition::loadFromPath(const std::string& path)
{
    setPosition(
        string::convert<int>(GlobalRegistry().getAttribute(path, "position"))
    );
}

void PanedPosition::onPositionChange(wxSplitterEvent& ev)
{
    if (_paned)
    {
        _position = _paned->GetSashPosition();
    }
}

} // namespace
