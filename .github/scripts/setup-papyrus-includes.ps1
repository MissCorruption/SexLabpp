# Stages Papyrus include directories for CI Caprica compiles.
# Layout mirrors SexLabpp xmake.lua include names where practical.
param(
    [string]$PapyrusRoot = ".github/Papyrus",
    [string]$Workspace = $env:GITHUB_WORKSPACE
)

$ErrorActionPreference = "Stop"
Set-Location $Workspace

New-Item -ItemType Directory -Path $PapyrusRoot -Force | Out-Null

function Sparse-Checkout {
    param(
        [string]$Url,
        [string]$Branch,
        [string]$SparsePath,
        [string]$DestName
    )
    $dest = Join-Path $PapyrusRoot $DestName
    if (Test-Path $dest) {
        Write-Host "Already present: $dest"
        return
    }
    $tmp = Join-Path $PapyrusRoot ("_tmp_" + $DestName)
    New-Item -ItemType Directory -Path $tmp -Force | Out-Null
    Push-Location $tmp
    try {
        git init
        git remote add origin $Url
        git sparse-checkout set --no-cone $SparsePath
        git fetch --depth=1 origin $Branch
        git checkout $Branch
        Remove-Item .git -Recurse -Force
        $src = Join-Path $tmp $SparsePath
        if (-not (Test-Path $src)) {
            throw "Sparse path missing after checkout: $SparsePath"
        }
        New-Item -ItemType Directory -Path (Split-Path $dest -Parent) -Force | Out-Null
        Move-Item $src $dest
    }
    finally {
        Pop-Location
        if (Test-Path $tmp) {
            Remove-Item $tmp -Recurse -Force
        }
    }
}

# Vanilla + SkyUI + PapyrusUtil (same bundle QuickLoot uses)
Sparse-Checkout -Url "https://github.com/IHateMyKite/PAPYRUS.git" -Branch "main" -SparsePath "SRC" -DestName "SRC"
Sparse-Checkout -Url "https://github.com/IHateMyKite/PAPYRUS.git" -Branch "main" -SparsePath "SRC_SKYUI" -DestName "SRC_SKYUI"
Sparse-Checkout -Url "https://github.com/IHateMyKite/PAPYRUS.git" -Branch "main" -SparsePath "SRC_PAPUTIL" -DestName "SRC_PAPUTIL"

# TESV flags (if not already under SRC)
$flags = Join-Path $PapyrusRoot "SRC/TESV_Papyrus_Flags.flg"
if (-not (Test-Path $flags)) {
    $tmp = Join-Path $PapyrusRoot "_tmp_flags"
    New-Item -ItemType Directory -Path $tmp -Force | Out-Null
    Push-Location $tmp
    try {
        git init
        git remote add origin "https://github.com/Rukan/Grimy-Skyrim-Papyrus-Source.git"
        git sparse-checkout set --no-cone "TESV_Papyrus_Flags.flg"
        git fetch --depth=1 origin master
        git checkout master
        Move-Item "TESV_Papyrus_Flags.flg" $flags
    }
    finally {
        Pop-Location
        Remove-Item $tmp -Recurse -Force -ErrorAction SilentlyContinue
    }
}

# NiOverride (RaceMenu)
$raceMenu = Join-Path $PapyrusRoot "SRC_RACEMENU"
if (-not (Test-Path $raceMenu)) {
    New-Item -ItemType Directory -Path $raceMenu -Force | Out-Null
    $tmp = Join-Path $PapyrusRoot "_tmp_nio"
    New-Item -ItemType Directory -Path $tmp -Force | Out-Null
    Push-Location $tmp
    try {
        git init
        git remote add origin "https://github.com/Rukan/Grimy-Skyrim-Papyrus-Source.git"
        git sparse-checkout set --no-cone "NiOverride.psc"
        git fetch --depth=1 origin master
        git checkout master
        Move-Item "NiOverride.psc" (Join-Path $raceMenu "NiOverride.psc")
    }
    finally {
        Pop-Location
        Remove-Item $tmp -Recurse -Force -ErrorAction SilentlyContinue
    }
}

# MfgFix NG
$mfg = Join-Path $PapyrusRoot "SRC_MFG"
if (-not (Test-Path $mfg)) {
    New-Item -ItemType Directory -Path $mfg -Force | Out-Null
    $tmp = Join-Path $PapyrusRoot "_tmp_mfg"
    New-Item -ItemType Directory -Path $tmp -Force | Out-Null
    Push-Location $tmp
    try {
        git init
        git remote add origin "https://github.com/KrisV-777/Mfg-Fix-NG.git"
        git sparse-checkout set --no-cone "dist/source/scripts"
        git fetch --depth=1 origin main
        git checkout main
        Copy-Item "dist/source/scripts/*.psc" $mfg
    }
    finally {
        Pop-Location
        Remove-Item $tmp -Recurse -Force -ErrorAction SilentlyContinue
    }
}

# SkyrimLovense
$lovense = Join-Path $PapyrusRoot "SRC_LOVENSE"
if (-not (Test-Path $lovense)) {
    New-Item -ItemType Directory -Path $lovense -Force | Out-Null
    $tmp = Join-Path $PapyrusRoot "_tmp_lovense"
    New-Item -ItemType Directory -Path $tmp -Force | Out-Null
    Push-Location $tmp
    try {
        git init
        git remote add origin "https://github.com/KrisV-777/Skyrim-Lovense.git"
        git sparse-checkout set --no-cone "dist/Source/Scripts"
        git fetch --depth=1 origin main
        git checkout main
        Copy-Item "dist/Source/Scripts/*.psc" $lovense
    }
    finally {
        Pop-Location
        Remove-Item $tmp -Recurse -Force -ErrorAction SilentlyContinue
    }
}

# VRIK CI stub
$vrik = Join-Path $PapyrusRoot "SRC_VRIK"
New-Item -ItemType Directory -Path $vrik -Force | Out-Null
Copy-Item ".github/ci/papyrus-stubs/VRIK.psc" (Join-Path $vrik "VRIK.psc") -Force

Write-Host "Papyrus includes ready under $PapyrusRoot"
Get-ChildItem $PapyrusRoot -Directory | ForEach-Object { Write-Host " - $($_.Name)" }
