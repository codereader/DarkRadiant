#pragma once

#include <sstream>
#include <map>
#include "iselectiongroup.h"
#include "imap.h"
#include "NodeUtils.h"

namespace scene
{

namespace merge
{

class SelectionGroupMergerBase
{
protected:
    std::stringstream _log;

public:
    std::string getLogMessages() const
    {
        return _log.str();
    }

protected:
    using GroupMembers = std::map<std::string, scene::INodePtr>;

    GroupMembers getGroupMemberFingerprints(selection::ISelectionGroup& group)
    {
        GroupMembers members;

        group.foreachNode([&](const INodePtr& member)
        {
            members.emplace(NodeUtils::GetGroupMemberFingerprint(member), member);
        });

        return members;
    }
};

}

}
