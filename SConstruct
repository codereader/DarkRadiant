# scons build script
# http://scons.sourceforge.net

import commands, re, sys, os, pickle, string, popen2
from makeversion import radiant_makeversion, get_version

# to access some internal stuff
import SCons

conf_filename='site.conf'
# there is a default hardcoded value, you can override on command line, those are saved between runs
# we only handle strings
serialized=['CC', 'CXX', 'JOBS', 'BUILD']

# help -------------------------------------------

Help("""
Usage: scons [OPTIONS] [TARGET] [CONFIG]

[OPTIONS] and [TARGET] are covered in command line options, use scons -H

[CONFIG]: KEY="VALUE" [...]
a number of configuration options saved between runs in the """ + conf_filename + """ file
erase """ + conf_filename + """ to start with default settings again

CC
CXX
	Specify C and C++ compilers (defaults gcc and g++)
	ex: CC="gcc-3.2"
	You can use ccache and distcc, for instance:
	CC="ccache distcc gcc" CXX="ccache distcc g++"

JOBS
	Parallel build
	ex: JOBS="4" is a good setting on SMP machines

BUILD
	Use debug/release to select build settings
	ex: BUILD="release" - default is debug
"""
)

# end help ---------------------------------------
  
# sanity -----------------------------------------

# get a recent python release
# that is broken in current version:
# http://sourceforge.net/tracker/index.php?func=detail&aid=794145&group_id=30337&atid=398971
#EnsurePythonVersion(2,1)
# above 0.90
EnsureSConsVersion( 0, 96 )
#print 'SCons ' + SCons.__version__

# end sanity -------------------------------------

# system detection -------------------------------

# Get the output of a shell command. We cannot use commands.getoutput
# on Windows.

def runCmd(cmd):
	stream = os.popen(cmd)
	return (stream.read(), stream.close()) # return the output and the exit status

# OS
def getOS():
	return Environment()['PLATFORM']

# CPU type
g_cpu = runCmd('uname -m')[0]
exp = re.compile('.*i?86.*')
if (g_cpu == 'Power Macintosh' or g_cpu == 'ppc'):
  g_cpu = 'ppc'
elif exp.match(g_cpu):
  g_cpu = 'x86'
else:
  g_cpu = 'cpu'

if (getOS() == 'posix'):
  # libc .. do the little magic!
  libc = commands.getoutput('/lib/libc.so.6 |grep "GNU C "|grep version|awk -F "version " \'{ print $2 }\'|cut -b -3') # OK to use this on posix

# end system detection ---------------------------

# default settings -------------------------------

CC='gcc'
CXX='g++'
JOBS='1'
BUILD='debug'
INSTALL='#install'
g_build_root = 'build'

# end default settings ---------------------------

# site settings ----------------------------------

site_dict = {}
if (os.path.exists(conf_filename)):
	site_file = open(conf_filename, 'r')
	p = pickle.Unpickler(site_file)
	site_dict = p.load()
#	print 'Loading build configuration from ' + conf_filename
	for k, v in site_dict.items():
		exec_cmd = k + '=\"' + v + '\"'
#		print exec_cmd
		exec(exec_cmd)

# end site settings ------------------------------

# command line settings --------------------------

for k in serialized:
	if (ARGUMENTS.has_key(k)):
		exec_cmd = k + '=\"' + ARGUMENTS[k] + '\"'
		print 'Command line: ' + exec_cmd
		exec(exec_cmd)

# end command line settings ----------------------

# sanity check -----------------------------------


def GetGCCVersion(name):
  ret = runCmd('%s -dumpversion' % name)
  if ( ret[1] != None ):
    return None
  vers = string.split(ret[0].strip(), '.')
  if ( len(vers) == 2 ):
    return [ vers[0], vers[1], 0 ]
  elif ( len(vers) == 3 ):
    return vers
  return None

ver_cc = GetGCCVersion(CC)
ver_cxx = GetGCCVersion(CXX)

if ( ver_cc is None or ver_cxx is None or ver_cc[0] < '3' or ver_cxx[0] < '3' or ver_cc != ver_cxx ):
  print 'Compiler version check failed - need gcc 3.x or later:'
  print 'CC: %s %s\nCXX: %s %s' % ( CC, repr(ver_cc), CXX, repr(ver_cxx) )
  Exit(1)

# end sanity check -------------------------------

# save site configuration ----------------------

for k in serialized:
	exec_cmd = 'site_dict[\'' + k + '\'] = ' + k
	exec(exec_cmd)

