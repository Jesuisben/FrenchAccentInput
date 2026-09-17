param(
    [switch]$SkipInstaller
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $ProjectRoot

function Find-Tool {
    param([string]$Name, [string[]]$Candidates)

    $Command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($Command) { return $Command.Source }

    foreach ($Candidate in $Candidates) {
        if (Test-Path -LiteralPath $Candidate) { return $Candidate }
    }
    throw "$Name 도구를 찾지 못했습니다. Visual Studio Build Tools 또는 Inno Setup 설치를 확인하세요."
}

# PATH에 없는 Visual Studio bundled CMake도 같은 명령으로 찾는다.
$CMake = Find-Tool 'cmake' @(
    'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
    'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
)
$CTest = Join-Path (Split-Path -Parent $CMake) 'ctest.exe'

& $CMake -S . -B build -G 'Visual Studio 18 2026' -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configure 실패' }

& $CMake --build build --config Release --target FrenchAccentInput fai_tests fai_output_tests
if ($LASTEXITCODE -ne 0) { throw 'Release build 실패' }

& $CTest --test-dir build -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'CTest 실패' }

New-Item -ItemType Directory -Path dist -Force | Out-Null
Copy-Item -LiteralPath 'build\Release\FrenchAccentInput.exe' -Destination 'dist\FrenchAccentInput.exe' -Force

if (-not $SkipInstaller) {
    $ISCC = Find-Tool 'ISCC' @(
        'C:\Program Files\Inno Setup 7\ISCC.exe',
        'C:\Program Files (x86)\Inno Setup 7\ISCC.exe',
        'C:\Program Files\Inno Setup 6\ISCC.exe',
        'C:\Program Files (x86)\Inno Setup 6\ISCC.exe'
    )
    & $ISCC 'installer\FrenchAccentInput.iss'
    if ($LASTEXITCODE -ne 0) { throw 'Inno Setup compile 실패' }
}

$Artifacts = Get-ChildItem -LiteralPath dist -File | Where-Object Extension -eq '.exe' | Sort-Object Name
$Lines = foreach ($Artifact in $Artifacts) {
    # Release 사용자는 다운로드 파일이 제작자가 만든 byte와 같은지 SHA-256으로 확인한다.
    $Hash = (Get-FileHash -LiteralPath $Artifact.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    "$Hash  $($Artifact.Name)"
}
$Lines | Set-Content -LiteralPath 'dist\SHA256SUMS.txt' -Encoding ascii

$Artifacts | Select-Object Name, Length, LastWriteTime
Get-Content -LiteralPath 'dist\SHA256SUMS.txt'
