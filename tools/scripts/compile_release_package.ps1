# Check input arguments
if ($args.Count -eq 0 -or ($args[0] -ne "x64" -and $args[0] -ne "x86"))
{
    Write-Host "Usage: compile_release_package.ps1 <x64|x86>"
    Write-Host ""
    Write-Host "e.g. to compile a 64 bit build: .\compile_release_package.ps1 x64"
    Write-Host ""
    return
}

$skipbuild = $false

foreach ($arg in $args)
{
	if ($arg -eq "skipbuild")
	{
		Write-Host "skipbuild: Will skip the build process."
		$skipbuild = $true
	}
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
    $issFile = "..\innosetup\darkradiant.iss"
    $portablePath = "DarkRadiant_install"
	$redistSource = "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Redist\MSVC\14.12.25810\x86\Microsoft.VC141.CRT"
} 
else
{
    $platform = "x64"
    $issFile = "..\innosetup\darkradiant.x64.iss"
    $portablePath = "DarkRadiant_install.x64"
	$redistSource = "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Redist\MSVC\14.12.25810\x64\Microsoft.VC141.CRT"
}

if (-not $skipbuild)
{
	Start-Process "msbuild" -ArgumentList ("..\msvc\DarkRadiant.sln", "/p:configuration=release", "/t:rebuild", "/p:platform=$platform", "/maxcpucount:4") -NoNewWindow -Wait
}

# Copy files to portable files folder

$pathToCheck = "..\..\..\$portablePath"
$portableFilesFolder = Get-Item -Path $pathToCheck

if ($portableFilesFolder -eq $null)
{
	$portableFilesFolder = New-Item -Path $pathToCheck -ItemType Directory -ErrorAction Stop
}

Write-Host ("Clearing output folder {0}" -f $portableFilesFolder)
Get-ChildItem -Path $portableFilesFolder | Remove-Item -Recurse -Force

Write-Host ("Copying files...")

$installFolder = Get-Item "..\..\install"
$excludes = @('*.exp', '*.lib', '*.iobj', '*.ipdb', '*.suo', '*.pgd', '*.fbp', 'darkradiant.desktop.in')

Get-ChildItem $installFolder -Recurse -Exclude $excludes | Copy-Item -Destination { Join-Path $portableFilesFolder $_.FullName.Substring($installFolder.FullName.Length) }

# Copy the VC++ redist files
$vcFolder = Get-Item $redistSource -Erroraction SilentlyContinue

if ($vcFolder -ne $null)
{
	Get-ChildItem "msvcp140.dll" -Path $vcFolder | Copy-Item -Destination $portableFilesFolder
	Get-ChildItem "vcruntime140.dll" -Path $vcFolder | Copy-Item -Destination $portableFilesFolder
}
else
{
	Write-Host -ForegroundColor Yellow "Warning: cannot find the VC++ redist folder, won't copy runtime DLLs."
}

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
Start-Process -FilePath "C:\Program Files\7-Zip\7z.exe" -ArgumentList ("a", "-r", "-x!*.pdb", "-mx9", "-mmt2", $portableFilename, "..\..\..\$portablePath\*.*")

# Compress Program Database Files
if ((Get-ChildItem -Path $pdbFilename -ErrorAction SilentlyContinue) -ne $null)
{
    Remove-Item -Path $pdbFilename
}
Start-Process -FilePath "C:\Program Files\7-Zip\7z.exe" -ArgumentList ("a", "-r", "-mx9", "-mmt2", $pdbFilename, "..\..\..\$portablePath\*.pdb") -Wait
