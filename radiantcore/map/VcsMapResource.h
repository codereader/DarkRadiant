#pragma once

#include "iversioncontrol.h"
#include "MapResource.h"

namespace map
{

// A map resource which is referring to a file entry in the VCS history
class VcsMapResource :
    public MapResource
{
private:
    std::string _mapFileUri;
    std::string _infoFileUri;

    vcs::IVersionControlModule::Ptr _vcsModule;

public:
    VcsMapResource(const std::string& mapFileUri);

    virtual bool load() override;
    virtual bool isReadOnly() override;
    virtual void save(const MapFormatPtr& mapFormat = MapFormatPtr()) override;

protected:
    virtual stream::MapResourceStream::Ptr openMapfileStream() override;
    virtual stream::MapResourceStream::Ptr openInfofileStream() override;

private:
    stream::MapResourceStream::Ptr openFileFromVcs(const std::string& uri);
};

}
