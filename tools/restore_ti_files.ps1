#Requires -Version 5.1
<#
.SYNOPSIS
    Restores the TI-licensed files that are not redistributed with this
    repository.

.DESCRIPTION
    The files listed below are part of TI's MSP430Ware / CC430 RF examples and
    are distributed under TI's "MSP Source and Object Code Software License
    Agreement", which does not permit redistribution. This script copies them
    from a local TI installation into the two CCS projects.

    By running this script you accept TI's license terms for those files.

.PARAMETER CcsRoot
    Root of a local TI installation. Default: C:\ti

.PARAMETER SourceRoot
    Optional additional directory to search (for example an unpacked
    MSP430Ware archive or your original CCS workspace).

.PARAMETER Force
    Overwrite files that already exist.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools\restore_ti_files.ps1

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools\restore_ti_files.ps1 `
        -SourceRoot "D:\downloads\msp430ware" -Force
#>
[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [string]$CcsRoot = "C:\ti",
    [string]$SourceRoot,
    [switch]$Force
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot

$files = @(
    @{ Name = "RF_Toggle_LED_Demo.h"; Dest = "BWST-sensor-demo\RF_Toggle_LED_Demo.h" },
    @{ Name = "RF_Toggle_LED_Demo.h"; Dest = "BWTS-Receiver-demo\RF_Toggle_LED_Demo.h" },
    @{ Name = "HAL_MACROS.h";         Dest = "BWST-sensor-demo\HAL\HAL_MACROS.h" },
    @{ Name = "HAL_PMM.c";            Dest = "BWST-sensor-demo\HAL\HAL_PMM.c" },
    @{ Name = "HAL_PMM.h";            Dest = "BWST-sensor-demo\HAL\HAL_PMM.h" },
    @{ Name = "hal_pmm.c";            Dest = "BWTS-Receiver-demo\HAL\hal_pmm.c" },
    @{ Name = "hal_pmm.h";            Dest = "BWTS-Receiver-demo\HAL\hal_pmm.h" }
)

$searchRoots = @()
if (Test-Path -LiteralPath $CcsRoot) {
    $searchRoots += (Resolve-Path -LiteralPath $CcsRoot).Path
} else {
    Write-Warning "CcsRoot not found: $CcsRoot"
}

if ($SourceRoot) {
    if (-not (Test-Path -LiteralPath $SourceRoot)) {
        throw "SourceRoot not found: $SourceRoot"
    }
    $searchRoots += (Resolve-Path -LiteralPath $SourceRoot).Path
}

if ($searchRoots.Count -eq 0) {
    throw "Nothing to search. Pass -CcsRoot or -SourceRoot pointing at a TI installation."
}

Write-Host "Searching: $($searchRoots -join ', ')"

$missing = @()

foreach ($item in $files) {
    $dest = Join-Path $RepoRoot $item.Dest
    $destDir = Split-Path -Parent $dest

    if ((Test-Path -LiteralPath $dest) -and -not $Force) {
        Write-Host "SKIP (exists): $($item.Dest)"
        continue
    }

    $hits = @()
    foreach ($root in $searchRoots) {
        $hits += Get-ChildItem -LiteralPath $root -Recurse -File -ErrorAction SilentlyContinue |
            Where-Object {
                # -ceq keeps HAL_PMM.* and hal_pmm.* distinct.
                $_.Name -ceq $item.Name -and $_.FullName -notlike "$RepoRoot*"
            }
    }
    $hits = @($hits | Sort-Object FullName -Unique)

    if ($hits.Count -eq 0) {
        Write-Warning "NOT FOUND: $($item.Name)  (needed as $($item.Dest))"
        $missing += $item
        continue
    }

    if ($hits.Count -gt 1) {
        Write-Warning "MULTIPLE candidates for $($item.Name); nothing copied. Point -SourceRoot at the correct package, or copy manually:"
        $hits | ForEach-Object { Write-Host "    $($_.FullName)" }
        $missing += $item
        continue
    }

    if (-not (Test-Path -LiteralPath $destDir)) {
        New-Item -ItemType Directory -Path $destDir -Force | Out-Null
    }

    if ($PSCmdlet.ShouldProcess($dest, "Copy from $($hits[0].FullName)")) {
        Copy-Item -LiteralPath $hits[0].FullName -Destination $dest -Force
        Write-Host "COPIED: $($item.Dest)  <-  $($hits[0].FullName)"
    }
}

if ($missing.Count -gt 0) {
    Write-Host ""
    Write-Warning "$($missing.Count) file(s) still missing."
    Write-Host "Install MSP430Ware and the CC430 RF examples through CCS Resource Explorer, then re-run:"
    Write-Host '  powershell -ExecutionPolicy Bypass -File tools\restore_ti_files.ps1 -CcsRoot "C:\ti"'
    exit 1
}

Write-Host ""
Write-Host "All required TI files restored."
Write-Host "Reminder: these files remain subject to TI's MSP Source and Object Code Software License Agreement."
