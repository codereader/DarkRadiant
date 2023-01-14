try
{
    Write-Host "Checking latest windeps package..."

    $headers = @{ "Accept" = "application/vnd.github.v3+json" }

    # Use the API token provided by Github Actions if available
    If ([string]::IsNullOrEmpty($env:GITHUB_API_TOKEN) -eq $false)
    {
        Write-Host "Using API Token to access the releases"
        $headers.Add("Authorization", "Bearer " + $env:GITHUB_API_TOKEN)
    }

    $uri = "https://api.github.com/repos/codereader/DarkRadiant_windeps/releases"
    $releases = Invoke-RestMethod -Headers $headers -Uri $uri

    $latest = $releases | sort created_at -Descending | select -First 1

    $windeps_package = $latest.assets | ? { $_.name -eq "windeps.7z" }

    if ($null -eq $windeps_package)
    {
        throw "Could not find the windows dependencies package URL"
    }

    $package_filename = [System.IO.Path]::GetFileNameWithoutExtension($windeps_package.name) + `
                        "_" + $windeps_package.id + [System.IO.Path]::GetExtension($windeps_package.name)

    $target_folder = Join-Path (Get-Location) "..\..\"
    $package_path = Join-Path $target_folder $package_filename

    Write-Host ("Found package {1} ({0:0.0} MB)" -f ($windeps_package.size / 1024 / 1024), $windeps_package.name)

    if ((Get-ChildItem $package_path -ErrorAction SilentlyContinue).Length -eq $windeps_package.size)
    {
        Write-Host "Package has already been downloaded to $package_path"
    }
    else
    {
        Write-Host ("Downloading dependencies package ({0:0.0} MB)" -f ($windeps_package.size / 1024 / 1024))
        Invoke-WebRequest $windeps_package.browser_download_url -OutFile $package_path
    }

    Write-Host ("Cleaning up old dependencies...")

    Remove-Item (Join-Path $target_folder "w32deps") -Recurse -ErrorAction SilentlyContinue
    Remove-Item (Join-Path $target_folder "w64deps") -Recurse -ErrorAction SilentlyContinue
    Remove-Item (Join-Path $target_folder "windeps") -Recurse -ErrorAction SilentlyContinue
    
    Write-Host ("Extracting dependencies package...")

    & ..\7z\7za.exe x $package_path -o"$target_folder" -aoa

    if ((Get-ChildItem "..\..\windeps" -ErrorAction SilentlyContinue) -eq $null)
    {
        throw "windeps folder still not present after download... cannot continue"
    }
}
catch
{
    Write-Host -Foreground Red ("Error: {0}" -f $_)
    exit 1
}
