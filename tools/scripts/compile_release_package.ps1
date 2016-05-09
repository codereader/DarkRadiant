# Check input arguments
if ($args.Count -eq 0 -or ($args[0] -ne "x64" -and $args[0] -ne "x86"))
{
    Write-Host "Usage: compile_release_package.ps1 <x64|x86>"
    Write-Host ""
    Write-Host "e.g. to compile a 64 bit build: .\compile_release_package.ps1 x64"
    Write-Host ""
    return
}

# Check tool reachability
if ((Get-Command "compil32" -ErrorAction SilentlyContinue) -eq $null)
{
    Write-Host -ForegroundColor Red "For this script to run, please make sure that InnoSetup's compil32 is found via the PATH environment variable"
    return
}

if ((Get-Command "msbuild" -ErrorAction SilentlyContinue) -eq $null)
{
    Write-Host -ForegroundColor Red "For this script to run, please make sure that you've opened the corresponding studio developer command prompt."
    return
}

# ----------------------------------------------

$target = $args[0]

Write-Host ("Compiling for target: {0}" -f $target)

$versionRegex = "AppVerName=DarkRadiant (.*) $target"
$portableFilenameTemplate = "darkradiant-{0}-$target.7z"
$pdbFilenameTemplate = "darkradiant-{0}-$target.pdb.7z"

if ($target -eq "x86")
{
    $platform = "Win32"
    $copyFilesCmd = ".\copy_install_files.cmd"
    $issFile = "..\innosetup\darkradiant.iss"
    $portableFilesFolder = "DarkRadiant_install"
} 
else
{
    $platform = "x64"
    $copyFilesCmd = ".\copy_install_files.x64.cmd"
    $issFile = "..\innosetup\darkradiant.x64.iss"
    $portableFilesFolder = "DarkRadiant_install.x64"
}

Start-Process "msbuild" -ArgumentList ("..\msvc2013\DarkRadiant.sln", "/p:configuration=release", "/t:rebuild", "/p:platform=$platform", "/maxcpucount:4") -NoNewWindow -Wait

Start-Process $copyFilesCmd -NoNewWindow -Wait

# Get the version from the innosetup file
$content = Get-Content $issFile
$defaultVersionString = "X.Y.ZpreV"
$foundVersionString = $defaultVersionString

foreach ($line in $content)
{
    if ($line -match $versionRegex)
    {
        $foundVersionString = $matches[1] 
        Write-Host ("Version is {0}" -f $matches[1])
        break
    }
}

$portableFilename = "..\innosetup\" + ($portableFilenameTemplate -f $foundVersionString)
$pdbFilename = "..\innosetup\" + ($pdbFilenameTemplate -f $foundVersionString)

# Compile the installer package
Start-Process "compil32" -ArgumentList ("/cc", $issFile)

# Compress to portable 7z package 
if ((Get-ChildItem -Path $portableFilename -ErrorAction SilentlyContinue) -ne $null)
{
    Remove-Item -Path $portableFilename
}
Start-Process -FilePath "C:\Program Files\7-Zip\7z.exe" -ArgumentList ("a", "-r", "-x!*.pdb", "-mx9", "-mmt2", $portableFilename, "..\..\..\$portableFilesFolder\*.*")

# Compress Program Database Files
if ((Get-ChildItem -Path $pdbFilename -ErrorAction SilentlyContinue) -ne $null)
{
    Remove-Item -Path $pdbFilename
}
Start-Process -FilePath "C:\Program Files\7-Zip\7z.exe" -ArgumentList ("a", "-r", "-mx9", "-mmt2", $pdbFilename, "..\..\..\$portableFilesFolder\*.pdb") -Wait
