#pragma once

#include "imapformat.h"
#include <boost/enable_shared_from_this.hpp>

namespace map
{

namespace
{
	const float MAP_VERSION_D3 = 2;
}

class Doom3MapFormat :
	public MapFormat,
	public boost::enable_shared_from_this<Doom3MapFormat>
{
public:
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	virtual void shutdownModule();

	virtual const std::string& getMapFormatName() const;
	virtual const std::string& getGameType() const;
	virtual IMapReaderPtr getMapReader(IMapImportFilter& filter) const;
	virtual IMapWriterPtr getMapWriter() const;

	virtual bool allowInfoFileCreation() const;

	virtual bool canLoad(std::istream& stream) const;
};
typedef boost::shared_ptr<Doom3MapFormat> Doom3MapFormatPtr;

} // namespace map
