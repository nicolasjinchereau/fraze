# Runs the demo's 3D scene for WaitSeconds, closes it and prints its output. Exits with 0 when the scene finished
# loading and the demo exited cleanly, and 1 otherwise.
param(
    [ValidateSet('Debug', 'Release')] [string]$Configuration = 'Debug',
    [int]$WaitSeconds = 30
)

$outputDir = Join-Path $PSScriptRoot output
$stdoutPath = Join-Path $outputDir stdout.txt
$stderrPath = Join-Path $outputDir stderr.txt
New-Item -ItemType Directory -Force $outputDir | Out-Null

$process = Start-Process (Join-Path $PSScriptRoot "bin\x64\$Configuration\Demo.exe") -WorkingDirectory $PSScriptRoot `
    -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath -PassThru

# The scene never exits on its own. CloseMainWindow() does nothing until the window is up, so the close is retried,
# because a force-kill loses all of the buffered output.
if(!$process.WaitForExit($WaitSeconds * 1000)) {
    for($i = 0; $i -lt 60; $i++) { if($process.CloseMainWindow() -or $process.WaitForExit(1000)) { break } }
    if(!$process.WaitForExit(10000)) { Stop-Process -Id $process.Id -Force }
}

$stdout = @(Get-Content $stdoutPath)
$stdout
Get-Content $stderrPath

$isSceneLoaded = $stdout -contains 'Finished loading scene.'
if($process.ExitCode -eq 0 -and $isSceneLoaded) { exit 0 }

if($process.ExitCode -eq 0) {
    'The window closed before the scene finished loading; rerun with a larger -WaitSeconds.'
}
exit 1
