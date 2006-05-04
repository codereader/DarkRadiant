import os, sys, commands, string
from makeversion import get_version
# OS Detection:
OS = commands.getoutput('uname')

Import('GLOBALS')
Import(GLOBALS)

def build_list(s_prefix, s_string):
    s_list = Split(s_string)
    for i in range(len(s_list)):
        s_list[i] = s_prefix + '/' + s_list[i]
    return s_list

# common code ------------------------------------------------------

exceptionLibSource = 'RadiantException.cpp ModuleSystemException.cpp'
exceptionLib = g_env.StaticLibrary(target='libs/exception', source=build_list('libs/exception', exceptionLibSource))

cmdlib_lib = g_env.StaticLibrary(target='libs/cmdlib', source='libs/cmdlib/cmdlib.cpp')

mathlib_src = 'mathlib.c bbox.c line.c m4x4.c ray.c'
mathlib_lib = g_env.StaticLibrary(target='libs/mathlib', source=build_list('libs/mathlib', mathlib_src))

md5lib_lib = g_env.StaticLibrary(target='libs/md5lib', source='libs/md5lib/md5lib.c')

ddslib_lib = g_env.StaticLibrary(target='libs/ddslib', source='libs/ddslib/ddslib.c')

jpeg_env = g_env.Copy()
jpeg_env.Prepend(CPPPATH = 'libs/jpeg6')
jpeg_src = 'jcomapi.cpp jdcoefct.cpp jdinput.cpp jdpostct.cpp jfdctflt.cpp jpgload.cpp jdapimin.cpp jdcolor.cpp jdmainct.cpp jdsample.cpp jidctflt.cpp jutils.cpp jdapistd.cpp jddctmgr.cpp jdmarker.cpp jdtrans.cpp jmemmgr.cpp jdatasrc.cpp jdhuff.cpp jdmaster.cpp jerror.cpp jmemnobs.cpp'
jpeg_lib = jpeg_env.StaticLibrary(target='libs/jpeg6', source=build_list('libs/jpeg6', jpeg_src))

l_net_lib = g_env.StaticLibrary(target='libs/l_net', source=['libs/l_net/l_net.c', 'libs/l_net/l_net_berkley.c'])

picomodel_src = 'picointernal.c picomodel.c picomodules.c pm_3ds.c pm_ase.c pm_md3.c pm_obj.c\
  pm_ms3d.c pm_mdc.c pm_fm.c pm_md2.c pm_lwo.c pm_terrain.c lwo/clip.c lwo/envelope.c lwo/list.c lwo/lwio.c\
  lwo/lwo2.c lwo/lwob.c lwo/pntspols.c lwo/surface.c lwo/vecmath.c lwo/vmap.c'
picomodel_lib = g_env.StaticLibrary(target='libs/picomodel', source=build_list('libs/picomodel', picomodel_src))

#splines_env = g_env.Copy()
#splines_src = build_list('libs/splines', 'math_angles.cpp math_matrix.cpp math_quaternion.cpp math_vector.cpp q_parse.cpp q_shared.cpp splines.cpp util_str.cpp')
#splines_env['CPPPATH'].append('include')
#splines_lib = splines_env.StaticLibrary(target='libs/splines', source=splines_src)

profile_env = g_env.Copy();
profile_env['CPPPATH'].append('include')
profile_src = 'profile.cpp file.cpp'
profile_lib = profile_env.StaticLibrary(target='libs/profile', source=build_list('libs/profile', profile_src))

gtkutil_env = g_env.Copy();
gtkutil_env['CPPPATH'].append('include')
gtkutil_env.useGlib2()
gtkutil_env.useGtk2()
gtkutil_env.useGtkGLExt()

gtkutil_src = '\
  accelerator.cpp\
  button.cpp\
  clipboard.cpp\
  closure.cpp\
  container.cpp\
  cursor.cpp\
  dialog.cpp\
  entry.cpp\
  frame.cpp\
  filechooser.cpp\
  glfont.cpp\
  glwidget.cpp\
  image.cpp\
  idledraw.cpp\
  menu.cpp\
  messagebox.cpp\
  nonmodal.cpp\
  paned.cpp\
  pointer.cpp\
  toolbar.cpp\
  widget.cpp\
  window.cpp\
  xorrectangle.cpp\
'

