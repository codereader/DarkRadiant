#pragma once

#include "imapresource.h"
#include "map/MapResource.h"

namespace map
{

class MapResourceManager :
	public IMapResourceManager
{
private:
	ExportEvent _resourceExporting;
	ExportEvent _resourceExported;

public:
	IMapResourcePtr createFromPath(const std::string& path) override;
    IMapResourcePtr createFromArchiveFile(const std::string& archivePath,
        const std::string& filePathWithinArchive) override;

	ExportEvent& signal_onResourceExporting() override;
	ExportEvent& signal_onResourceExported() override;

	// RegisterableModule implementation
	virtual const std::string& getName() const override;
	virtual const StringSet& getDependencies() const override;
	virtual void initialiseModule(const IApplicationContext& ctx) override;
};

}
