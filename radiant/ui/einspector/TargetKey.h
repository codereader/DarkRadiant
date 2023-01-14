#pragma once

#include <regex>
#include <stdexcept>
#include "ui/ientityinspector.h"
#include "fmt/format.h"

namespace ui
{

class TargetKey :
    public ITargetKey
{
public:
    static constexpr const char* const SetKeyPattern = "^set (\\w+) on (\\w+)$";

private:
    std::string _affectedKey;
    std::string _attachmentName;

public:
    std::string getFullKey() const override
    {
        return _attachmentName.empty() ? _affectedKey :
            fmt::format("set {0} on {1}", _affectedKey, _attachmentName);
    }

    const std::string& getAffectedKey() const override
    {
        return _affectedKey;
    }

    void setAffectedKey(const std::string& affectedKey) override
    {
        if (affectedKey.empty()) throw std::invalid_argument("Affected key must not be empty");

        _affectedKey = affectedKey;
    }

    bool isTargetingAttachment() const override
    {
        return !_attachmentName.empty();
    }

    const std::string& getAttachmentName() const override
    {
        return _attachmentName;
    }

    void setAttachmentName(const std::string& name) override
    {
        _attachmentName = name;
    }

    Ptr clone() const override
    {
        return std::make_shared<TargetKey>(*this);
    }

    static Ptr CreateFromString(const std::string& key)
    {
        auto instance = std::make_shared<TargetKey>();

        static std::regex _pattern(SetKeyPattern, std::regex::icase);

        std::smatch match;
        if (std::regex_match(key, match, _pattern))
        {
            instance->setAffectedKey(match[1].str());
            instance->setAttachmentName(match[2].str());
        }
        else
        {
            instance->setAffectedKey(key);
        }

        return instance;
    }
};

}