gtkutil_lib = gtkutil_env.StaticLibrary(target='libs/gtkutil', source=build_list('libs/gtkutil', gtkutil_src))

# end static / common libraries ---------------------------------------------------

# radiant, modules and plugins ----------------------------------------------------

module_env = g_env.Copy()
module_env['CPPPATH'].append('include')
if (module_env['PLATFORM'] == 'posix'):
    module_env['LINKFLAGS'] += '-ldl ' # do we need this library?
if (module_env['PLATFORM'] == 'win32'):
    module_env['LINKFLAGS'] += '-Wl,--kill-at ' # must not append @n to exported DLL symbols otherwise Radiant will not find them
module_env['LIBPREFIX'] = ''
module_env.Append(LIBPATH = ['libs'])
module_env['no_import_lib'] = 1

vfspk3_env = module_env.Copy()
vfspk3_lst = build_list('plugins/vfspk3', 'vfspk3.cpp vfs.cpp archive.cpp')
vfspk3_env.useGlib2()
vfspk3_env.Append(LIBS = ['exception'])
vfspk3_lib = vfspk3_env.SharedLibrary(target='vfspk3', source=vfspk3_lst, no_import_lib=1, WIN32_INSERT_DEF=0)
vfspk3_env.Depends(vfspk3_lib, exceptionLib)
vfspk3_env.Install(INSTALL + '/modules', vfspk3_lib)

archivepak_env = module_env.Copy()
archivepak_lst = build_list('plugins/archivepak', 'plugin.cpp archive.cpp pak.cpp')
archivepak_lib = archivepak_env.SharedLibrary(target='archivepak', source=archivepak_lst, LIBS='cmdlib', LIBPATH='libs', no_import_lib=1, WIN32_INSERT_DEF=0)
archivepak_env.Depends(archivepak_lib, cmdlib_lib)
archivepak_env.Install(INSTALL + '/modules', archivepak_lib)

archivewad_env = module_env.Copy()
archivewad_lst = build_list('plugins/archivewad', 'plugin.cpp archive.cpp wad.cpp')
archivewad_lib = archivewad_env.SharedLibrary(target='archivewad', source=archivewad_lst, LIBS='cmdlib', LIBPATH='libs', no_import_lib=1, WIN32_INSERT_DEF=0)
archivewad_env.Depends(archivewad_lib, cmdlib_lib)
archivewad_env.Install(INSTALL + '/modules', archivewad_lib)

archivezip_env = module_env.Copy()
archivezip_lst = build_list('plugins/archivezip', 'plugin.cpp archive.cpp pkzip.cpp zlibstream.cpp')
archivezip_env.useZLib()
archivezip_env.Append(LIBPATH = ['libs'])
archivezip_env.Append(LIBS = ['cmdlib'])
archivezip_lib = archivezip_env.SharedLibrary(target='archivezip', source=archivezip_lst, no_import_lib=1, WIN32_INSERT_DEF=0)
archivezip_env.Depends(archivezip_lib, cmdlib_lib)
archivezip_env.Install(INSTALL + '/modules', archivezip_lib)

shaders_env = module_env.Copy()
shaders_lst = build_list('plugins/shaders', 'shaders.cpp plugin.cpp')
shaders_env.useGlib2()
shaders_env.Append(LIBS = ['cmdlib'])
shaders_env.Append(LIBPATH = ['libs'])
shaders_lib = shaders_env.SharedLibrary(target='shaders', source=shaders_lst, no_import_lib=1, WIN32_INSERT_DEF=0)
shaders_env.Depends(shaders_lib, cmdlib_lib)
shaders_env.Install(INSTALL + '/modules', shaders_lib)

image_env = module_env.Copy()
image_lst = build_list('plugins/image', 'bmp.cpp jpeg.cpp image.cpp pcx.cpp tga.cpp dds.cpp')
image_lib = image_env.SharedLibrary(target='image', source=image_lst, LIBS=['jpeg6', 'ddslib'], LIBPATH='libs', no_import_lib=1, WIN32_INSERT_DEF=0)
image_env.Depends(image_lib, jpeg_lib)
image_env.Depends(image_lib, ddslib_lib)
image_env.Install(INSTALL + '/modules', image_lib)

