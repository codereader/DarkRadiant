# scons build script
# http://scons.sourceforge.net

import commands, re, sys, os, pickle, string, popen2
import tempfile

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

CCFLAGS = POSIXFLAGS + warningFlags + '-fPIC '
CXXFLAGS = '-pipe ' + POSIXFLAGS + warningFlags + warningFlagsCXX + '-fPIC '
LINKFLAGS = ''

CPPPATH = ['radiant', 'include', 'libs']
if (BUILD == 'debug'):
	CXXFLAGS += '-g -D_DEBUG '
	CCFLAGS += '-g -D_DEBUG '
elif (BUILD == 'profile'):
	# Settings for profiling. We want profile data (-pg) and debug data (-g)
	# with basic optimisations to get a better picture of what is happening
	# in "real life". Inlining will confuse the call information however, so
	# this is turned off.
    CXXFLAGS += '-g -pg -O1 -fno-inline '
    CCFLAGS += '-g -pg -O1 -fno-inline '
    LINKFLAGS += '-pg '
elif (BUILD == 'release' or BUILD == 'final'):
	CXXFLAGS += '-O2 '
	CCFLAGS += '-O2 '
	# TEMPORARY HACK: Disable inlining on Windows due to problems with NaNs and certain
	# maths functions causing infinite loops
	#if getOS() == 'win32':
	#	CXXFLAGS += '-fno-inline -fno-default-inline '
	#	CCFLAGS += '-fno-inline -fno-default-inline '
else:
	print 'Unknown build configuration ' + BUILD
	sys.exit( 0 )

if ( getOS() == 'posix' ):
  LINKFLAGS += '-Wl,-fini,fini_stub -L. -static-libgcc -Wl,--export-dynamic '

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

	# Use Boost includes.
	def useBoost(self):
		if (self['PLATFORM'] == 'win32'):
			self.Append(CPPPATH = ['#/boost.w32/include'])
			self.Append(LIBPATH = ['#/boost.w32/lib'])
			
	# Link with the boost_regex library
	def useBoostRegex(self):
		self.useBoost()
		if (self['PLATFORM'] == 'win32'):
			self.Append(LIBS = ['libboost_regex-gcc'])
		else:
			self.Append(LIBS = ['boost_regex'])
	
	# Link with the boost_filesystem library
	def useBoostFilesystem(self):
		self.useBoost()
		if (self['PLATFORM'] == 'win32'):
			self.Append(LIBS = ['libboost_filesystem-gcc-1_33_1'])
		else:
			self.Append(LIBS = ['boost_filesystem'])
	
	# Link with the OpenAL library
	def useOpenAL(self):
		if (self['PLATFORM'] == 'win32'):
			self.Append(CPPPATH = ['#/w32/openal/include', '#/w32/vorbis/include'])
			self.Append(LIBPATH = ['#/w32/openal/lib', '#/w32/vorbis/lib'])
			self.Append(LIBS = ['OpenAL32', 'alut', 'libvorbis', 'vorbisfile'])
		else:
			self.Append(LIBS = ['openal', 'alut', 'vorbisfile'])
	
	def useGlib2(self):
	# On Win32 we need to add the local paths, since there is no
	# global include/lib path.
		if (self['PLATFORM'] == 'win32'):
			self.Append(CPPPATH = ['#/gtk2.w32/include/glib-2.0', '#/gtk2.w32/lib/glib-2.0/include', '#/gtk2.w32/include/cairo'])
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
			self.Append(CPPPATH = ['#/w32/glew/include'])
			self.Append(LIBPATH = ['#/w32/glew/lib'])
			self.Append(LIBS = ['glew32', 'glu32', 'opengl32', 'gdi32' ]) # MinGW libs
		else:
			self.Append(LIBS = ['GLEW', 'GL'])

	def useZLib(self):
		if (self['PLATFORM'] == 'win32'):
			self.Append(CPPPATH = ['#/zlib.w32/include'])
			self.Append(LIBPATH = ['#/zlib.w32/lib'])
		self.Append(LIBS = ['z'])
    
	def buildIconResource(self):
		# build the windows icon resource file
		if sys.platform == 'win32':
			self.RES(['radiant/darkradiant.rc'])

g_env = idEnvironment()

# greebo: Fix for problems with long command lines in Windows/MinGW
if g_env['PLATFORM'] == 'win32':
    import win32file
    import win32event
    import win32process
    import win32security
    import string

    def my_spawn(sh, escape, cmd, args, spawnenv):
        for var in spawnenv:
            spawnenv[var] = spawnenv[var].encode('ascii', 'replace')

        sAttrs = win32security.SECURITY_ATTRIBUTES()
        StartupInfo = win32process.STARTUPINFO()
        newargs = string.join(map(escape, args[1:]), ' ')
        cmdline = cmd + " " + newargs

        # check for any special operating system commands
        if cmd == 'del':
            for arg in args[1:]:
                win32file.DeleteFile(arg)
            exit_code = 0
        else:
            # otherwise execute the command.
            hProcess, hThread, dwPid, dwTid = win32process.CreateProcess(None, cmdline, None, None, 1, 0, spawnenv, None, StartupInfo)
            win32event.WaitForSingleObject(hProcess, win32event.INFINITE)
            exit_code = win32process.GetExitCodeProcess(hProcess)
            win32file.CloseHandle(hProcess);
            win32file.CloseHandle(hThread);
        return exit_code

    g_env['SPAWN'] = my_spawn

g_env.useBoost()

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

# end targets ------------------------------------

################################################################################
# POST-PROCESSING
#
# Build Linux packages if required

packageType = ARGUMENTS.get('build_package', None)
if packageType == 'deb':
    
    print "Building Debian package..."

    # Build a Debian package. We need a temporary directory into which the
    # installation tree and DEBIAN/control will be copied.
    tempDir = tempfile.mkdtemp()
    os.system('mkdir -p %s/usr/local' % tempDir)
    os.system('cp -R install %s/usr/local/darkradiant' % tempDir)
    os.system('cp -R tools/linux/DEBIAN %s' % tempDir)
    os.system('dpkg-deb -b %s darkradiant.deb' % tempDir)
    os.system('rm -rf %s' % tempDir)
