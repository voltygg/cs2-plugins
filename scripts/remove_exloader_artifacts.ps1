<#
.SYNOPSIS
Removes ExLoader, OsirisV2, and Enigma HvH artifacts from this Windows user profile.

.DESCRIPTION
This cleanup is scoped to installed/downloaded artifacts and matching persistence points:
AppData ExLoader data, OsirisCS2 config data, the downloaded ExLoader installer, matching
startup registry values, scheduled tasks, services, system drivers, WMI command consumers,
temp remnants, and app-specific Recent shortcuts.

Use -RemoveBundledApps, -RemoveOpera, or -RemoveAvast to also remove Opera and/or Avast
artifacts that were bundled by the installer. Avast removal uses registered uninstallers
when present and does not bypass Avast self-defense.

Beyond installed files it also removes stale Microsoft Defender exclusions that match the
artifacts and clears targeted history traces that reference them (shell MRUs always; and,
with -EraseHistory, Prefetch, jump lists, and PowerShell console history).

Windows Event Logs are append-only, so individual records referencing the artifacts cannot
be selectively deleted. The script reports where they appear; pass -ClearEventLogs to clear
entire logs, which also discards unrelated audit data and cannot be undone.

It does not touch AmCache, ShimCache, SRUM, the USN journal, or browser history.

.EXAMPLE
powershell -ExecutionPolicy Bypass -File .\scripts\remove_exloader_artifacts.ps1 -WhatIf

.EXAMPLE
powershell -ExecutionPolicy Bypass -File .\scripts\remove_exloader_artifacts.ps1 -Force

.EXAMPLE
powershell -ExecutionPolicy Bypass -File .\scripts\remove_exloader_artifacts.ps1 -Force -RemoveBundledApps -EraseHistory

.NOTES
When launching with -File, pass -Force (not -Confirm:$false, which -File would treat as a
literal string). -Confirm:$false still works when dot-sourcing or calling the script with &.
#>

