param(
    [ValidateSet("Doctor", "Configure", "Build", "Apk", "Install")]
    [string] $Action = "Build",
    [string] $DeviceSerial = "",
    [string] $BuildDirectory = ""
)

$ErrorActionPreference = "Stop"

function Find-ExistingPath {
    param(
        [string] $Description,
        [string[]] $Candidates
    )

    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate)) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    throw "Unable to locate $Description. Checked: $($Candidates -join ', ')"
}

function Find-CommandPath {
    param(
        [string] $Description,
        [string] $Override,
        [string] $CommandName,
        [string[]] $Candidates
    )

    if ($Override) {
        return Find-ExistingPath $Description @($Override)
    }

    $command = Get-Command $CommandName -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    return Find-ExistingPath $Description $Candidates
}

function Find-LatestQtAndroid {
    if ($env:QK4_QT_ANDROID) {
        return Find-ExistingPath "Qt Android ARM64 kit" @($env:QK4_QT_ANDROID)
    }

    $kits = Get-ChildItem -LiteralPath "C:\Qt" -Directory -ErrorAction SilentlyContinue |
        ForEach-Object {
            $candidate = Join-Path $_.FullName "android_arm64_v8a"
            if (Test-Path -LiteralPath $candidate) {
                Get-Item -LiteralPath $candidate
            }
        } |
        Sort-Object { [version]$_.Parent.Name } -Descending

    if (-not $kits) {
        throw "Unable to locate a Qt Android ARM64 kit. Set QK4_QT_ANDROID."
    }

    return $kits[0].FullName
}

function Find-LatestNdk {
    param([string] $AndroidSdk)

    if ($env:ANDROID_NDK_ROOT) {
        return Find-ExistingPath "Android NDK" @($env:ANDROID_NDK_ROOT)
    }

    $ndkRoot = Join-Path $AndroidSdk "ndk"
    $ndks = Get-ChildItem -LiteralPath $ndkRoot -Directory -ErrorAction SilentlyContinue |
        Sort-Object { [version]$_.Name } -Descending

    if (-not $ndks) {
        throw "Unable to locate an Android NDK below $ndkRoot. Set ANDROID_NDK_ROOT."
    }

    return $ndks[0].FullName
}

$projectDir = $PSScriptRoot
if (-not $BuildDirectory) {
    $BuildDirectory = Join-Path $projectDir "build-android-arm64"
}
$buildDir = [System.IO.Path]::GetFullPath($BuildDirectory)

$androidSdk = Find-ExistingPath "Android SDK" @(
    $env:ANDROID_SDK_ROOT,
    $env:ANDROID_HOME,
    (Join-Path $env:LOCALAPPDATA "Android\Sdk")
)
$androidNdk = Find-LatestNdk $androidSdk
$qtAndroid = Find-LatestQtAndroid

$qtVersionRoot = Split-Path -Parent $qtAndroid
$qtHost = Find-ExistingPath "matching Qt Windows host kit" @(
    $env:QK4_QT_HOST,
    (Join-Path $qtVersionRoot "mingw_64")
)

$cmake = Find-CommandPath "CMake" $env:QK4_CMAKE "cmake.exe" @(
    "C:\Qt\Tools\CMake_64\bin\cmake.exe"
)
$ninja = Find-CommandPath "Ninja" $env:QK4_NINJA "ninja.exe" @(
    "C:\Qt\Tools\Ninja\ninja.exe"
)
$javaHome = Find-ExistingPath "Java/JDK" @(
    $env:QK4_JAVA_HOME,
    "C:\Program Files\Android\Android Studio\jbr",
    $env:JAVA_HOME
)
$adb = Find-ExistingPath "Android Debug Bridge" @(
    (Join-Path $androidSdk "platform-tools\adb.exe")
)
$androidDeployQt = Find-ExistingPath "androiddeployqt" @(
    (Join-Path $qtHost "bin\androiddeployqt.exe")
)
$opusRoot = Find-ExistingPath "Android ARM64 Opus development files" @(
    $env:QK4_OPUS_ROOT,
    (Join-Path $projectDir "third_party\android\opus"),
    (Join-Path (Split-Path -Parent $projectDir) "qk4-android-deps\opus")
)
$opusHeader = Find-ExistingPath "Opus header" @(
    (Join-Path $opusRoot "include\opus\opus.h")
)
$opusLibrary = Find-ExistingPath "Android ARM64 Opus library" @(
    (Join-Path $opusRoot "lib\libopus.a")
)

