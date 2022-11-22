#include "PanedPosition.h"

#include "string/convert.h"
#include "registry/registry.h"
#include <wx/splitter.h>

#include "string/predicate.h"

namespace
{
    constexpr int DEFAULT_POSITION = 200;
}

namespace wxutil
{

PanedPosition::PanedPosition(const std::string& name) :
    _name(name),
    _position(DEFAULT_POSITION)
{}

PanedPosition::~PanedPosition()
{
    disconnect();
}

void PanedPosition::connect(wxSplitterWindow* paned)
{
    if (_paned != nullptr)
    {
        disconnect();
    }

    _paned = paned;

    _paned->Bind(wxEVT_SPLITTER_SASH_POS_CHANGED, &PanedPosition::onPositionChange, this);
}

void PanedPosition::disconnect()
{
    if (_paned)
    {
        _paned->Unbind(wxEVT_SPLITTER_SASH_POS_CHANGED, &PanedPosition::onPositionChange, this);

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
    GlobalRegistry().setAttribute(registry::combinePath(path, _name), "position", string::to_string(_position));
}

void PanedPosition::loadFromPath(const std::string& path)
{
    setPosition(string::convert<int>(GlobalRegistry().getAttribute(registry::combinePath(path, _name), "position")));
}

void PanedPosition::onPositionChange(wxSplitterEvent& ev)
{
    if (_paned)
    {
        _position = _paned->GetSashPosition();
    }
}

} // namespace