[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'High')]
param(
    [switch]$SkipDownloads,
    [switch]$SkipRecentShortcuts,
    [switch]$SkipTempScan,
    [switch]$RemoveBundledApps,
    [switch]$RemoveOpera,
    [switch]$RemoveAvast,
    [switch]$RunDefenderQuickScan,
    [switch]$EraseHistory,
    [switch]$ClearEventLogs,
    [switch]$Force
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Continue'

# -Force runs without confirmation prompts. Use this instead of -Confirm:$false when
# launching via `powershell -File`, which passes -Confirm:$false as an unparsed string.
if ($Force) {
    $ConfirmPreference = 'None'
}

$SignalPattern = '(?i)(exloader|osiris|swiftsoft|enigma[_\s-]?hvh|enigmav2)'
$OperaPattern = '(?i)(Opera Software|Programs\\Opera(\\|$)|\\Opera(\\|$)|^Opera(\s|$)|^Opera$|opera_autoupdate|^\.opera$)'
$AvastPattern = '(?i)(\bAvast\b|AVAST Software|Avast Software)'
$ShouldRemoveOpera = [bool]($RemoveBundledApps -or $RemoveOpera)
$ShouldRemoveAvast = [bool]($RemoveBundledApps -or $RemoveAvast)
$RemovedCount = 0
$SkippedCount = 0
$ErrorCount = 0

$originalWhatIfPreference = $WhatIfPreference
$WhatIfPreference = $false
Import-Module CimCmdlets -ErrorAction SilentlyContinue -WarningAction SilentlyContinue
$WhatIfPreference = $originalWhatIfPreference

function Write-Section {
    param([Parameter(Mandatory)][string]$Message)
    Write-Host ""
    Write-Host "== $Message =="
}

function Write-Removed {
    param([Parameter(Mandatory)][string]$Message)
    $script:RemovedCount++
    Write-Host "[removed] $Message"
}

function Write-Skipped {
    param([Parameter(Mandatory)][string]$Message)
    $script:SkippedCount++
    Write-Host "[skip] $Message"
}

function Write-CleanupError {
    param(
        [Parameter(Mandatory)][string]$Message,
        [Parameter(Mandatory)][System.Management.Automation.ErrorRecord]$ErrorRecord
    )
    $script:ErrorCount++
    Write-Warning "$Message`: $($ErrorRecord.Exception.Message)"
}

function Test-IsAdmin {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = [Security.Principal.WindowsPrincipal]::new($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Normalize-PathText {
    param([Parameter(Mandatory)][string]$Path)
    return [System.IO.Path]::GetFullPath($Path).TrimEnd('\')
}

function Get-OptionalPropertyValue {
    param(
        [Parameter(Mandatory)][AllowNull()][object]$Object,
        [Parameter(Mandatory)][string]$Name
    )

    if ($null -eq $Object) {
        return ''
    }

    $property = $Object.PSObject.Properties[$Name]
    if (-not $property) {
        return ''
    }

    return [string]$property.Value
}

function Select-TopLevelPaths {
    param([Parameter(Mandatory)][AllowEmptyCollection()][string[]]$Paths)

    $topLevelPaths = [System.Collections.Generic.List[string]]::new()
    if (-not $Paths) {
        return $topLevelPaths.ToArray()
    }

    foreach ($path in $Paths | Sort-Object { $_.Length }) {
        $fullPath = Normalize-PathText $path
        $coveredByExistingPath = $false

        foreach ($existingPath in $topLevelPaths) {
            if ($fullPath.Equals($existingPath, [StringComparison]::OrdinalIgnoreCase) -or
                $fullPath.StartsWith("$existingPath\", [StringComparison]::OrdinalIgnoreCase)) {
                $coveredByExistingPath = $true
                break
            }
        }

        if (-not $coveredByExistingPath) {
            $topLevelPaths.Add($fullPath)
        }
    }

    return $topLevelPaths.ToArray()
}

function Test-PathUnderAllowedRoot {
    param([Parameter(Mandatory)][string]$Path)

    $publicDesktop = if ($env:PUBLIC) { Join-Path $env:PUBLIC 'Desktop' } else { $null }
    $allowedRoots = @(
        $env:USERPROFILE,
        $env:APPDATA,
        $env:LOCALAPPDATA,
        $env:ProgramFiles,
        ${env:ProgramFiles(x86)},
        $env:PROGRAMDATA,
        $env:TEMP,
        (Join-Path $env:WINDIR 'Temp'),
        $publicDesktop
    ) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }

    $fullPath = Normalize-PathText $Path
    foreach ($root in $allowedRoots) {
        $fullRoot = Normalize-PathText $root
        if ($fullPath.Equals($fullRoot, [StringComparison]::OrdinalIgnoreCase)) {
            return $false
        }

        if ($fullPath.StartsWith("$fullRoot\", [StringComparison]::OrdinalIgnoreCase)) {
            return $true
        }
    }

    return $false
}

function Remove-ArtifactPath {
    param([Parameter(Mandatory)][string]$Path)

    $resolvedPaths = Resolve-Path -LiteralPath $Path -ErrorAction SilentlyContinue
    if (-not $resolvedPaths) {
        Write-Skipped "$Path (not present)"
        return
    }

    foreach ($resolvedPath in $resolvedPaths) {
        $fullPath = $resolvedPath.ProviderPath
        if (-not (Test-PathUnderAllowedRoot $fullPath)) {
            Write-Warning "Refusing to remove path outside allowed cleanup roots: $fullPath"
            $script:SkippedCount++
            continue
        }

        if ($PSCmdlet.ShouldProcess($fullPath, 'Remove file or directory')) {
            try {
                Remove-Item -LiteralPath $fullPath -Recurse -Force -ErrorAction Stop
                Write-Removed $fullPath
            } catch {
                Write-CleanupError "Failed to remove $fullPath" $_
            }
        }
    }
}

function Remove-EmptyDirectory {
    param([Parameter(Mandatory)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        return
    }

    $hasChildren = Get-ChildItem -LiteralPath $Path -Force -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($hasChildren) {
        return
    }

    if ($PSCmdlet.ShouldProcess($Path, 'Remove empty directory')) {
        try {
            Remove-Item -LiteralPath $Path -Force -ErrorAction Stop
            Write-Removed "$Path (empty parent)"
        } catch {
            Write-CleanupError "Failed to remove empty directory $Path" $_
        }
    }
}

function Remove-UninstallRegistryKeys {
    Write-Section 'Uninstall registry'

    $uninstallRoots = @(
        'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall',
        'HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall',
        'HKLM:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall'
    )

    $found = $false
    foreach ($root in $uninstallRoots) {
        if (-not (Test-Path -LiteralPath $root)) {
            continue
        }

        foreach ($key in Get-ChildItem -LiteralPath $root -ErrorAction SilentlyContinue) {
            $props = Get-ItemProperty -LiteralPath $key.PSPath -ErrorAction SilentlyContinue
            $haystack = @(
                $key.PSChildName,
                (Get-OptionalPropertyValue -Object $props -Name 'DisplayName'),
                (Get-OptionalPropertyValue -Object $props -Name 'InstallLocation'),
                (Get-OptionalPropertyValue -Object $props -Name 'UninstallString'),
                (Get-OptionalPropertyValue -Object $props -Name 'Publisher')
            ) -join ' '

            if ($haystack -match $SignalPattern) {
                $found = $true
                if ($PSCmdlet.ShouldProcess($key.Name, 'Remove uninstall registry key')) {
                    try {
                        Remove-Item -LiteralPath $key.PSPath -Recurse -Force -ErrorAction Stop
                        Write-Removed $key.Name
                    } catch {
                        Write-CleanupError "Failed to remove uninstall registry key $($key.Name)" $_
                    }
                }
            }
        }
    }

    if (-not $found) {
        Write-Skipped 'No matching uninstall registry keys'
    }
}

function Get-UninstallRegistryEntries {
    param([Parameter(Mandatory)][string]$Pattern)

    $uninstallRoots = @(
        'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall',
        'HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall',
        'HKLM:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall'
    )

    foreach ($root in $uninstallRoots) {
        if (-not (Test-Path -LiteralPath $root)) {
            continue
        }

        foreach ($key in Get-ChildItem -LiteralPath $root -ErrorAction SilentlyContinue) {
            $props = Get-ItemProperty -LiteralPath $key.PSPath -ErrorAction SilentlyContinue
            $haystack = @(
                $key.PSChildName,
                (Get-OptionalPropertyValue -Object $props -Name 'DisplayName'),
                (Get-OptionalPropertyValue -Object $props -Name 'InstallLocation'),
                (Get-OptionalPropertyValue -Object $props -Name 'UninstallString'),
                (Get-OptionalPropertyValue -Object $props -Name 'QuietUninstallString'),
                (Get-OptionalPropertyValue -Object $props -Name 'Publisher')
            ) -join ' '

            if ($haystack -match $Pattern) {
                [pscustomobject]@{
                    Key = $key.Name
                    PSPath = $key.PSPath
                    DisplayName = Get-OptionalPropertyValue -Object $props -Name 'DisplayName'
                    UninstallString = Get-OptionalPropertyValue -Object $props -Name 'UninstallString'
                    QuietUninstallString = Get-OptionalPropertyValue -Object $props -Name 'QuietUninstallString'
                }
            }
        }
    }
}

function Invoke-RegisteredUninstallers {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$Pattern
    )

    $entries = @(Get-UninstallRegistryEntries -Pattern $Pattern)
    if (-not $entries) {
        Write-Skipped "No registered $Name uninstallers"
        return
    }

    foreach ($entry in $entries) {
        $command = if ($entry.QuietUninstallString) { $entry.QuietUninstallString } else { $entry.UninstallString }
        if (-not $command) {
            Write-Skipped "$Name uninstall entry has no uninstall command: $($entry.Key)"
            continue
        }

        $label = if ($entry.DisplayName) { $entry.DisplayName } else { $entry.Key }
        if ($PSCmdlet.ShouldProcess($label, "Run registered uninstaller: $command")) {
            try {
                Start-Process -FilePath $env:ComSpec -ArgumentList @('/d', '/s', '/c', $command) -Wait -WindowStyle Hidden -ErrorAction Stop
                Write-Removed "Ran $Name uninstaller: $label"
            } catch {
                Write-CleanupError "Failed to run $Name uninstaller $label" $_
            }
        }
    }
}

function Remove-StartupRegistryValuesByPattern {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$Pattern
    )

    $runKeys = @(
        'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run',
        'HKCU:\Software\Microsoft\Windows\CurrentVersion\RunOnce',
        'HKLM:\Software\Microsoft\Windows\CurrentVersion\Run',
        'HKLM:\Software\Microsoft\Windows\CurrentVersion\RunOnce',
        'HKLM:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Run',
        'HKLM:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\RunOnce'
    )

    $found = $false
    foreach ($key in $runKeys) {
        if (-not (Test-Path -LiteralPath $key)) {
            continue
        }

        $properties = Get-ItemProperty -LiteralPath $key -ErrorAction SilentlyContinue
        foreach ($property in $properties.PSObject.Properties) {
            if ($property.Name -match '^PS') {
                continue
            }

            if ($property.Name -match $Pattern -or [string]$property.Value -match $Pattern) {
                $found = $true
                $label = "$key\$($property.Name)"
                if ($PSCmdlet.ShouldProcess($label, "Remove $Name startup registry value")) {
                    try {
                        Remove-ItemProperty -LiteralPath $key -Name $property.Name -Force -ErrorAction Stop
                        Write-Removed $label
                    } catch {
                        Write-CleanupError "Failed to remove $Name startup registry value $label" $_
                    }
                }
            }
        }
    }

    if (-not $found) {
        Write-Skipped "No matching $Name startup registry values"
    }
}

function Remove-ScheduledTasksByPattern {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$Pattern
    )

    $tasks = Get-ScheduledTask -ErrorAction SilentlyContinue | Where-Object {
        $_.TaskName -match $Pattern -or
        $_.TaskPath -match $Pattern -or
        (($_.Actions | Out-String) -match $Pattern)
    }

    if (-not $tasks) {
        Write-Skipped "No matching $Name scheduled tasks"
        return
    }

    foreach ($task in $tasks) {
        $label = "$($task.TaskPath)$($task.TaskName)"
        if ($PSCmdlet.ShouldProcess($label, "Unregister $Name scheduled task")) {
            try {
                Unregister-ScheduledTask -TaskName $task.TaskName -TaskPath $task.TaskPath -Confirm:$false -ErrorAction Stop
                Write-Removed $label
            } catch {
                Write-CleanupError "Failed to unregister $Name scheduled task $label" $_
            }
        }
    }
}

function Remove-RegistryKeyIfPresent {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$Path
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        Write-Skipped "$Path (not present)"
        return
    }

    if ($PSCmdlet.ShouldProcess($Path, "Remove $Name registry key")) {
        try {
            Remove-Item -LiteralPath $Path -Recurse -Force -ErrorAction Stop
            Write-Removed $Path
        } catch {
            Write-CleanupError "Failed to remove $Name registry key $Path" $_
        }
    }
}

function Stop-ProductProcesses {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$Pattern
    )

    $processes = Get-Process -ErrorAction SilentlyContinue | Where-Object {
        $_.ProcessName -match $Pattern -or
        ($_.Path -and $_.Path -match $Pattern)
    }

    if (-not $processes) {
        Write-Skipped "No matching running $Name processes"
        return
    }

    foreach ($process in $processes) {
        $label = "$($process.ProcessName) (PID $($process.Id))"
        if ($PSCmdlet.ShouldProcess($label, "Stop $Name process")) {
            try {
                Stop-Process -Id $process.Id -Force -ErrorAction Stop
                Write-Removed "Stopped $label"
            } catch {
                Write-CleanupError "Failed to stop $label" $_
            }
        }
    }
}

function Remove-MatchingServiceLikeEntries {
    param(
        [Parameter(Mandatory)][string]$SectionName,
        [Parameter(Mandatory)][object[]]$Entries
    )

    Write-Section $SectionName

    $matches = @($Entries | Where-Object {
        $_.Name -match $SignalPattern -or
        $_.DisplayName -match $SignalPattern -or
        $_.PathName -match $SignalPattern
    })

    if (-not $matches) {
        Write-Skipped "No matching $($SectionName.ToLowerInvariant())"
        return
    }

    foreach ($entry in $matches) {
        $label = "$($entry.Name) ($($entry.DisplayName))"
        if ($PSCmdlet.ShouldProcess($label, 'Stop and delete service/driver')) {
            try {
                & sc.exe stop $entry.Name | Out-Null
            } catch {
                Write-CleanupError "Failed to stop $label" $_
            }

            try {
                & sc.exe delete $entry.Name | Out-Null
                Write-Removed $label
            } catch {
                Write-CleanupError "Failed to delete $label" $_
            }
        }
    }
}

function Remove-MatchingWmiConsumers {
    Write-Section 'WMI command consumers'

    $consumers = Get-CimInstance -Namespace root\subscription -ClassName CommandLineEventConsumer -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match $SignalPattern -or $_.CommandLineTemplate -match $SignalPattern }

    if (-not $consumers) {
        Write-Skipped 'No matching WMI command consumers'
        return
    }

    $bindings = Get-CimInstance -Namespace root\subscription -ClassName __FilterToConsumerBinding -ErrorAction SilentlyContinue

    foreach ($consumer in $consumers) {
        $consumerName = $consumer.Name
        foreach ($binding in $bindings | Where-Object { $_.Consumer -like "*Name=`"$consumerName`"*" }) {
            if ($PSCmdlet.ShouldProcess($binding.Consumer, 'Remove WMI filter-to-consumer binding')) {
                try {
                    Remove-CimInstance -InputObject $binding -ErrorAction Stop
                    Write-Removed "WMI binding for $consumerName"
                } catch {
                    Write-CleanupError "Failed to remove WMI binding for $consumerName" $_
                }
            }
        }

        if ($PSCmdlet.ShouldProcess($consumerName, 'Remove WMI command consumer')) {
            try {
                Remove-CimInstance -InputObject $consumer -ErrorAction Stop
                Write-Removed "WMI consumer $consumerName"
            } catch {
                Write-CleanupError "Failed to remove WMI consumer $consumerName" $_
            }
        }
    }
}

function Find-MatchingArtifacts {
    param(
        [Parameter(Mandatory)][string[]]$Roots,
        [string]$Pattern = $SignalPattern
    )

    foreach ($root in $Roots) {
        if (-not (Test-Path -LiteralPath $root)) {
            continue
        }

        Get-ChildItem -LiteralPath $root -Force -Recurse -ErrorAction SilentlyContinue |
            Where-Object {
                -not ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -and
                ($_.Name -match $Pattern -or $_.FullName -match $Pattern)
            } |
            Select-Object -ExpandProperty FullName
    }
}

function Remove-ShortcutArtifactsByPattern {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$Pattern
    )

    $roots = @(
        (Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu'),
        (Join-Path $env:PROGRAMDATA 'Microsoft\Windows\Start Menu'),
        (Join-Path $env:USERPROFILE 'Desktop'),
        $(if ($env:PUBLIC) { Join-Path $env:PUBLIC 'Desktop' } else { $null })
    ) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }

    $matches = @(Find-MatchingArtifacts -Roots $roots -Pattern $Pattern | Sort-Object -Unique)
    $shortcuts = @($matches | Where-Object { $_ -match '\.(lnk|url)$' })

    if (-not $shortcuts) {
        Write-Skipped "No matching $Name shortcuts"
        return
    }

    foreach ($shortcut in $shortcuts) {
        Remove-ArtifactPath $shortcut
    }
}

function Show-RemainingProductArtifacts {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$Pattern,
        [Parameter(Mandatory)][string[]]$Roots
    )

    $existingRoots = @($Roots | Where-Object { $_ -and (Test-Path -LiteralPath $_) })
    if (-not $existingRoots) {
        Write-Host "No remaining $Name cleanup roots exist."
        return
    }

    $remaining = @(Find-MatchingArtifacts -Roots $existingRoots -Pattern $Pattern | Sort-Object -Unique)
    if (-not $remaining) {
        Write-Host "No remaining $Name files found in scanned cleanup roots."
        return
    }

    $topLevelRemaining = @(Select-TopLevelPaths -Paths $remaining)
    Write-Warning "Matching $Name files still exist ($($remaining.Count) matches under $($topLevelRemaining.Count) top-level path(s)):"
    $topLevelRemaining | Select-Object -First 40 | ForEach-Object { Write-Host "  $_" }
    if ($topLevelRemaining.Count -gt 40) {
        Write-Host "  ...and $($topLevelRemaining.Count - 40) more top-level path(s)"
    }
}

function Remove-BundledProduct {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$Pattern,
        [Parameter(Mandatory)][string[]]$KnownPaths,
        [Parameter(Mandatory)][string[]]$RegistryKeys,
        [Parameter(Mandatory)][string[]]$CheckRoots,
        [string]$ProcessPattern,
        [switch]$StopProcesses
    )

    Write-Section "$Name bundled software"

    if ($StopProcesses -and $ProcessPattern) {
        Stop-ProductProcesses -Name $Name -Pattern $ProcessPattern
    }

    Invoke-RegisteredUninstallers -Name $Name -Pattern $Pattern
    Remove-StartupRegistryValuesByPattern -Name $Name -Pattern $Pattern
    Remove-ScheduledTasksByPattern -Name $Name -Pattern $Pattern
    Remove-ShortcutArtifactsByPattern -Name $Name -Pattern $Pattern

    foreach ($registryKey in $RegistryKeys) {
        Remove-RegistryKeyIfPresent -Name $Name -Path $registryKey
    }

    foreach ($path in $KnownPaths) {
        if ($path) {
            Remove-ArtifactPath $path
        }
    }

    $tempRoots = @(
        $env:TEMP,
        (Join-Path $env:WINDIR 'Temp')
    ) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }

    $tempMatches = @(Select-TopLevelPaths -Paths @(Find-MatchingArtifacts -Roots $tempRoots -Pattern $Pattern | Sort-Object -Unique))
    if ($tempMatches) {
        foreach ($tempMatch in $tempMatches) {
            Remove-ArtifactPath $tempMatch
        }
    } else {
        Write-Skipped "No matching $Name temp artifacts"
    }

    Show-RemainingProductArtifacts -Name $Name -Pattern $Pattern -Roots $CheckRoots
}

function Remove-BundledSoftware {
    if (-not $ShouldRemoveOpera -and -not $ShouldRemoveAvast) {
        Write-Section 'Bundled software'
        Write-Skipped 'Use -RemoveBundledApps, -RemoveOpera, or -RemoveAvast to remove bundled Opera/Avast artifacts'
        return
    }

    if ($ShouldRemoveOpera) {
        $operaPaths = @(
            (Join-Path $env:LOCALAPPDATA 'Programs\Opera'),
            (Join-Path $env:LOCALAPPDATA 'Programs\Opera GX'),
            (Join-Path $env:APPDATA 'Opera Software'),
            (Join-Path $env:LOCALAPPDATA 'Opera Software'),
            (Join-Path $env:ProgramFiles 'Opera'),
            $(if (${env:ProgramFiles(x86)}) { Join-Path ${env:ProgramFiles(x86)} 'Opera' } else { $null })
        )

        $operaKeys = @(
            'HKCU:\Software\Opera Software',
            'HKLM:\Software\Opera Software',
            'HKLM:\Software\WOW6432Node\Opera Software',
            'HKCU:\Software\Clients\StartMenuInternet\OperaStable',
            'HKLM:\Software\Clients\StartMenuInternet\OperaStable',
            'HKLM:\Software\WOW6432Node\Clients\StartMenuInternet\OperaStable'
        )

        $operaCheckRoots = @(
            $env:APPDATA,
            $env:LOCALAPPDATA,
            $env:ProgramFiles,
            ${env:ProgramFiles(x86)},
            $env:PROGRAMDATA
        )

        Remove-BundledProduct `
            -Name 'Opera' `
            -Pattern $OperaPattern `
            -KnownPaths $operaPaths `
            -RegistryKeys $operaKeys `
            -CheckRoots $operaCheckRoots `
            -ProcessPattern '(?i)(^opera(_autoupdate|_crashreporter)?$|Programs\\Opera|\\Opera\\)' `
            -StopProcesses
    }

    if ($ShouldRemoveAvast) {
        $avastPaths = @(
            (Join-Path $env:PROGRAMDATA 'Avast Software'),
            (Join-Path $env:LOCALAPPDATA 'AVAST Software'),
            (Join-Path $env:APPDATA 'AVAST Software'),
            (Join-Path $env:LOCALAPPDATA 'Avast Software'),
            (Join-Path $env:APPDATA 'Avast Software'),
            (Join-Path $env:ProgramFiles 'Avast Software'),
            $(if (${env:ProgramFiles(x86)}) { Join-Path ${env:ProgramFiles(x86)} 'Avast Software' } else { $null })
        )

        $avastKeys = @(
            'HKCU:\Software\AVAST Software',
            'HKLM:\Software\AVAST Software',
            'HKLM:\Software\WOW6432Node\AVAST Software',
            'HKCU:\Software\Avast Software',
            'HKLM:\Software\Avast Software',
            'HKLM:\Software\WOW6432Node\Avast Software'
        )

        $avastCheckRoots = @(
            $env:APPDATA,
            $env:LOCALAPPDATA,
            $env:ProgramFiles,
            ${env:ProgramFiles(x86)},
            $env:PROGRAMDATA
        )

        Remove-BundledProduct `
            -Name 'Avast' `
            -Pattern $AvastPattern `
            -KnownPaths $avastPaths `
            -RegistryKeys $avastKeys `
            -CheckRoots $avastCheckRoots
    }
}

function Remove-KnownFiles {
    Write-Section 'Known artifact paths'

    $paths = @(
        (Join-Path $env:APPDATA 'com.swiftsoft\ExLoader'),
        (Join-Path $env:APPDATA 'com.swiftsoft\ExLoader_Installer'),
        (Join-Path $env:LOCALAPPDATA 'com.swiftsoft\ExLoader'),
        (Join-Path $env:LOCALAPPDATA 'com.swiftsoft\ExLoader_Installer'),
        (Join-Path $env:APPDATA 'OsirisCS2'),
        (Join-Path $env:ProgramFiles 'ExLoader')
    )

    if (${env:ProgramFiles(x86)}) {
        $paths += (Join-Path ${env:ProgramFiles(x86)} 'ExLoader')
    }

    if (-not $SkipDownloads) {
        $paths += @(
            (Join-Path $env:USERPROFILE 'Downloads\ExLoader_Installer'),
            (Join-Path $env:USERPROFILE 'Downloads\ExLoader_Installer.zip')
        )
    }

    foreach ($path in $paths) {
        Remove-ArtifactPath $path
    }

    Remove-EmptyDirectory (Join-Path $env:APPDATA 'com.swiftsoft')
    Remove-EmptyDirectory (Join-Path $env:LOCALAPPDATA 'com.swiftsoft')
}

function Remove-RecentShortcuts {
    if ($SkipRecentShortcuts) {
        Write-Section 'Recent shortcuts'
        Write-Skipped 'Skipped by request'
        return
    }

    Write-Section 'Recent shortcuts'

    $recentRoot = Join-Path $env:APPDATA 'Microsoft\Windows\Recent'
    if (-not (Test-Path -LiteralPath $recentRoot)) {
        Write-Skipped "$recentRoot (not present)"
        return
    }

    $shortcuts = Get-ChildItem -LiteralPath $recentRoot -Force -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match $SignalPattern }

    if (-not $shortcuts) {
        Write-Skipped 'No matching Recent shortcuts'
        return
    }

    foreach ($shortcut in $shortcuts) {
        Remove-ArtifactPath $shortcut.FullName
    }
}

function Remove-TempArtifacts {
    if ($SkipTempScan) {
        Write-Section 'Temp artifacts'
        Write-Skipped 'Skipped by request'
        return
    }

    Write-Section 'Temp artifacts'

    $tempRoots = @(
        $env:TEMP,
        (Join-Path $env:WINDIR 'Temp')
    ) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }

    $matches = @(Select-TopLevelPaths -Paths @(Find-MatchingArtifacts -Roots $tempRoots | Sort-Object -Unique))
    if (-not $matches) {
        Write-Skipped 'No matching temp artifacts'
        return
    }

    foreach ($match in $matches) {
        Remove-ArtifactPath $match
    }
}

function Show-RemainingArtifacts {
    Write-Section 'Post-cleanup check'

    $roots = @(
        (Join-Path $env:USERPROFILE 'Downloads'),
        $env:APPDATA,
        $env:LOCALAPPDATA,
        $env:PROGRAMDATA,
        $env:ProgramFiles,
        ${env:ProgramFiles(x86)},
        $env:TEMP,
        (Join-Path $env:WINDIR 'Temp')
    ) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }

    $remaining = @(Find-MatchingArtifacts -Roots $roots | Sort-Object -Unique)
    if (-not $remaining) {
        Write-Host 'No remaining matching files found in scanned cleanup roots.'
        return
    }

    $topLevelRemaining = @(Select-TopLevelPaths -Paths $remaining)
    Write-Warning "Matching files still exist in scanned cleanup roots ($($remaining.Count) matches under $($topLevelRemaining.Count) top-level path(s)):"
    $topLevelRemaining | Select-Object -First 80 | ForEach-Object { Write-Host "  $_" }
    if ($topLevelRemaining.Count -gt 80) {
        Write-Host "  ...and $($topLevelRemaining.Count - 80) more top-level path(s)"
    }
}

function Remove-DefenderExclusions {
    Write-Section 'Defender exclusions'

    if (-not (Get-Command Get-MpPreference -ErrorAction SilentlyContinue)) {
        Write-Skipped 'Defender cmdlets not available'
        return
    }
    if (-not (Test-IsAdmin)) {
        Write-Skipped 'Administrator required to read/remove Defender exclusions'
        return
    }

    try {
        $pref = Get-MpPreference -ErrorAction Stop
    } catch {
        Write-CleanupError 'Failed to read Defender preferences' $_
        return
    }

    $found = $false
    foreach ($path in @($pref.ExclusionPath) | Where-Object { $_ -and $_ -match $SignalPattern }) {
        $found = $true
        if ($PSCmdlet.ShouldProcess($path, 'Remove Defender path exclusion')) {
            try { Remove-MpPreference -ExclusionPath $path -ErrorAction Stop; Write-Removed "Defender path exclusion: $path" }
            catch { Write-CleanupError "Failed to remove Defender path exclusion $path" $_ }
        }
    }
    foreach ($proc in @($pref.ExclusionProcess) | Where-Object { $_ -and $_ -match $SignalPattern }) {
        $found = $true
        if ($PSCmdlet.ShouldProcess($proc, 'Remove Defender process exclusion')) {
            try { Remove-MpPreference -ExclusionProcess $proc -ErrorAction Stop; Write-Removed "Defender process exclusion: $proc" }
            catch { Write-CleanupError "Failed to remove Defender process exclusion $proc" $_ }
        }
    }

    if (-not $found) {
        Write-Skipped 'No matching Defender exclusions'
    }
}

function Remove-RegistryMruValues {
    param(
        [Parameter(Mandatory)][string]$KeyPath,
        [switch]$Recurse
    )

    if (-not (Test-Path -LiteralPath $KeyPath)) {
        return
    }

    $keys = @($KeyPath)
    if ($Recurse) {
        $keys += Get-ChildItem -LiteralPath $KeyPath -Recurse -ErrorAction SilentlyContinue | ForEach-Object { $_.PSPath }
    }

    foreach ($key in $keys) {
        $props = Get-ItemProperty -LiteralPath $key -ErrorAction SilentlyContinue
        if (-not $props) { continue }
        foreach ($property in $props.PSObject.Properties) {
            if ($property.Name -match '^PS') { continue }
            $text = if ($property.Value -is [byte[]]) {
                [System.Text.Encoding]::Unicode.GetString($property.Value)
            } else {
                [string]$property.Value
            }
            if ($text -match $SignalPattern) {
                $script:MruHit = $true
                $label = "$key\$($property.Name)"
                if ($PSCmdlet.ShouldProcess($label, 'Remove MRU value')) {
                    try {
                        Remove-ItemProperty -LiteralPath $key -Name $property.Name -Force -ErrorAction Stop
                        Write-Removed $label
                    } catch {
                        Write-CleanupError "Failed to remove MRU value $label" $_
                    }
                }
            }
        }
    }
}

function Remove-ShellMruTraces {
    Write-Section 'Shell MRU history'

    $script:MruHit = $false
    $explorer = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer'

    Remove-RegistryMruValues -KeyPath "$explorer\RecentDocs" -Recurse
    Remove-RegistryMruValues -KeyPath "$explorer\RunMRU"
    Remove-RegistryMruValues -KeyPath "$explorer\TypedPaths"
    Remove-RegistryMruValues -KeyPath "$explorer\ComDlg32\OpenSavePidlMRU" -Recurse
    Remove-RegistryMruValues -KeyPath "$explorer\ComDlg32\LastVisitedPidlMRU" -Recurse
    Remove-RegistryMruValues -KeyPath "$explorer\ComDlg32\CIDSizeMRU" -Recurse

    if (-not $script:MruHit) {
        Write-Skipped 'No matching shell MRU entries'
    }
}

function Clear-ArtifactEventLogs {
    Write-Section 'Windows Event Logs'

    if (-not (Test-IsAdmin)) {
        Write-Skipped 'Administrator required to inspect/clear event logs'
        return
    }

    # Event log records are append-only: entries referencing the artifacts cannot be
    # selectively deleted, only reported. Clearing a whole log is all-or-nothing.
    foreach ($log in 'Application', 'System', 'Microsoft-Windows-Windows Defender/Operational') {
        try {
            $hits = @(Get-WinEvent -LogName $log -MaxEvents 1000 -ErrorAction SilentlyContinue |
                Where-Object { $_.Message -match $SignalPattern })
            if ($hits.Count) {
                Write-Host "  ${log}: $($hits.Count) recent record(s) reference the artifacts (append-only; not individually removable)"
            }
        } catch { }
    }

    if (-not $ClearEventLogs) {
        Write-Skipped 'Logs left intact. Use -ClearEventLogs to clear entire logs (also discards unrelated audit data).'
        return
    }

    Write-Warning 'Clearing ENTIRE Windows Event Logs. This discards all audit history, not just artifact references, and cannot be undone.'
    foreach ($log in @(& wevtutil.exe el)) {
        if ($PSCmdlet.ShouldProcess($log, 'Clear event log')) {
            try {
                & wevtutil.exe cl "$log" 2>$null
                if ($LASTEXITCODE -eq 0) { Write-Removed "Cleared event log: $log" }
            } catch {
                Write-CleanupError "Failed to clear event log $log" $_
            }
        }
    }
}

function Remove-HistoryTraces {
    if (-not $EraseHistory) {
        Write-Section 'Execution history traces'
        Write-Skipped 'Use -EraseHistory to erase Prefetch, jump lists, and console history traces'
        Clear-ArtifactEventLogs
        return
    }

    Write-Section 'Execution history traces'

    $historyPattern = $SignalPattern
    if ($ShouldRemoveOpera) { $historyPattern = "$historyPattern|(?i)opera" }
    if ($ShouldRemoveAvast) { $historyPattern = "$historyPattern|(?i)avast" }

    # Prefetch (*.pf named after the executable)
    $prefetch = Join-Path $env:WINDIR 'Prefetch'
    if (-not (Test-IsAdmin)) {
        Write-Skipped 'Administrator required to clean Prefetch'
    } elseif (Test-Path -LiteralPath $prefetch) {
        $pf = @(Get-ChildItem -LiteralPath $prefetch -Filter '*.pf' -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -match $historyPattern })
        if ($pf.Count) {
            foreach ($file in $pf) {
                # Prefetch is outside Remove-ArtifactPath's allowed roots, so delete directly.
                if ($PSCmdlet.ShouldProcess($file.FullName, 'Remove Prefetch entry')) {
                    try { Remove-Item -LiteralPath $file.FullName -Force -ErrorAction Stop; Write-Removed "Prefetch: $($file.Name)" }
                    catch { Write-CleanupError "Failed to remove Prefetch $($file.Name)" $_ }
                }
            }
        } else {
            Write-Skipped 'No matching Prefetch entries'
        }
    }

    # Jump lists whose contents reference the artifacts (removes the app's whole list)
    $jumpRoots = @(
        (Join-Path $env:APPDATA 'Microsoft\Windows\Recent\AutomaticDestinations'),
        (Join-Path $env:APPDATA 'Microsoft\Windows\Recent\CustomDestinations')
    ) | Where-Object { Test-Path -LiteralPath $_ }
    $jumpHit = $false
    foreach ($root in $jumpRoots) {
        foreach ($file in Get-ChildItem -LiteralPath $root -File -Force -ErrorAction SilentlyContinue) {
            try {
                $text = [System.Text.Encoding]::Unicode.GetString([System.IO.File]::ReadAllBytes($file.FullName))
            } catch { continue }
            if ($text -match $historyPattern) {
                $jumpHit = $true
                Remove-ArtifactPath $file.FullName
            }
        }
    }
    if (-not $jumpHit) { Write-Skipped 'No jump lists reference the artifacts' }

    # PowerShell console history (scrub matching lines, keep the rest)
    $historyPaths = @()
    try { $historyPaths += (Get-PSReadLineOption -ErrorAction SilentlyContinue).HistorySavePath } catch { }
    $historyPaths += (Join-Path $env:APPDATA 'Microsoft\Windows\PowerShell\PSReadLine\ConsoleHost_history.txt')
    $psHit = $false
    foreach ($hf in @($historyPaths | Sort-Object -Unique | Where-Object { $_ -and (Test-Path -LiteralPath $_) })) {
        $lines = @(Get-Content -LiteralPath $hf -ErrorAction SilentlyContinue)
        $kept = @($lines | Where-Object { $_ -notmatch $SignalPattern })
        if ($kept.Count -ne $lines.Count) {
            $psHit = $true
            if ($PSCmdlet.ShouldProcess($hf, 'Scrub matching PowerShell history lines')) {
                try {
                    Set-Content -LiteralPath $hf -Value $kept -Encoding UTF8 -Force -ErrorAction Stop
                    Write-Removed "Scrubbed $($lines.Count - $kept.Count) line(s) from $hf"
                } catch { Write-CleanupError "Failed to scrub $hf" $_ }
            }
        }
    }
    if (-not $psHit) { Write-Skipped 'No matching PowerShell history lines' }

    Clear-ArtifactEventLogs
}

Write-Host 'ExLoader/Osiris/Enigma artifact cleanup'
Write-Host "User: $env:USERNAME"
Write-Host "Admin: $(Test-IsAdmin)"
Write-Warning 'This removes artifacts, matching persistence, stale Defender exclusions, and targeted history traces. With -ClearEventLogs it also clears entire Windows Event Logs (unrelated audit data included).'

Write-Section 'Processes'
Stop-ProductProcesses -Name 'ExLoader' -Pattern $SignalPattern
Write-Section 'Startup registry'
Remove-StartupRegistryValuesByPattern -Name 'ExLoader' -Pattern $SignalPattern
Remove-UninstallRegistryKeys
Write-Section 'Scheduled tasks'
Remove-ScheduledTasksByPattern -Name 'ExLoader' -Pattern $SignalPattern
Remove-MatchingServiceLikeEntries -SectionName 'Services' -Entries @(Get-CimInstance Win32_Service -ErrorAction SilentlyContinue)
Remove-MatchingServiceLikeEntries -SectionName 'System drivers' -Entries @(Get-CimInstance Win32_SystemDriver -ErrorAction SilentlyContinue)
Remove-MatchingWmiConsumers
Remove-DefenderExclusions
Remove-KnownFiles
Remove-RecentShortcuts
Remove-ShellMruTraces
Remove-TempArtifacts
Remove-BundledSoftware
Remove-HistoryTraces
Show-RemainingArtifacts

if ($RunDefenderQuickScan) {
    Write-Section 'Microsoft Defender'
    if (Get-Command Start-MpScan -ErrorAction SilentlyContinue) {
        if ($PSCmdlet.ShouldProcess('Microsoft Defender', 'Start quick scan')) {
            try {
                Start-MpScan -ScanType QuickScan -ErrorAction Stop
                Write-Host 'Microsoft Defender quick scan started.'
            } catch {
                Write-CleanupError 'Failed to start Microsoft Defender quick scan' $_
            }
        }
    } else {
        Write-Skipped 'Start-MpScan is not available on this system'
    }
}

Write-Section 'Summary'
Write-Host "Removed actions: $RemovedCount"
Write-Host "Skipped actions: $SkippedCount"
Write-Host "Errors: $ErrorCount"
Write-Host 'Reboot Windows after deletion, then run the script once more with -WhatIf to verify no matching artifacts remain.'
