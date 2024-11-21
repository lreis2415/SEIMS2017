param ($gdalPath = "$env:SystemDrive\gdal", $VSversion = "1928", $GDALversion = "3.9.1", $MAPSversion = "8.2.0")
$GDALversion=$GDALversion -replace '\.','-'
$MAPSversion=$MAPSversion -replace '\.','-'
$urllib = "https://download.gisinternals.com/sdk/downloads/release-$VSversion-x64-gdal-$GDALversion-mapserver-$MAPSversion-libs.zip"
$urlbin = "https://download.gisinternals.com/sdk/downloads/release-$VSversion-x64-gdal-$GDALversion-mapserver-$MAPSversion.zip"
$zipLibFile = "$gdalPath\gdallib.zip"
$zipBinFile = "$gdalPath\gdal.zip"

# Check if gdalPath existed
if ((Test-Path -path $gdalPath) -eq $false) {
    mkdir $gdalPath
}
Set-Location $gdalPath
Write-Host "Downloading GDAL-$GDALversion-mapserver-$MAPSversion built by VS-$VSversion……"
$webClient = New-Object System.Net.WebClient
Write-Host $urllib
$webClient.DownloadFile($urllib,$zipLibFile)
Write-Host $urlbin
$webClient.DownloadFile($urlbin,$zipBinFile)
Expand-Archive -Path $zipLibFile -DestinationPath "$gdalPath"
Expand-Archive -Path $zipBinFile -DestinationPath "$gdalPath"
Get-ChildItem
Write-Host "Setting environmetal paths of GDAL……"
$env:GDAL_ROOT = $gdalPath
$env:GDAL_DATA = "$gdalPath\bin\gdal-data"
$env:GDAL_BIN = "$gdalPath\bin;$gdalPath\include;$gdalPath\lib;$gdalPath\bin\gdal\apps;$gdalPath\bin\gdal\java;$gdalPath\bin\proj\apps;$gdalPath\bin\curl;"
$env:PATH = "$env:GDAL_BIN;$env:PATH"

Write-Output "GDAL_ROOT=$env:GDAL_ROOT"
Write-Output "GDAL_DATA=$env:GDAL_DATA"
Write-Output "GDAL_BIN=$env:GDAL_BIN"
Write-Output "PATH=$env:PATH"