# We DO need this, it is used as mapdoom3 in the editor
# - OrbWeaver
mapq3_env = module_env.Copy()
mapq3_lst=build_list('plugins/mapq3', 'plugin.cpp parse.cpp write.cpp')
mapq3_lib = mapq3_env.SharedLibrary(target='mapq3', source=mapq3_lst, LIBS='cmdlib', LIBPATH='libs')
mapq3_env.Depends(mapq3_lib, cmdlib_lib)
mapq3_env.Install(INSTALL + '/modules', mapq3_lib)

imagepng_env = module_env.Copy()
imagepng_lst = build_list('plugins/imagepng', 'plugin.cpp')
imagepng_env.usePNG()
imagepng_lib = imagepng_env.SharedLibrary(target='imagepng', source=imagepng_lst, no_import_lib=1, WIN32_INSERT_DEF=0)
imagepng_env.Install(INSTALL + '/modules', imagepng_lib)

#Does not yet build on Win32, probably only needed for Q3
#mapxml_env = module_env.Copy()
#mapxml_lst = build_list('plugins/mapxml', 'plugin.cpp xmlparse.cpp xmlwrite.cpp')
#mapxml_env.useXML2()
#mapxml_env.useGlib2()
#mapxml_lib = mapxml_env.SharedLibrary(target='mapxml', source=mapxml_lst)
#mapxml_env.Install(INSTALL + '/modules', mapxml_lib)

model_env = module_env.Copy()
model_lst = build_list('plugins/model', 'plugin.cpp model.cpp')
model_lib = model_env.SharedLibrary(target='model', source=model_lst, LIBS=['mathlib', 'picomodel'], LIBPATH='libs', no_import_lib=1, WIN32_INSERT_DEF=0)
model_env.Depends(model_lib, mathlib_lib)
model_env.Depends(model_lib, picomodel_lib)
model_env.Install(INSTALL + '/modules', model_lib)

md3model_lst=build_list('plugins/md3model', 'plugin.cpp mdl.cpp md3.cpp md2.cpp mdc.cpp mdlimage.cpp md5.cpp')
md3model_lib = module_env.SharedLibrary(target='md3model', source=md3model_lst, no_import_lib=1, WIN32_INSERT_DEF=0)
module_env.Install(INSTALL + '/modules', md3model_lib)

entity_lst = build_list('plugins/entity', 'plugin.cpp entity.cpp eclassmodel.cpp generic.cpp group.cpp light.cpp miscmodel.cpp doom3group.cpp skincache.cpp angle.cpp angles.cpp colour.cpp filters.cpp model.cpp namedentity.cpp origin.cpp scale.cpp targetable.cpp rotation.cpp modelskinkey.cpp')
entity_lib = module_env.SharedLibrary(target='entity', source=entity_lst, no_import_lib=1, WIN32_INSERT_DEF=0)
module_env.Install(INSTALL + '/modules', entity_lib)

radiant_env = g_env.Copy()
radiant_env['CPPPATH'].append('include')
if radiant_env['PLATFORM'] == 'posix':
    radiant_env['LINKFLAGS'] += '-ldl '
if ( OS == 'Darwin' ):
  radiant_env['CXXFLAGS'] += '-fno-common '
  radiant_env['CCFLAGS'] += '-fno-common '
  radiant_env['LINKFLAGS'] += '-lX11 -lGL -lGLU '
radiant_env['LIBPREFIX'] = ''
radiant_env.useGlib2()
radiant_env.useXML2()
radiant_env.useGtk2()
radiant_env.useGtkGLExt()
radiant_env.useOpenGL()

