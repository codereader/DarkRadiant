#pragma once

#include <string>
#include <set>
#include "iregistry.h"
#include <sigc++/signal.h>

namespace game
{

class FavouriteSet
{
private:
    std::string _typeName;
    std::set<std::string> _set;

    sigc::signal<void> _sigSetChanged;

public:
    FavouriteSet() :
        FavouriteSet("")
    {}

    FavouriteSet(const std::string& typeName) :
        _typeName(typeName)
    {}

    std::set<std::string>& get()
    {
        return _set;
    }

    const std::set<std::string>& get() const
    {
        return _set;
    }

    void add(FavouriteSet& other)
    {
        _set.insert(other.get().begin(), other.get().end());
    }

    void loadFromRegistry(const std::string& rootPath)
    {
        auto path = _typeName.empty() ? rootPath : rootPath + "/" + _typeName;

        auto favourites = GlobalRegistry().findXPath(path + "//favourite");

        for (const auto& node : favourites)
        {
            _set.insert(node.getAttributeValue("value"));
        }
    }

    void saveToRegistry(const std::string& rootPath) const
    {
        auto path = _typeName.empty() ? rootPath : rootPath + "/" + _typeName;
        GlobalRegistry().deleteXPath(path + "//favourite");

        auto favourites = GlobalRegistry().createKey(path);

        for (const auto& favourite : _set)
        {
            auto node = favourites.createChild("favourite");
            node.setAttributeValue("value", favourite);
        }
    }

    sigc::signal<void>& signal_setChanged()
    {
        return _sigSetChanged;
    }
};

}
