# Kirby: Nightmare in Dream Land Recomp - Controller Remapping Utility
# Native zero-latency remapping for Xbox, Nintendo Switch, PlayStation & PC gamepads.
param([string]$Preset = "")

$ErrorActionPreference = "Stop"
$iniPath = Join-Path $PSScriptRoot "keybinds.ini"

if (-not (Test-Path $iniPath)) {
    Write-Host "Error: keybinds.ini not found in $PSScriptRoot" -ForegroundColor Red
    Pause
    Exit 1
}

function Get-CurrentLayout {
    if (-not (Test-Path $iniPath)) { return "Unknown" }
    $layout = "xbox"
    $trigR = "true"
    Get-Content $iniPath | ForEach-Object {
        if ($_ -match '^\s*layout\s*=\s*(.+)$') { $layout = $matches[1].Trim() }
        if ($_ -match '^\s*trigger_r\s*=\s*(.+)$') { $trigR = $matches[1].Trim() }
    }
    if ($trigR -eq "dash" -or $trigR -eq "b") { return "Trigger Action (RT = Inhale/Attack)" }
    if ($layout -eq "nintendo" -or $layout -eq "switch") { return "Nintendo / Switch (B=Jump, A/Y=Inhale)" }
    if ($layout -eq "classic") { return "Classic 1:1 Hardware" }
    return "Xbox / PC Standard (A=Jump, B/X=Inhale, Y=Jump)"
}

function Save-GamepadConfig($layout, $a, $b, $x, $y, $lb, $rb, $trigL, $trigR) {
    $lines = Get-Content $iniPath
    $out = [System.Collections.Generic.List[string]]::new()
    $inGamepad = $false
    $written = $false

    foreach ($line in $lines) {
        if ($line -match '^\s*\[gamepad1\]') {
            $inGamepad = $true
            $written = $true
            $out.Add("[gamepad1]")
            $out.Add("layout     = $layout")
            $out.Add("a          = $a")
            $out.Add("b          = $b")
            $out.Add("x          = $x")
            $out.Add("y          = $y")
            $out.Add("lb         = $lb")
            $out.Add("rb         = $rb")
            $out.Add("select     = back")
            $out.Add("start      = start")
            $out.Add("trigger_l  = $trigL")
            $out.Add("trigger_r  = $trigR")
            $out.Add("deadzone   = 10000")
            $out.Add("analog     = true")
            continue
        }
        if ($inGamepad) {
            if ($line -match '^\s*\[' -or $line -match '^#') {
                $inGamepad = $false
                $out.Add($line)
            }
            continue
        }
        $out.Add($line)
    }

    if (-not $written) {
        $out.Add("[gamepad1]")
        $out.Add("layout     = $layout")
        $out.Add("a          = $a")
        $out.Add("b          = $b")
        $out.Add("x          = $x")
        $out.Add("y          = $y")
        $out.Add("lb         = $lb")
        $out.Add("rb         = $rb")
        $out.Add("select     = back")
        $out.Add("start      = start")
        $out.Add("trigger_l  = $trigL")
        $out.Add("trigger_r  = $trigR")
        $out.Add("deadzone   = 10000")
        $out.Add("analog     = true")
    }

    Set-Content -Path $iniPath -Value $out -Encoding UTF8
    Write-Host "`n[OK] Controller layout saved to keybinds.ini!" -ForegroundColor Green
    Write-Host "[OK] Changes take effect IMMEDIATELY (even if game is running).`n" -ForegroundColor Cyan
}

if ($Preset -ne "") {
    switch ($Preset.ToLower()) {
        "xbox"     { Save-GamepadConfig "xbox" "a" "b" "b" "a" "l" "r" "true" "true" }
        "nintendo" { Save-GamepadConfig "nintendo" "b" "a" "a" "b" "l" "r" "true" "true" }
        "action"   { Save-GamepadConfig "xbox" "a" "b" "b" "a" "l" "r" "true" "b" }
        "classic"  { Save-GamepadConfig "classic" "a" "b" "x" "y" "l" "r" "true" "true" }
    }
    Exit 0
}

while ($true) {
    Clear-Host
    $curr = Get-CurrentLayout
    Write-Host "==================================================================" -ForegroundColor Cyan
    Write-Host "     KIRBY: NIGHTMARE IN DREAM LAND - CONTROLLER REMAPPER        " -ForegroundColor Yellow
    Write-Host "==================================================================" -ForegroundColor Cyan
    Write-Host " Current Active Layout : $curr" -ForegroundColor Green
    Write-Host "------------------------------------------------------------------" -ForegroundColor DarkGray
    Write-Host " Select a preset below (or press Q to quit):`n"
    Write-Host " [1] Xbox / PC Standard (Recommended)" -ForegroundColor White
    Write-Host "     - A: Jump / Float"
    Write-Host "     - B: Inhale / Attack"
    Write-Host "     - X: Inhale / Attack (Comfort Ergonomic Fallback)"
    Write-Host "     - Y: Jump / Float"
    Write-Host "     - LT: Drop Power (L)  |  RT: Drop Power (R)`n"

    Write-Host " [2] Nintendo Switch / SNES Layout" -ForegroundColor White
    Write-Host "     - B: Jump / Float"
    Write-Host "     - A: Inhale / Attack"
    Write-Host "     - Y: Inhale / Attack"
    Write-Host "     - X: Jump / Float`n"

    Write-Host " [3] Modern Trigger Action Layout" -ForegroundColor White
    Write-Host "     - A: Jump / Float"
    Write-Host "     - B: Inhale / Attack"
    Write-Host "     - RT: Inhale / Attack (Trigger Attack!)`n"

    Write-Host " [4] Classic 1:1 Hardware Match" -ForegroundColor White
    Write-Host "     - A -> A, B -> B, X -> X, Y -> Y (Literal GBA hardware mapping)`n"

    Write-Host " [Q] Quit`n" -ForegroundColor Gray
    Write-Host "==================================================================" -ForegroundColor Cyan

    $choice = Read-Host " Enter choice (1-4, Q)"
    switch ($choice.Trim()) {
        "1" {
            Save-GamepadConfig "xbox" "a" "b" "b" "a" "l" "r" "true" "true"
            Start-Sleep -Seconds 2
        }
        "2" {
            Save-GamepadConfig "nintendo" "b" "a" "a" "b" "l" "r" "true" "true"
            Start-Sleep -Seconds 2
        }
        "3" {
            Save-GamepadConfig "xbox" "a" "b" "b" "a" "l" "r" "true" "b"
            Start-Sleep -Seconds 2
        }
        "4" {
            Save-GamepadConfig "classic" "a" "b" "x" "y" "l" "r" "true" "true"
            Start-Sleep -Seconds 2
        }
        "q" { Exit 0 }
        "Q" { Exit 0 }
    }
}
