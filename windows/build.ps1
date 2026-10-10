$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
if (!(Test-Path 't7patch/Arxan.cpp')) { throw 'Clona el repositorio con submódulos recursivos.' }
cmake -S steam -B build-steam -A x64
if ($LASTEXITCODE -ne 0) { throw 'Falló la configuración de la integración Steam.' }
cmake --build build-steam --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw 'Falló la compilación de la integración Steam.' }
ctest --test-dir build-steam -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Fallaron las pruebas del protocolo comunitario.' }
& ./build-steam/Release/BO3SteamAttach.exe --smoke-test
if ($LASTEXITCODE -ne 0) { throw 'Falló el inicio del cargador Steam.' }
python -m unittest discover -s tests
if ($LASTEXITCODE -ne 0) { throw 'Fallaron las pruebas.' }
python -m pip install -r requirements-build.txt
if ($LASTEXITCODE -ne 0) { throw 'Falló la instalación de herramientas.' }
New-Item -ItemType Directory -Force -Path app/steam | Out-Null
Copy-Item build-steam/Release/BO3SteamFix.dll app/steam/ -Force
Copy-Item build-steam/Release/BO3SteamAttach.exe app/steam/ -Force
python -m PyInstaller --noconfirm --clean --windowed --onedir --name BO3CommunityClient --add-data 'app/steam;steam' app/client.py
if ($LASTEXITCODE -ne 0) { throw 'Falló el empaquetado de la app.' }
$smoke = Start-Process -FilePath 'dist/BO3CommunityClient/BO3CommunityClient.exe' -ArgumentList '--smoke-test' -PassThru
if (!$smoke.WaitForExit(20000)) { $smoke.Kill(); throw 'La app empaquetada no cerró en la prueba de inicio.' }
if ($smoke.ExitCode -ne 0) { throw 'Falló el inicio de la app empaquetada.' }
Copy-Item LICENSE dist/BO3CommunityClient/LICENSE-app.txt
Copy-Item engine/LICENSE dist/BO3CommunityClient/LICENSE-engine.txt
Copy-Item README.md dist/BO3CommunityClient/README.txt
Copy-Item installer/* dist/BO3CommunityClient/ -Force
Compress-Archive -Path dist/BO3CommunityClient -DestinationPath dist/BO3CommunityClient-Windows-beta3.zip -Force
