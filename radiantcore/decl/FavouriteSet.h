#pragma once

#include <string>
#include <set>
#include "iregistry.h"
#include <sigc++/signal.h>

namespace decl
{

class FavouriteSet
{
private:
    std::set<std::string> _set;

    sigc::signal<void> _sigSetChanged;

public:
    std::set<std::string>& get()
    {
        return _set;
    }

    const std::set<std::string>& get() const
    {
        return _set;
    }

    void loadFromRegistry(const std::string& rootPath)
    {
        xml::NodeList favourites = GlobalRegistry().findXPath(rootPath + "//favourite");

        for (xml::Node& node : favourites)
        {
            _set.insert(node.getAttributeValue("value"));
        }
    }

    void saveToRegistry(const std::string& rootPath)
    {
        GlobalRegistry().deleteXPath(rootPath + "//favourite");

        xml::Node favourites = GlobalRegistry().createKey(rootPath);

        for (const auto& favourite : _set)
        {
            xml::Node node = favourites.createChild("favourite");
            node.setAttributeValue("value", favourite);
        }
    }

    sigc::signal<void>& signal_setChanged()
    {
        return _sigSetChanged;
    }
};

}