radiant_src = [
'autosave.cpp',
'brush.cpp',
'brushmanip.cpp',
'brushmodule.cpp',
'brushnode.cpp',
'brushtokens.cpp',
'brushxml.cpp',
'brush_primit.cpp',
'build.cpp',
'camwindow.cpp',
'clippertool.cpp',
'commands.cpp',
'console.cpp',
'csg.cpp',
'dialog.cpp',
'eclass.cpp',
'eclass_def.cpp',
'eclass_doom3.cpp',
'eclass_fgd.cpp',
'eclass_xml.cpp',
'entity.cpp',
'entityinspector.cpp',
'entitylist.cpp',
'environment.cpp',
'error.cpp',
'feedback.cpp',
'filetypes.cpp',
'filters.cpp',
'findtexturedialog.cpp',
'glwidget.cpp',
'grid.cpp',
'groupdialog.cpp',
'gtkdlgs.cpp',
'gtkmisc.cpp',
'help.cpp',
'image.cpp',
'main.cpp',
'mainframe.cpp',
'map.cpp',
'mru.cpp',
'nullmodel.cpp',
'parse.cpp',
'patch.cpp',
'patchdialog.cpp',
'patchmanip.cpp',
'patchmodule.cpp',
'plugin.cpp',
'pluginapi.cpp',
'pluginmanager.cpp',
'pluginmenu.cpp',
'plugintoolbar.cpp',
'points.cpp',
'preferencedictionary.cpp',
'preferences.cpp',
'qe3.cpp',
'qgl.cpp',
'referencecache.cpp',
'renderer.cpp',
'renderstate.cpp',
'scenegraph.cpp',
'select.cpp',
'selection.cpp',
'server.cpp',
'shaders.cpp',
'surfacedialog.cpp',
'texmanip.cpp',
'textures.cpp',
'texwindow.cpp',
'timer.cpp',
'treemodel.cpp',
'undo.cpp',
'url.cpp',
'view.cpp',
'watchbsp.cpp',
'winding.cpp',
'windowobservers.cpp',
'xmlstuff.cpp',
'xywindow.cpp',
'dialog/einspector/EntityInspector.cpp',
'dialog/einspector/EntityKeyValueVisitor.cpp'
]

for i in range(len(radiant_src)):
  radiant_src[i] = 'radiant/' + radiant_src[i]

radiant_env.Prepend(LIBS = ['mathlib', 'cmdlib', 'profile', 'gtkutil', 'l_net', 'exception'])
radiant_env.Prepend(LIBPATH = ['libs'])

# Win32 libs

if radiant_env['PLATFORM'] == 'win32':
    radiant_env.Append(LIBS = ['ws2_32', 'comdlg32'])
    radiant_src.append('radiant/multimon.cpp')

radiant_prog = radiant_env.Program(target='darkradiant', source=radiant_src)
radiant_env.Depends(radiant_prog, mathlib_lib)
radiant_env.Depends(radiant_prog, cmdlib_lib)
radiant_env.Depends(radiant_prog, l_net_lib)
radiant_env.Depends(radiant_prog, profile_lib)
radiant_env.Depends(radiant_prog, gtkutil_lib)
radiant_env.Depends(radiant_prog, exceptionLib)
radiant_env.Install(INSTALL, radiant_prog)

# Radiant post-install

if radiant_env['PLATFORM'] == 'win32':
    radiant_env.Install(INSTALL, '#libxml2.w32/lib/libxml2.dll')

# setup -------------------------------------------------------------------------------------------

class setup_builder:

  g_dryrun = 0
  
  def system(self, cmd):
    if (self.g_dryrun):
      print cmd
    else:
      sys.stdout.write(cmd)
      ret = commands.getstatusoutput(cmd)
      print ret[1]
      if (ret[0] != 0):
        raise 'command failed'

  def copy_core(self):
    # binaries and misc
    self.system('mkdir -p %s/modules' % self.SETUP_BIN_DIR)
    self.system('mkdir -p %s/plugins' % self.SETUP_BIN_DIR)
    self.system('cp install/%s %s' % (self.EDITOR_BIN, self.SETUP_BIN_DIR))
    self.system('cp install/modules/*.so %s/modules' % self.SETUP_BIN_DIR )