site_file = open(conf_filename, 'w')
p = pickle.Pickler(site_file)
p.dump(site_dict)
site_file.close()

# end save site configuration ------------------

# general configuration, target selection --------

SConsignFile( "scons.signatures" )

g_build = g_build_root + '/' + BUILD

SetOption('num_jobs', JOBS)

LINK = CXX
# common flags
warningFlags = '-W -Wall -Wcast-align -Wcast-qual -Wno-unused-parameter '
warningFlagsCXX = '-Wno-non-virtual-dtor -Wreorder ' # -Wold-style-cast
if getOS() == 'posix':
	POSIXFLAGS = '-DPOSIX -DXWINDOWS '
else:
	POSIXFLAGS = ''
# POSIX macro: platform supports posix IEEE Std 1003.1:2001
# XWINDOWS macro: platform supports X-Windows API
CCFLAGS = POSIXFLAGS + warningFlags
CXXFLAGS = '-pipe ' + POSIXFLAGS + warningFlags + warningFlagsCXX
CPPPATH = ['radiant']
if (BUILD == 'debug'):
	CXXFLAGS += '-g -D_DEBUG '
	CCFLAGS += '-g -D_DEBUG '
elif (BUILD == 'release' or BUILD == 'final'):
	CXXFLAGS += '-O2 -fno-inline -fno-default-inline '
	CCFLAGS += '-O2 -fno-inline -fno-default-inline '
else:
	print 'Unknown build configuration ' + BUILD
	sys.exit( 0 )

LINKFLAGS = ''
if ( getOS() == 'posix' ):

  # static
  # 2112833 /opt/gtkradiant/radiant.x86
  # 35282 /opt/gtkradiant/modules/archivezip.so
  # 600099 /opt/gtkradiant/modules/entity.so
  
  # dynamic
  # 2237060 /opt/gtkradiant/radiant.x86
  # 110605 /opt/gtkradiant/modules/archivezip.so
  # 730222 /opt/gtkradiant/modules/entity.so
  
  # EVIL HACK - force static-linking for libstdc++ - create a symbolic link to the static libstdc++ in the root
  os.system("ln -s `g++ -print-file-name=libstdc++.a`")
  
  #if not os.path.exists("./install"):
  #  os.mkdir("./install")
  #os.system("cp `g++ -print-file-name=libstdc++.so` ./install")
  
  #CXXFLAGS += '-fno-rtti '
  LINKFLAGS += '-Wl,-fini,fini_stub -L. -static-libgcc '

CPPPATH.append('libs')

# extend the standard Environment a bit

# Configure the toolchain. On Win32 we want to force MinGW as the default
# is to prefer MSVC which breaks the build.

if (getOS() == 'win32'):
	toolList = ['mingw', 'g++']
else:
	toolList = ['default']