function Show-AndroidEnvironment {
    Write-Host "QK4 Android build environment is ready:"
    Write-Host "  Project: $projectDir"
    Write-Host "  Qt Android: $qtAndroid"
    Write-Host "  Qt host: $qtHost"
    Write-Host "  Android SDK: $androidSdk"
    Write-Host "  Android NDK: $androidNdk"
    Write-Host "  Java: $javaHome"
    Write-Host "  CMake: $cmake"
    Write-Host "  Ninja: $ninja"
    Write-Host "  ADB: $adb"
    Write-Host "  androiddeployqt: $androidDeployQt"
    Write-Host "  Opus: $opusRoot"
}

function Configure-AndroidBuild {
    $opusInclude = Split-Path -Parent (Split-Path -Parent $opusHeader)

    & $cmake `
        -S $projectDir `
        -B $buildDir `
        -G Ninja `
        "-DCMAKE_MAKE_PROGRAM=$ninja" `
        -DCMAKE_BUILD_TYPE=Release `
        "-DCMAKE_TOOLCHAIN_FILE=$qtAndroid\lib\cmake\Qt6\qt.toolchain.cmake" `
        "-DQT_HOST_PATH=$qtHost" `
        "-DANDROID_SDK_ROOT=$androidSdk" `
        "-DANDROID_NDK_ROOT=$androidNdk" `
        "-DQT_CHAINLOAD_TOOLCHAIN_FILE=$androidNdk\build\cmake\android.toolchain.cmake" `
        -DANDROID_ABI=arm64-v8a `
        -DANDROID_PLATFORM=android-26 `
        "-DQK4_ANDROID_DEPLOYMENT_TYPE=DEBUG" `
        "-DQK4_OPUS_INCLUDE_DIR=$opusInclude" `
        "-DQK4_OPUS_LIBRARY=$opusLibrary"

    if ($LASTEXITCODE -ne 0) {
        throw "Android configuration failed with exit code $LASTEXITCODE."
    }
}

if ($Action -eq "Doctor") {
    Show-AndroidEnvironment
    exit 0
}

if ($Action -eq "Configure") {
    Configure-AndroidBuild
    Write-Host "Configured Android build in $buildDir"
    exit 0
}

if (-not (Test-Path -LiteralPath (Join-Path $buildDir "CMakeCache.txt"))) {
    Configure-AndroidBuild
}

if ($Action -eq "Build") {
    & $cmake --build $buildDir --target QK4 --parallel 4
    if ($LASTEXITCODE -ne 0) {
        throw "Android build failed with exit code $LASTEXITCODE."
    }
    exit 0
}

$packageDir = Join-Path $buildDir "android-build"
$deploymentSettings = Join-Path $buildDir "android-QK4-deployment-settings.json"
$applicationLibrary = Join-Path $buildDir "libQK4_arm64-v8a.so"
$packageLibraryDir = Join-Path $packageDir "libs\arm64-v8a"
$packageLibrary = Join-Path $packageLibraryDir "libQK4_arm64-v8a.so"

$env:JAVA_HOME = $javaHome
$env:Path = "$javaHome\bin;$env:Path"

& $cmake --build $buildDir --target QK4 --parallel 4
if ($LASTEXITCODE -ne 0) {
    throw "Android build failed with exit code $LASTEXITCODE."
}

New-Item -ItemType Directory -Force -Path $packageLibraryDir | Out-Null
Copy-Item -LiteralPath $applicationLibrary -Destination $packageLibrary -Force

& $androidDeployQt `
    --input $deploymentSettings `
    --output $packageDir
if ($LASTEXITCODE -ne 0) {
    throw "Android packaging failed with exit code $LASTEXITCODE."
}

$apk = Get-ChildItem $packageDir -Filter "*.apk" -Recurse |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1

if (-not $apk) {
    throw "The APK target completed but no APK was produced under $packageDir."
}

Write-Host "APK: $($apk.FullName)"

if ($Action -eq "Install") {
    $adbArgs = @()
    if ($DeviceSerial) {
        $adbArgs += @("-s", $DeviceSerial)
    }
    $adbArgs += @("install", "-r", $apk.FullName)

    & $adb @adbArgs
    if ($LASTEXITCODE -ne 0) {
        throw "APK installation failed with exit code $LASTEXITCODE."
    }
}