#    self.system('cp install/plugins/*.so %s/plugins' % self.SETUP_BIN_DIR )
    self.system('cp install/q3map2.%s %s' % ( g_cpu, self.SETUP_BIN_DIR ) )
    self.M4_STDC = ''
    if (not self.g_darwin):    
      # fugly
      # copy libgcc_s and stdc++ over to distribute it and reduce potential ABI fuckups
      ret = commands.getstatusoutput('ldd -r install/' + self.EDITOR_BIN + ' 2>/dev/null | grep libgcc_s | sed -e \'s/.* => \\([^ ]*\\) .*/\\1/\'')
      if (ret[0] != 0):
        raise 'ldd command failed'
      self.system('cp ' + ret[1] + ' ' + self.SETUP_BIN_DIR)
      ret = commands.getstatusoutput('ldd -r install/' + self.EDITOR_BIN + ' 2>/dev/null | grep libstdc++ | sed -e \'s/.* => \\([^ ]*\\) .*/\\1/\'')
      if (ret[0] != 0):
        raise 'ldd command failed'
      lines = string.split(ret[1], '\n')
      self.M4_STDC = '"' 
      for i in lines:
        self.system('cp ' + i + ' ' + self.SETUP_BIN_DIR)
        self.M4_STDC += os.path.basename(i) + ' \n'
      self.M4_STDC += '"'
    # hack for symlink
    # setup process generates the wrapper at install time
    # but we need a dummy executable for symlink in loki_setup
    self.system('echo -n "#!/bin/sh\necho If you read this then there was a bug during setup. Report the bug and try running %s directly from it\'s installation directory.\n" > %s/radiant' % (self.EDITOR_BIN, self.SETUP_BIN_DIR));
    self.system('echo -n "#!/bin/sh\necho If you read this then there was a bug during setup. Report the bug and try running %s directly from it\'s installation directory.\n" > %s/q3map2' % (self.EDITOR_BIN, self.SETUP_BIN_DIR));
    ## this goes to the core install directory
    DEST = self.SETUP_DIR + '/core'
    self.system('mkdir -p ' + DEST + '/modules/bitmaps')
    # general content stuff
    self.system('cp -R plugins/model/bitmaps/* ' + DEST + '/modules/bitmaps')
    self.system('cp setup/data/tools/credits.html ' + DEST)
    self.system('cp setup/data/tools/links.htm ' + DEST)
    self.system('cp setup/data/tools/q3data.qdt ' + DEST)
    self.system('cp setup/data/tools/global.xlink ' + DEST)
    self.system('cp -R radiant/bitmaps ' + DEST)
    self.system('cp setup/changelog.txt ' + DEST)
    self.system('cp setup/openurl.sh ' + DEST)
    self.system('cp tools/quake3/q3map2/changelog.q3map2.txt ' + DEST)
    # documentation
    self.system('cp -R docs/manual/Q3Rad_Manual ' + DEST)
    self.system('cp -R docs/manual/quake3/Compile_Manual ' + DEST)
    self.system('cp -R docs/manual/quake3/Model_Manual ' + DEST)
    self.system('cp -R docs/manual/quake3/Terrain_Manual ' + DEST)
    # copy plugins media
    #self.system('mkdir -p ' + DEST + '/plugins/bitmaps')
    #self.system('cp -R contrib/bobtoolz/bitmaps/* ' + DEST + '/plugins/bitmaps')
    #self.system('cp -R contrib/bobtoolz/bt ' + DEST + '/plugins')
    #self.system('cp -R contrib/camera/bitmaps/* ' + DEST + '/plugins/bitmaps' )
    #self.system('cp -R contrib/bkgrnd2d/bitmaps/* ' + DEST + '/plugins/bitmaps' )
    # games files
    self.system('mkdir -p ' + self.SETUP_DIR + '/games')
  
  def copy_doom3(self):
    # goes in core
    DEST = self.SETUP_DIR + '/core/doom3.game'
    self.system('mkdir -p ' + DEST)
    self.system('cp -R games/Doom3Pack/tools/doom3.game/* ' + DEST)
  
    # game file
    self.system('cp -R games/Doom3Pack/tools/games/doom3.game ' + self.SETUP_DIR + '/games')

  def build_setup(self):
    self.system( 'cp -R ' + self.SETUP_IMAGE_OS + '/* ' + self.SETUP_DIR )
    self.system( 'cp -fR ' + self.SETUP_IMAGE + '/* ' + self.SETUP_DIR )
    self.system('cp setup/license.txt ' + self.SETUP_DIR)
    self.system('cp setup/linux/README ' + self.SETUP_DIR)
    OS_DEFS=''
    if (self.g_darwin):
      OS_DEFS='--define=M4_OSX'
    M4_LINE = OS_DEFS + ' --define=M4_VER_MAJOR=' + self.major + ' --define=M4_VER_MINOR=' + self.minor + ' --define=M4_VER=' + self.line 
    M4_LINE += ' --define=M4_GAME_ET=%d' % self.DO_GAME_ET
    M4_LINE += ' --define=M4_GAME_DOOM3=%d' % self.DO_GAME_DOOM3
    M4_LINE += ' --define=M4_GAME_Q2=%d' % self.DO_GAME_Q2
    if ( self.M4_STDC != '' ):
      M4_LINE += ' --define=M4_STDC=' + self.M4_STDC 
    # setup.xml
    self.system('m4 ' + M4_LINE + ' ' + self.SETUP_DIR + '/setup.data/setup.xml.in > ' + self.SETUP_DIR + '/setup.data/setup.xml')
    # postinstall.sh
    self.system('m4 ' + M4_LINE + ' ' + self.SETUP_DIR + '/setup.data/postinstall.sh.in > ' + self.SETUP_DIR + '/setup.data/postinstall.sh')
    # config.sh
    self.system('m4 ' + M4_LINE + ' ' + self.SETUP_DIR + '/setup.data/config.sh.in > ' + self.SETUP_DIR + '/setup.data/config.sh')
    # setup.sh
    self.system('m4 ' + M4_LINE + ' ' + self.SETUP_DIR + '/setup.sh.in > ' + self.SETUP_DIR + '/setup.sh')
    self.system('chmod +x ' +self.SETUP_DIR + '/setup.sh')
    self.system('find ' + self.SETUP_DIR + ' -name .svn | while read i ; do rm -r "$i" ; done')
    # pack it up
    self.system('setup/linux/makeself/makeself.sh ' + self.SETUP_DIR + ' ' + self.SETUP_TARGET + ' "GtkRadiant ' + self.line + ' setup" ./setup.sh')
    if (self.g_darwin):
      def build_fink_deb(self):
        print "Building installer .deb\n"
        self.F_REV = '1'
        self.FINKINFO_DIR = '/sw/fink/10.2/unstable/main/finkinfo/games'
        self.TARBALL_DIR='radiant-' + self.F_REV + '.' + self.major 
        self.TARBALL_NAME='radiant-' + self.F_REV + '.' + self.major + '.tar.gz'
        self.TARBALL_DEST='/sw/src'

        # prepare package description
        self.system('mkdir -p ' + self.FINKINFO_DIR)
        self.system('m4 ' + M4_LINE + ' --define=M4_SETUP_TARGET=' + self.SETUP_TARGET + '  --define=M4_F_REV=' + self.F_REV + ' ' + 'setup/osx/radiant.info.m4 > ' + self.FINKINFO_DIR + '/radiant-' + self.TARBALL_DIR + '.info')

        # build the tarball
        self.system('if [ -r /tmp/' + self.TARBALL_DIR + ' ] ; then rm -r ' '/tmp/' + self.TARBALL_DIR + ' ; fi')
        self.system('mkdir -p ' '/tmp/' + self.TARBALL_DIR)
        self.system('cp ' + self.SETUP_TARGET + ' ' + '/tmp/' + self.TARBALL_DIR)
        self.system('cd /tmp ; tar -cvzf ' + self.TARBALL_NAME + ' ' + self.TARBALL_DIR + ' ; cp ' + self.TARBALL_NAME + ' ' + self.TARBALL_DEST + '/')
        self.system('/sw/bin/fink rebuild radiant')

        build_fink_deb(self)
      
  def spawn_setup(self, env, target, source):
    if ( OS == 'Darwin' ):
      self.g_darwin = 1
    else:
      self.g_darwin = 0
    (self.line, self.major, self.minor) = get_version()
    print 'Setup: GtkRadiant %s' % self.line  
    if ( self.g_darwin ):
      self.SETUP_IMAGE_OS = '../loki_setup/image'
    else:
      self.SETUP_IMAGE_OS = 'setup/linux/setup_image.Linux'
    self.SETUP_IMAGE = 'setup/linux/setup_image'
    self.SETUP_DIR = '/tmp/radiant-setup.%d' % os.getpid()
    self.EDITOR_BIN='radiant.' + g_cpu
    if ( self.g_darwin ):
      self.SETUP_BIN_DIR = self.SETUP_DIR + '/bin/Darwin/ppc'
      self.SETUP_TARGET = 'osx-radiant-%s.run' % self.line
    else:
      self.SETUP_BIN_DIR = self.SETUP_DIR + '/bin/Linux/x86'
      self.SETUP_TARGET = 'linux-radiant-%s.run' % self.line
    # TODO: eval a conf file instead
    self.DO_CORE=1
    self.DO_GAME_Q3=1
    self.DO_GAME_WOLF=1
    self.DO_GAME_ET=1
    self.DO_GAME_DOOM3=1
    self.DO_GAME_Q2=1
    self.DO_GAME_HER2=1
    if ( self.g_darwin ):
      self.DO_GAME_Q2=0
      self.DO_GAME_ET=0
      self.DO_GAME_HER2=0
    # verbose a bit
    print 'version: %s major: %s minor: %s\neditor core: %d\nq3: %d\nwolf: %d\net: %d\ndoom3: %d\nq2: %d\nher2: %d' % (self.line, self.major, self.minor, self.DO_CORE, self.DO_GAME_Q3, self.DO_GAME_WOLF, self.DO_GAME_ET, self.DO_GAME_DOOM3, self.DO_GAME_Q2, self.DO_GAME_HER2)
    if (self.DO_CORE):
      self.copy_core()
    if (self.DO_GAME_Q3):
      self.copy_q3()
    if (self.DO_GAME_WOLF):
      self.copy_wolf()
    if (self.DO_GAME_ET):
      self.copy_et()
    if (self.DO_GAME_DOOM3):
      self.copy_doom3()
    if ( OS != 'Darwin' ):
      if (self.DO_GAME_Q2):
        self.copy_q2()
      if (self.DO_GAME_HER2):
        self.copy_her2()
    self.build_setup()
    return 0
    
