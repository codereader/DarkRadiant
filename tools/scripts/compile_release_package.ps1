param (
    [Parameter(Mandatory, ValueFromPipeline)]
    [string]$Platform, 

    [Parameter(Mandatory=$false)]
    [string]$OutputFolder,

    [Parameter(Mandatory=$false)]
    [switch]$SkipBuild,
    
    [Parameter(Mandatory=$false)]
    [switch]$GenerateSetupPackage = $true,

    [Parameter(Mandatory=$false)]
    [switch]$GeneratePortablePackage = $true
)

if ($SkipBuild)
{
    Write-Host "SkipBuild specified: Will skip the build process."
}

if ($null -eq $GenerateSetupPackage)
{
    $GenerateSetupPackage = $true
}

if ($null -eq $GeneratePortablePackage)
{
    $GeneratePortablePackage = $true
}

Write-Host ("Generate InnoSetup Package: {0}" -f $(if ($GenerateSetupPackage -eq $true) { "yes" } else { "no" }))
Write-Host ("Generate Portable Package: {0}" -f $(if ($GeneratePortablePackage -eq $true) { "yes" } else { "no" }))

$dummy = [Reflection.Assembly]::LoadWithPartialName("System.IO");

if ([string]::IsNullOrEmpty($OutputFolder) -eq $false)
{
    if (-not [System.IO.Directory]::Exists($OutputFolder))
    {
        Write-Host "The given output path $OutputFolder doesn't exist, creating..."
        $dummy = New-Item -Path $OutputFolder -ItemType Directory -ErrorAction Stop
    }

    Write-Host ("Will copy the resulting packages to the folder {0}" -f $OutputFolder)
}

# Check tool reachability
if ($GenerateSetupPackage -and $null -eq (Get-Command "compil32" -ErrorAction SilentlyContinue))
{
    Write-Host -ForegroundColor Red "For this script to run, please make sure that InnoSetup's compil32 is found via the PATH environment variable"
    return
}

if (-not $SkipBuild -and ($null -eq (Get-Command "msbuild" -ErrorAction SilentlyContinue)))
{
    Write-Host -ForegroundColor Red "For this script to run, please make sure that you've opened the corresponding studio developer command prompt."
    return
}

# ----------------------------------------------

$target = $Platform

Write-Host ("Compiling for target: {0}" -f $target)

$portableFilenameTemplate = "darkradiant-{0}-$target.portable.7z"
$pdbFilenameTemplate = "darkradiant-{0}-$target.pdb.7z"
$innoSetupFilenameTemplate = "darkradiant-{0}-$target.exe"

$versionRegex = '#define RADIANT_VERSION "(.+)"'
$versionIncludeFile = "..\..\include\version.h"

# Extract the version string from the version.h file
$content = Get-Content $versionIncludeFile
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

# Check the location of the VC redist folder
$vcRedistFolder = $env:VCToolsRedistDir

if ($null -eq $vcRedistFolder)
{
    Write-Host "Trying to guess the VCRedistTools folder location..."

    $candidate = Get-ChildItem -Path "C:\Program Files\Microsoft Visual Studio\" -Recurse -Filter "Redist" | ? { (Get-ChildItem -Path $_.FullName -Filter "MSVC") }

    if ($null -ne $candidate)
    {
        $candidate = Join-Path $candidate.FullName "MSVC"
        
        $candidate = Get-ChildItem -Recurse -Path $candidate -Filter $target -Depth 1 | ? { (Get-ChildItem -Path $_.FullName -Filter "Microsoft.VC143.CRT") } | Select -First 1
        
        $vcRedistFolder = $candidate.Parent.FullName
        Write-Host "Guessed this path: $vcRedistFolder"
    }
}

if ($target -eq "x86")
{
    $platform = "Win32"
    $issFile = "..\innosetup\darkradiant.iss"
    $portablePath = "DarkRadiant_install"
	$redistSource = Join-Path $vcRedistFolder "x86\Microsoft.VC143.CRT"
} 
else
{
    $platform = "x64"
    $issFile = "..\innosetup\darkradiant.x64.iss"
    $portablePath = "DarkRadiant_install.x64"
	$redistSource = Join-Path $vcRedistFolder "x64\Microsoft.VC143.CRT"
}

if (-not $SkipBuild)
{
	Start-Process "msbuild" -ArgumentList ("..\..\DarkRadiant.sln", "/p:configuration=release", "/t:rebuild", "/p:platform=$platform", "/maxcpucount:4", "/nodeReuse:false", "/p:UseSharedConfiguration=false") -NoNewWindow -Wait
}

