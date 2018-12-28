#
# Twili Driver Bootstrap Script
#
# This script downloads support tooling and libusbK, to set up the 
# on-disk hierarchy to support driver installation via twili.inf
#

$LibusbkUrl    = "https://sourceforge.net/projects/libusbk/files/libusbK-release/3.0.7.0/libusbK-3.0.7.0-bin.7z/download"
$LibusbkPrefix = "libusbK-3.0.7.0"
$7zipUrl       = "https://www.7-zip.org/a/7z1805.msi"

New-Item -ItemType Directory tools -ErrorAction Stop | Out-Null
$env:Path += ";.\tools"

# # #
Write-Output "• Staging 7-Zip"

$WorkingFilename = [IO.Path]::GetRandomFileName()
$WorkingDirectory = [IO.Path]::GetRandomFileName()
Invoke-WebRequest $7zipUrl -OutFile $WorkingFilename
Start-Process msiexec.exe -Wait -NoNewWindow -ArgumentList @(
    "/q","/a",$WorkingFilename,"TARGETDIR=$((Get-Item -Path '.\' -Verbose).FullName)\$WorkingDirectory")
Move-Item .\$WorkingDirectory\Files\7-Zip\7z.??? .\tools
Remove-Item .\$WorkingFilename
Remove-Item .\$WorkingDirectory -Recurse

# # #
Write-Output "• Staging libusbk"

New-Item -ItemType Directory @("amd64", "x86") -ErrorAction Stop | Out-Null
$WorkingFilename = [IO.Path]::GetRandomFileName()
$WorkingDirectory = [IO.Path]::GetRandomFileName()
Invoke-WebRequest $LibusbkUrl -OutFile $WorkingFilename -MaximumRedirection 3 -UserAgent "PowerShell/Windows"
Start-Process 7z.exe -Wait -ArgumentList @("x", $WorkingFilename, "-i!*\bin\dll", "-i!*\bin\sys", "-o$WorkingDirectory")

# # #
Write-Output "• Creating driver directory hierarchy"
Move-Item .\$WorkingDirectory\$LibusbkPrefix-bin\bin\*\x86\* .\x86
Move-Item .\$WorkingDirectory\$LibusbkPrefix-bin\bin\*\amd64\* .\amd64

# # #
Write-Output "• Cleaning up"
Remove-Item .\$WorkingFilename
Remove-Item .\$WorkingDirectory -Recurse

# # #
Write-Output "✓ Done"
