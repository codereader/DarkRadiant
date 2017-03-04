echo Building Package in $TARGET_BUILD_DIR
cd $TARGET_BUILD_DIR
cd DarkRadiant.app
cd Contents
cd MacOS

# Copy wxwidgets dependencies
WXLIBPOSTFIX=*wx*3.0.0.dylib
APP=DarkRadiantMain
     
WXLIBDIR=/opt/local/Library/Frameworks/wxWidgets.framework/Versions/wxWidgets/3.0/lib/
BINDIR=./
LIBDEFDIR=/opt/local/Library/Frameworks/wxWidgets.framework/Versions/wxWidgets/3.0/lib/
     
echo "Copying dynamic libraries to " $BINDIR " ..."
cp $WXLIBDIR/*wx*3.0.0.dylib $BINDIR
     
echo "Changing directory to " $BINDIR " ..."
export TMP=$PWD
cd $BINDIR

function fetchLibraries()
{
    # The lib we're inspecting
    local file=$1
    local targetDir=$2

    # Fetch all the other libs referenced in /opt/local/lib
    # e.g. /opt/local/lib/libftgl.2.dylib
    local fileRegex=(\(/opt/local/lib/\(.*.dylib\)\))

    echo "Inspecting binary " $file

    for dependency in `otool -L $file`
    do
        if [[ $dependency =~ $fileRegex ]];
        then
            #echo ${BASH_REMATCH[0]} ${BASH_REMATCH[1]} ${BASH_REMATCH[2]}

            local dependencyPath=${BASH_REMATCH[1]}
            local filename=${BASH_REMATCH[2]}
            local fetchedFile=$targetDir$filename

            echo "==> " $file " depends on " $filename

            if [ ! -f $fetchedFile ]
            then
                echo Fetching ${BASH_REMATCH[0]} to $targetDir...
                cp $dependencyPath $targetDir

                echo "Fixing ID of file " $fetchedFile
                echo install_name_tool -id @executable_path/$filename $fetchedFile
                install_name_tool -id @executable_path/$filename $fetchedFile
            else
                echo $fetchedFile already exists
            fi

            echo "Fixing link in referencing file " $file
            echo install_name_tool -change ${BASH_REMATCH[1]} @executable_path/$filename $file
            install_name_tool -change ${BASH_REMATCH[1]} @executable_path/$filename $file

            echo "Entering recursion for lib " $fetchedFile
            fetchLibraries $fetchedFile $targetDir
        fi
    done
}

# patch all wx dynlibs and the main executable
for file in `ls $WXLIBPOSTFIX`
do
    # patch all library internal cross references
    echo "Patching " $file "..."
    for fileother in `ls $WXLIBPOSTFIX `
    do
        # library
        echo "  Patching " $fileother " with " $file "..."
        install_name_tool -change $LIBDEFDIR/$file @executable_path/$file  $fileother
    done
    
    # patch current library itself
    install_name_tool -id @executable_path/$file $file
    
    # patch executable
    install_name_tool -change $LIBDEFDIR$file @executable_path/$file $APP 

    # Resolve the 3.0.0.2.0 version issue
    fileRegex=(\(libwx_[A-Za-z_]+-\)\(\([0-9]\.[0-9]\.[0-9]\)\.[0-9]\.[0-9]\)\.dylib)
    for dependency in `otool -L $file`
    do
        if [[ $dependency =~ $fileRegex ]]; 
        then 
            echo Replacing ${BASH_REMATCH[0]} with ${BASH_REMATCH[1]}3.0.0.dylib...
            install_name_tool -change $LIBDEFDIR${BASH_REMATCH[0]} @executable_path/${BASH_REMATCH[1]}3.0.0.dylib $file
        fi
    done

    # Fetch referenced libraries
    fetchLibraries $file $BINDIR
done

echo Patching Main Executable $APP

pwd
otool -L $APP

# Replace the 3.0 version references with 3.0.0 ones
fileRegex=(\(libwx_[A-Za-z_]+-\)\([0-9]\.[0-9]\)\.dylib)
for dependency in `otool -L $APP`
do
    if [[ $dependency =~ $fileRegex ]]; 
    then 
        echo Replacing ${BASH_REMATCH[0]} with ${BASH_REMATCH[1]}3.0.0.dylib...
        install_name_tool -change $LIBDEFDIR${BASH_REMATCH[0]} @executable_path/${BASH_REMATCH[1]}3.0.0.dylib $APP
    fi       
done

# Fetch dependencies from /opt/local
fetchLibraries $APP $BINDIR

# Patch all plugins
for plugin in `ls *.so`
do
    echo Patching $plugin... 
    
    # wx library dependency
    fileRegex=(\(libwx_[A-Za-z_]+-\)\([0-9]\.[0-9]\)\.dylib)
    for dependency in `otool -L $plugin`
    do
        if [[ $dependency =~ $fileRegex ]]; 
        then 
            echo Replacing ${BASH_REMATCH[0]} with ${BASH_REMATCH[1]}3.0.0.dylib...
            install_name_tool -change $LIBDEFDIR${BASH_REMATCH[0]} @executable_path/${BASH_REMATCH[1]}3.0.0.dylib $plugin
        fi       
    done

    # Fetch all the other libs referenced in /opt/local/lib
    fetchLibraries $plugin $BINDIR
done

cd $TMP