# Copy files to portable files folder

$pathToCheck = ".\$portablePath"
$portableFilesFolder = Get-Item -Path $pathToCheck -ErrorAction SilentlyContinue

if ($null -eq $portableFilesFolder)
{
	$portableFilesFolder = New-Item -Path $pathToCheck -ItemType Directory -ErrorAction Stop
}

Write-Host ("Clearing output folder {0}" -f $portableFilesFolder)
Get-ChildItem -Path $portableFilesFolder | Remove-Item -Recurse -Force

Write-Host ("Copying files...")

$installFolder = Get-Item "..\..\install"
$excludes = @('*.exp', '*.lib', '*.iobj', '*.ipdb', '*.suo', '*.pgd', '*.fbp', 'darkradiant.desktop.in', 'Tests.*')

Get-ChildItem $installFolder -Recurse -Exclude $excludes | Copy-Item -Destination { Join-Path $portableFilesFolder $_.FullName.Substring($installFolder.FullName.Length) }

# Copy the VC++ redist files
$vcFolder = Get-Item $redistSource -Erroraction SilentlyContinue

if ($null -ne $vcFolder)
{
	Get-ChildItem "msvcp140.dll" -Path $vcFolder | Copy-Item -Destination $portableFilesFolder
    Get-ChildItem "vcruntime140.dll" -Path $vcFolder | Copy-Item -Destination $portableFilesFolder
    Get-ChildItem "vcruntime140_1.dll" -Path $vcFolder | Copy-Item -Destination $portableFilesFolder
}
else
{
	Write-Host -ForegroundColor Yellow "Warning: cannot find the VC++ redist folder, won't copy runtime DLLs."
}

$portableFilename = Join-Path "..\innosetup\" ($portableFilenameTemplate -f $foundVersionString)
$pdbFilename = Join-Path "..\innosetup\" ($pdbFilenameTemplate -f $foundVersionString)
$innoSetupFilename = Join-Path "..\innosetup\" ($innoSetupFilenameTemplate -f $foundVersionString)

if ($GenerateSetupPackage)
{
    # Write the version to the innosetup source file
    Write-Host ("Writing version {0} to InnoSetup file" -f $foundVersionString)
    $issContent = Get-Content $issFile
    $issContent = $issContent -replace '#define DarkRadiantVersion "(.+)"', ('#define DarkRadiantVersion "{0}"' -f $foundVersionString)
    Write-Host ("Writing redist folder {0} to InnoSetup file" -f $vcRedistFolder)
    $issContent = $issContent -replace '#define VCRedistDir "(.+)"', ('#define VCRedistDir "{0}"' -f $vcRedistFolder)
    Set-Content -Path $issFile -Value $issContent

    # Compile the installer package
    Start-Process "compil32" -ArgumentList ("/cc", $issFile)
}

if ($GeneratePortablePackage)
{
    $pathTo7z = "..\7z\7za.exe"

    # Compress to portable 7z package 
    if ($null -ne (Get-ChildItem -Path $portableFilename -ErrorAction SilentlyContinue))
    {
        Remove-Item -Path $portableFilename
    }

    Start-Process -FilePath $pathTo7z -ArgumentList ("a", "-r", "-x!*.pdb", "-mx9", "-mmt2", $portableFilename, "$portableFilesFolder\*.*")

    # Compress Program Database Files
    if ($null -ne (Get-ChildItem -Path $pdbFilename -ErrorAction SilentlyContinue))
    {
        Remove-Item -Path $pdbFilename
    }

    Start-Process -FilePath $pathTo7z -ArgumentList ("a", "-r", "-mx9", "-mmt2", $pdbFilename, "$portableFilesFolder\*.pdb") -Wait
}

# Copy to target folder when everything is done
if ([string]::IsNullOrEmpty($OutputFolder) -eq $false)
{
    Write-Host "Copying files to output folder $OutputFolder..."

    if ($GenerateSetupPackage)
    {
        Copy-Item -Path $innoSetupFilename -Destination (Join-Path $OutputFolder ([System.IO.Path]::GetFileName($innoSetupFilename))) -Force
    }

    if ($GeneratePortablePackage)
    {
        Copy-Item -Path $portableFilename -Destination (Join-Path $OutputFolder ([System.IO.Path]::GetFileName($portableFilename))) -Force
        Copy-Item -Path $pdbFilename -Destination (Join-Path $OutputFolder ([System.IO.Path]::GetFileName($pdbFilename))) -Force
    }
}
