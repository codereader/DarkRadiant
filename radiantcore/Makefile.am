libradiantcoredir = $(pkglibdir)
libradiantcore_LTLIBRARIES = libradiantcore.la

libradiantcore_la_CPPFLAGS = -DPKGLIBDIR='"$(pkglibdir)"' \
              -DPKGDATADIR='"$(pkgdatadir)"' \
              -DLOCALEDIR='"$(localedir)"' \
              -DHTMLDIR='"$(htmldir)"' \
              -I$(top_srcdir)/include \
              -I$(top_srcdir)/libs \
              $(LIBSIGC_CFLAGS) \
              $(XML_CFLAGS) \
              $(FTGL_CFLAGS)

libradiantcore_la_LIBADD = $(top_builddir)/libs/scene/libscenegraph.la \
             $(top_builddir)/libs/wxutil/libwxutil.la \
             $(top_builddir)/libs/xmlutil/libxmlutil.la \
             $(top_builddir)/libs/math/libmath.la

libradiantcore_la_LDFLAGS = -module -avoid-version \
                    $(XML_LIBS) \
                    $(GLEW_LIBS) \
                    $(GL_LIBS) \
                    $(GLU_LIBS) \
                    $(JPEG_LIBS) \
                    $(PNG_LIBS) \
                    $(FTGL_LIBS) \
                    $(LIBSIGC_LIBS) \
                    $(FILESYSTEM_LIBS) \
                    $(DL_LIBS) \
                    $(INTL_LIBS) \
                    $(WX_LIBS) \
                    $(Z_LIBS)

libradiantcore_la_SOURCES = Radiant.cpp \
                commandsystem/CommandSystem.cpp \
                eclass/Doom3EntityClass.cpp \
                eclass/EClassManager.cpp \
                entity/ShaderParms.cpp \
                entity/EntitySettings.cpp \
                entity/KeyValueObserver.cpp \
                entity/EntityNode.cpp \
                entity/RotationMatrix.cpp \
                entity/target/TargetManager.cpp \
                entity/target/TargetLineNode.cpp \
                entity/target/TargetKeyCollection.cpp \
                entity/target/TargetKey.cpp \
                entity/target/TargetableNode.cpp \
                entity/light/Renderables.cpp \
                entity/light/Light.cpp \
                entity/light/LightNode.cpp \
                entity/eclassmodel/EclassModelNode.cpp \
                entity/RotationKey.cpp \
                entity/NameKeyObserver.cpp \
                entity/doom3group/Doom3GroupNode.cpp \
                entity/doom3group/Doom3Group.cpp \
                entity/NamespaceManager.cpp \
                entity/generic/GenericEntity.cpp \
                entity/generic/GenericEntityNode.cpp \
                entity/speaker/SpeakerNode.cpp \
                entity/speaker/SpeakerRenderables.cpp \
                entity/Doom3Entity.cpp \
                entity/ModelKey.cpp \
                entity/KeyValue.cpp \
                entity/curve/CurveCatmullRom.cpp \
                entity/curve/Curve.cpp \
                entity/curve/CurveEditInstance.cpp \
                entity/curve/CurveNURBS.cpp \
                entity/AngleKey.cpp \
                entity/EntityModule.cpp \
                filetypes/FileTypeRegistry.cpp \
                filters/BasicFilterSystem.cpp \
                filters/XMLFilter.cpp \
                filters/XmlFilterEventAdapter.cpp \
                grid/GridManager.cpp \
                imagefile/dds.cpp \
                imagefile/ddslib.cpp \
                imagefile/DDSImage.cpp \
                imagefile/ImageLoader.cpp \
                imagefile/ImageLoaderWx.cpp \
                imagefile/TGALoader.cpp \
                log/COutRedirector.cpp \
                log/LogFile.cpp \
                log/LogStream.cpp \
                log/LogStreamBuf.cpp \
                log/LogWriter.cpp \
                log/StringLogDevice.cpp \
                map/algorithm/Models.cpp \
                map/format/MapFormatManager.cpp \
                map/namespace/ComplexName.cpp \
                map/namespace/Namespace.cpp \
                map/namespace/NamespaceFactory.cpp \
                model/ModelCache.cpp \
                model/ModelFormatManager.cpp \
                model/NullModel.cpp \
                model/NullModelNode.cpp \
                modulesystem/ModuleLoader.cpp \
                modulesystem/ModuleRegistry.cpp \
                rendersystem/backend/glprogram/ARBBumpProgram.cpp \
                rendersystem/backend/glprogram/ARBDepthFillProgram.cpp \
                rendersystem/backend/glprogram/GenericVFPProgram.cpp \
                rendersystem/backend/glprogram/GLSLBumpProgram.cpp \
                rendersystem/backend/glprogram/GLSLDepthFillProgram.cpp \
                rendersystem/backend/OpenGLShader.cpp \
                rendersystem/backend/GLProgramFactory.cpp \
                rendersystem/backend/OpenGLShaderPass.cpp \
                rendersystem/LinearLightList.cpp \
                rendersystem/OpenGLRenderSystem.cpp \
                rendersystem/RenderSystemFactory.cpp \
                rendersystem/debug/SpacePartitionRenderer.cpp \
                scenegraph/SceneGraph.cpp \
                scenegraph/Octree.cpp \
                scenegraph/SceneGraphFactory.cpp \
                settings/Game.cpp \
                settings/GameManager.cpp \
                settings/LanguageManager.cpp \
                settings/PreferencePage.cpp \
                settings/PreferenceSystem.cpp \
                shaders/CameraCubeMapDecl.cpp \
                shaders/textures/GLTextureManager.cpp \
                shaders/textures/TextureManipulator.cpp \
                shaders/CShader.cpp \
                shaders/Doom3ShaderLayer.cpp \
                shaders/Doom3ShaderSystem.cpp \
                shaders/MapExpression.cpp \
                shaders/ShaderExpression.cpp \
                shaders/ShaderLibrary.cpp \
                shaders/ShaderTemplate.cpp \
                shaders/TableDefinition.cpp \
                skins/Doom3SkinCache.cpp \
                undo/UndoSystem.cpp \
                vfs/DeflatedInputStream.cpp \
                vfs/DirectoryArchive.cpp \
                vfs/Doom3FileSystem.cpp \
                vfs/Doom3FileSystemModule.cpp \
                vfs/ZipArchive.cpp \
                xmlregistry/RegistryTree.cpp \
                xmlregistry/XMLRegistry.cpp