def spawn_setup(env, target, source):
  setup = setup_builder()
  setup.spawn_setup(env, target, source)

# NOTE: could modify g_env to add the deps auto when calling SharedLibrarySafe ..
if (SETUP == '1'):
  g_env.Command('foo', INSTALL + '/radiant.' + g_cpu, [ spawn_setup ] )
  depends_list = [ 
    INSTALL + '/modules/archivepak.so',
    INSTALL + '/modules/archivewad.so',
    INSTALL + '/modules/archivezip.so',
    INSTALL + '/modules/entity.so',
#    INSTALL + '/modules/fgd.so',
    INSTALL + '/modules/imagehl.so',
    INSTALL + '/modules/imageq2.so',
    INSTALL + '/modules/image.so',
    INSTALL + '/modules/imagepng.so',
    INSTALL + '/modules/mapq3.so',
    INSTALL + '/modules/mapxml.so',
    INSTALL + '/modules/model.so',
    INSTALL + '/modules/md3model.so',
    INSTALL + '/modules/shaders.so',
    INSTALL + '/modules/vfspk3.so',
#    INSTALL + '/modules/vfswad.so',
#    INSTALL + '/plugins/bobtoolz.so',
#    INSTALL + '/plugins/camera.so',
#    INSTALL + '/plugins/prtview.so',
#    INSTALL + '/plugins/gensurf.so',
#    INSTALL + '/plugins/bkgrnd2d.so',
    INSTALL + '/q3map2.' + g_cpu,
    INSTALL + '/radiant.' + g_cpu,
    INSTALL + '/q3data.' + g_cpu ]
  if ( OS != 'Darwin' ):
    depends_list += [
      INSTALL + '/q2/q2map',
      INSTALL + '/q2/qdata3',
      INSTALL + '/heretic2/qdata3',
      INSTALL + '/heretic2/q2map' ]
  g_env.Depends( 'foo', depends_list )

# end setup ---------------------------------------------------------------------------------------
