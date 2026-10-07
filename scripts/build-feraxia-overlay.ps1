param(
  [switch]$PrepareDependencies,
  [ValidateRange(1, 32)][int]$Jobs = 4
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$previousLocation = Get-Location
$previousGenerator = $env:CMAKE_GENERATOR
try {
  Set-Location -LiteralPath $repositoryRoot
  $modules = @('third-party/common/win', 'third-party/build/win')
  if ($PrepareDependencies) { $modules += @('third-party/deps/common', 'third-party/deps/win') }
  foreach ($module in $modules) {
    # Forks may contain only the default branch. These inputs are pinned gitlinks
    # in the feature base and are fetched from their original upstream network.
    & git config "submodule.$module.url" 'https://github.com/Detanup01/gbe_fork.git'
    if ($LASTEXITCODE -ne 0) { throw 'Could not configure pinned dependency source.' }
  }
  & git submodule update --init --depth 1 --jobs $Jobs -- @modules
  if ($LASTEXITCODE -ne 0) { throw 'Could not initialize pinned Windows dependencies.' }

  $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
  $vsRoot = & $vswhere -latest -products '*' -version '[17.0,18.0)' -requires Microsoft.Component.MSBuild -property installationPath
  if (!$vsRoot) { throw 'Visual Studio 2022 with C++ tools and MSBuild is required.' }
  $msbuild = Join-Path $vsRoot 'MSBuild/Current/Bin/MSBuild.exe'
  $premake = Join-Path $repositoryRoot 'third-party/common/win/premake/premake5.exe'
  $env:CMAKE_GENERATOR = 'Visual Studio 17 2022'
  if ($PrepareDependencies) {
    & $premake --file=premake5-deps.lua --64-build --32-build --all-ext --all-build "--j=$Jobs" --os=windows vs2022
    if ($LASTEXITCODE -ne 0) { throw 'Windows dependency build failed.' }
  }
  $protoc = Join-Path $repositoryRoot 'build/deps/win/vs2022/protobuf/install64/bin/protoc.exe'
  if (!(Test-Path -LiteralPath $protoc)) { throw 'Build dependencies first with -PrepareDependencies.' }
  $revision = (& git rev-parse --short=12 HEAD).Trim()
  if ($LASTEXITCODE -ne 0) { throw 'Could not determine source revision.' }
  $dirty = @(& git status --porcelain).Count -gt 0
  $buildId = "feraxia-$revision"
  if ($dirty) { $buildId += '+local' }
  # Deliberately omit upstream --winsign (fake certificate signing).
  & $premake --file=premake5.lua --genproto --dosstub --winrsrc "--emubuild=$buildId" --os=windows vs2022
  if ($LASTEXITCODE -ne 0) { throw 'Overlay project generation failed.' }
  foreach ($platform in @('x64', 'Win32')) {
    & $msbuild /nologo /m:1 "/p:CL_MPCount=$Jobs" /v:minimal /p:Configuration=release "/p:Platform=$platform" /target:api_experimental build/project/vs2022/win/gbe.sln
    if ($LASTEXITCODE -ne 0) { throw "Experimental overlay build failed: $platform" }
  }
  Write-Output "Built experimental x86/x64 overlay $buildId under build/win/vs2022/release/experimental."
  Write-Output 'App resources were not replaced. Verify hashes, corresponding source, and game behavior before integration.'
} finally {
  [Environment]::SetEnvironmentVariable('CMAKE_GENERATOR', $previousGenerator, 'Process')
  Set-Location -LiteralPath $previousLocation
}
