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
    std::string _uri;
    std::string _revision;
    std::string _filePath;

    vcs::IVersionControlModule::Ptr _vcsModule;

public:
    VcsMapResource(const std::string& uri);

    virtual bool isReadOnly() override;
    virtual void save(const MapFormatPtr& mapFormat = MapFormatPtr()) override;

protected:
    virtual stream::MapResourceStream::Ptr openMapfileStream() override;
    virtual stream::MapResourceStream::Ptr openInfofileStream() override;

private:
    stream::MapResourceStream::Ptr openFileFromVcs(const std::string& revision, const std::string& filePath);
};

}
