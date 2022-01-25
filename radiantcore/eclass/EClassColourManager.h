#pragma once

#include <map>
#include "ieclasscolours.h"

namespace eclass
{

class EClassColourManager :
    public IColourManager
{
private:
    std::map<std::string, Vector4> _overrides;
    sigc::signal<void, const std::string&, bool> _overrideChangedSignal;

public:
    // IColourManager implementation

    void addOverrideColour(const std::string& eclass, const Vector4& colour) override;
    bool applyColours(IEntityClass& eclass) override;
    void foreachOverrideColour(const std::function<void(const std::string&, const Vector4&)>& functor) override;
    void removeOverrideColour(const std::string& eclass) override;
    void clearOverrideColours() override;
    sigc::signal<void, const std::string&, bool>& sig_overrideColourChanged() override;

    // RegisterableModule implementation

    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
};

}
