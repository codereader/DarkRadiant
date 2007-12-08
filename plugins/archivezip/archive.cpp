/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "idatastream.h"
#include "cmdlib.h"
#include "bytestreamutils.h"

#include "iarchive.h"

#include <algorithm>
#include "stream/filestream.h"
#include "container/array.h"
#include "archivelib.h"
#include "zlibstream.h"

#include "os/path.h"
#include "pkzip.h"

#include <map>
#include "string/string.h"
#include "fs_filesystem.h"
#include "ZipArchive.h"

ArchivePtr OpenArchive(const char* name)
{
  return ArchivePtr(new ZipArchive(name));
}

#if 0

class TestZip
{
  class TestVisitor : public Archive::IVisitor
  {
  public:
    void visit(const char* name)
    {
      int bleh = 0;
    }
  };
public:
  TestZip()
  {
    testzip("c:/quake3/baseq3/mapmedia.pk3", "textures/radiant/notex.tga");
  }

  void testzip(const char* name, const char* filename)
  {
    Archive* archive = OpenArchive(name);
    ArchiveFile* file = archive->openFile(filename);
    if(file != 0)
    {
      unsigned char buffer[4096];
      std::size_t count = file->getInputStream().read((InputStream::byte_type*)buffer, 4096);
      file->release();
    }
    TestVisitor visitor;
    archive->forEachFile(Archive::VisitorFunc(&visitor, Archive::eFilesAndDirectories, 0), "");
    archive->forEachFile(Archive::VisitorFunc(&visitor, Archive::eFilesAndDirectories, 1), "");
    archive->forEachFile(Archive::VisitorFunc(&visitor, Archive::eFiles, 1), "");
    archive->forEachFile(Archive::VisitorFunc(&visitor, Archive::eDirectories, 1), "");
    archive->forEachFile(Archive::VisitorFunc(&visitor, Archive::eFilesAndDirectories, 1), "textures");
    archive->forEachFile(Archive::VisitorFunc(&visitor, Archive::eFilesAndDirectories, 1), "textures/");
    archive->forEachFile(Archive::VisitorFunc(&visitor, Archive::eFilesAndDirectories, 2), "");
    archive->release();
  }
};

TestZip g_TestZip;

#endif
