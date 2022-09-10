#pragma once

#include "ifilesystem.h"
#include "itextstream.h"
#include "idecltypes.h"
#include "debugging/ScopedDebugTimer.h"
#include "parser/ParseException.h"
#include "parser/ThreadedDefLoader.h"

namespace parser
{

/**
 * Threaded declaration parser, visiting all files associated to the given
 * decl type, processing the files in the correct order.
 */
template <typename ReturnType>
class ThreadedDeclParser :
    public ThreadedDefLoader<ReturnType>
{
private:
    decl::Type _declType;
    std::string _baseDir;
    std::string _extension;
    std::size_t _depth;

protected:
    // Construct a parser traversing all files matching the given extension in the given VFS path
    // Subclasses need to implement the parse(std::istream) overload for this scenario
    ThreadedDeclParser(decl::Type declType, const std::string& baseDir, const std::string& extension, std::size_t depth = 1) :
        ThreadedDefLoader<ReturnType>(std::bind(&ThreadedDeclParser::doParse, this)),
        _baseDir(baseDir),
        _extension(extension),
        _depth(depth),
        _declType(declType)
    {}

public:
    ~ThreadedDeclParser() override
    {
        // Ensure that reset() is called while this class is still intact
        ThreadedDefLoader<ReturnType>::reset();
    }

    // Bypass the threading, and just perform the parse routine
    ReturnType parseSynchronously()
    {
        return doParse();
    }

protected:
    virtual void onBeginParsing() {}
    virtual ReturnType onFinishParsing() { return ReturnType(); }

    // Main parse entry point, process all files
    ReturnType doParse()
    {
        try
        {
            onBeginParsing();

            processFiles();

            return onFinishParsing();
        }
        catch (const std::exception& ex)
        {
            rError() << "Exception in ThreadedDeclParser: " << ex.what() << std::endl;
            throw;
        }
    }

    // Parse all decls found in the given stream, to be implemented by subclasses
    virtual void parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir) = 0;

    void processFiles()
    {
        ScopedDebugTimer timer("[DeclParser] Parsed " + decl::getTypeName(_declType) + " declarations");

        // Accumulate all the files and sort them before calling the protected parse() method
        std::vector<vfs::FileInfo> _incomingFiles;
        _incomingFiles.reserve(200);

        GlobalFileSystem().forEachFile(_baseDir, _extension, [&](const vfs::FileInfo& info)
        {
            _incomingFiles.push_back(info);
        }, _depth);

        // Sort the files by name
        std::sort(_incomingFiles.begin(), _incomingFiles.end(), [](const vfs::FileInfo& a, const vfs::FileInfo& b)
        {
            return a.name < b.name;
        });

        // Dispatch the sorted list to the protected parse() method
        for (const auto& fileInfo : _incomingFiles)
        {
            auto file = GlobalFileSystem().openTextFile(fileInfo.fullPath());

            if (!file) continue;

            try
            {
                // Parse entity defs from the file
                std::istream stream(&file->getInputStream());
                parse(stream, fileInfo, file->getModName());
            }
            catch (ParseException& e)
            {
                rError() << "[DeclParser] Failed to parse " << fileInfo.fullPath()
                    << " (" << e.what() << ")" << std::endl;
            }
        }
    }
};

}
