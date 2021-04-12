try
{
    Write-Host "Checking latest windeps package..."

    $headers = @{ "Accept" = "application/vnd.github.v3+json" }
    $uri = "https://api.github.com/repos/codereader/DarkRadiant_windeps/releases"
    $releases = Invoke-RestMethod -Headers $headers -Uri $uri

    $latest = $releases | sort created_at -Descending | select -First 1

    $windeps_package = $latest.assets | ? { $_.name -eq "windeps.7z" }

    if ($null -eq $windeps_package)
    {
        throw "Could not find the windows dependencies package URL"
    }

    Write-Host ("Downloading dependencies package ({0:0.0} MB)" -f ($windeps_package.size / 1024 / 1024))

    $package_path = Join-Path "..\..\" $windeps_package.name

    if ((Get-ChildItem $package_path -ErrorAction SilentlyContinue).Length -eq $windeps_package.size)
    {
        Write-Host "Package has already been downloaded to $package_path"
    }

    Invoke-WebRequest $windeps_package.browser_download_url -OutFile $package_path

    $target_folder = Join-Path (Get-Location) "..\..\"
    Write-Host ("Extracting dependencies package...")

    & ..\7z\7za.exe x $package_path -o"$target_folder"

    if ((Get-ChildItem "..\..\w32deps" -ErrorAction SilentlyContinue) -eq $null)
    {
        throw "w32deps folder still not present after download... cannot continue"
    }
}
catch
{
    Write-Host -Foreground Red ("Error: {0}" -f $_)
    exit 1
}
