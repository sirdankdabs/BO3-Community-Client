$ErrorActionPreference = 'Stop'
try {
    $source = $PSScriptRoot
    $target = Join-Path $env:LOCALAPPDATA 'Programs/BO3 Community Client'
    if (!(Test-Path (Join-Path $source 'BO3CommunityClient.exe'))) { throw 'Extrae el ZIP completo antes de instalar.' }
    if (Get-Process -Name BO3CommunityClient -ErrorAction SilentlyContinue) { throw 'Cierra la app BO3 Community Client antes de actualizarla.' }
    New-Item -ItemType Directory -Force -Path $target | Out-Null
    if ([IO.Path]::GetFullPath($source) -ne [IO.Path]::GetFullPath($target)) {
        Copy-Item (Join-Path $source '*') $target -Recurse -Force
    }
    $shell = New-Object -ComObject WScript.Shell
    $shortcut = $shell.CreateShortcut((Join-Path ([Environment]::GetFolderPath('Desktop')) 'BO3 Community Client.lnk'))
    $shortcut.TargetPath = Join-Path $target 'BO3CommunityClient.exe'
    $shortcut.WorkingDirectory = $target
    $shortcut.Save()
    Write-Host 'Instalado. Tu juego, perfil e identidad se conservan.'
    Start-Process (Join-Path $target 'BO3CommunityClient.exe')
} catch { Write-Error $_; exit 1 }