class idEnvironment(Environment):

	def __init__(self):
		Environment.__init__(self,
			ENV = os.environ, 
			CC = CC,
			CXX = CXX,
			LINK = LINK,
			CCFLAGS = CCFLAGS,
			CXXFLAGS = CXXFLAGS,
			CPPPATH = CPPPATH,
			LINKFLAGS = LINKFLAGS,
			TOOLS = toolList) # don't want Scons to choose MSVC on Win32

	def useBoost(self):
		if (self['PLATFORM'] == 'win32'):
			self.Append(CPPPATH = ['#/boost.w32/include'])
			self.Append(LIBPATH = ['#/boost.w32/lib'])
			self.Append(LIBS = ['libboost_regex-gcc'])
		else:
			self.Append(LIBS = ['boost_regex'])
	
	def useGlib2(self):
	# On Win32 we need to add the local paths, since there is no
	# global include/lib path.
		if (self['PLATFORM'] == 'win32'):
			self.Append(CPPPATH = ['#/gtk2.w32/include/glib-2.0', '#/gtk2.w32/lib/glib-2.0/include'])
			self.Append(LIBPATH = ['#/gtk2.w32/lib'])
			self.Append(LIBS = ['glib-2.0', 'gobject-2.0'])
		else: # Assume Linux
			self.Append(CXXFLAGS = '`pkg-config glib-2.0 --cflags` ')
			self.Append(CFLAGS = '`pkg-config glib-2.0 --cflags` ')
			self.Append(LINKFLAGS = '`pkg-config glib-2.0 --libs` ')

	def useW32Iconv(self):
		self.Append(LIBPATH = ['#/libiconv.w32/lib'])
		self.Append(LIBS = ['iconv'])

	def useXML2(self):
		if (self['PLATFORM'] == 'win32'):
			self.Append(CCFLAGS = '-DLIBXML_STATIC ')
			self.Append(CXXFLAGS = '-DLIBXML_STATIC ')
			self.Append(CPPPATH = ['#/libxml2.w32/include'])
			self.Append(LIBPATH = ['#/libxml2.w32/lib'])
			self.Append(LIBS = ['libxml2'])
			self.useZLib()
			self.useW32Iconv()
			#self.useMSVC()
		else: # Linux
			self.Append(CXXFLAGS = '`xml2-config --cflags` ')
			self.Append(CFLAGS = '`xml2-config --cflags` ')
			self.Append(LINKFLAGS = '`xml2-config --libs` ')

	def useGtk2(self):
		if (self['PLATFORM'] == 'win32'):
			self.Append(CPPPATH = ['#/gtk2.w32/include/gtk-2.0', '#/gtk2.w32/lib/gtk-2.0/include', '#/gtk2.w32/include/pango-1.0', '#/gtk2.w32/include/atk-1.0'])
			self.Append(LIBPATH = ['#/gtk2.w32/lib'])
			self.Append(LIBS = ['gtk-win32-2.0', 'gdk-win32-2.0', 'atk-1.0', 'pango-1.0', 'pangowin32-1.0', 'gdk_pixbuf-2.0'])
			self.Append(CXXFLAGS = '-mms-bitfields ')
			self.Append(CFLAGS = '-mms-bitfields ')
		else: # Assume X11
			self.Append(CXXFLAGS = '`pkg-config gtk+-2.0 --cflags` ')
			self.Append(CFLAGS = '`pkg-config gtk+-2.0 --cflags` ')
			self.Append(LINKFLAGS = '`pkg-config gtk+-2.0 --libs` ')
   
	def useGtkGLExt(self):
		if (self['PLATFORM'] == 'win32'):
			self.Append(CPPPATH = ['#/gtk2.w32/include/gtkglext-1.0', '#/gtk2.w32/lib/gtkglext-1.0/include'])
			self.Append(LIBPATH = ['#/gtk2.w32/lib'])
			self.Append(LIBS = ['gtkglext-win32-1.0', 'gdkglext-win32-1.0'])
		else: # Assume X11
			self.Append(CXXFLAGS = '`pkg-config gtkglext-1.0 --cflags` ')
			self.Append(CFLAGS = '`pkg-config gtkglext-1.0 --cflags` ')
			self.Append(LIBS = ['gtkglext-x11-1.0', 'gdkglext-x11-1.0', 'GLU'])
    
	def useOpenGL(self):
		if (self['PLATFORM'] == 'win32'):
			self.Append(LIBS = ['opengl32', 'gdi32']) # MinGW libs
		else:
			self.Append(LIBS = ['GL'])

	def usePNG(self):
		self.useZLib() # libPNG requires ZLib
		if (self['PLATFORM'] == 'win32'):
			self.Append(CPPPATH = ['#/libpng.w32/include'])
			self.Append(LIBPATH = ['#/libpng.w32/lib'])
		else: # Linux
			self.Append(CXXFLAGS = '`libpng-config --cflags` ')
			self.Append(CFLAGS = '`libpng-config --cflags` ')
		self.Append(LIBS = ['png'])
    
	def useMHash(self):
		self['LINKFLAGS'] += '-lmhash '
  
	def useZLib(self):
		if (self['PLATFORM'] == 'win32'):
			self.Append(CPPPATH = ['#/zlib.w32/include'])
			self.Append(LIBPATH = ['#/zlib.w32/lib'])
		self.Append(LIBS = ['z'])
    
	def usePThread(self):
		if ( getOS() == 'Darwin' ):
			self['LINKFLAGS'] += '-lpthread -Wl,-stack_size,0x400000 '
		else:
			self['LINKFLAGS'] += '-lpthread '

g_env = idEnvironment()

# export the globals
GLOBALS = 'g_env INSTALL g_cpu'

#radiant_makeversion('\\ngcc version: %s.%s.%s' % ( ver_cc[0], ver_cc[1], ver_cc[2] ) )

# end general configuration ----------------------

# targets ----------------------------------------

Default('.')

SourceSignatures('timestamp')
Export('GLOBALS ' + GLOBALS)
BuildDir(g_build, '.', duplicate = 0)
SConscript(g_build + '/SConscript')
#os.system('python ./install.py')

# end targets ------------------------------------
