$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
if (!(Test-Path 'engine/src/client/main.cpp')) { throw 'Clona el repositorio con git clone --recurse-submodules.' }
Copy-Item bridge/community_client.cpp engine/src/client/component/community_client.cpp -Force
Push-Location engine
premake5 vs2022
if ($LASTEXITCODE -ne 0) { throw 'Premake falló.' }
msbuild /m /v:minimal /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=ClangCL build/boiii.sln
if ($LASTEXITCODE -ne 0) { throw 'Falló la compilación del motor.' }
Pop-Location
python -m unittest discover -s tests
if ($LASTEXITCODE -ne 0) { throw 'Fallaron las pruebas.' }
python -m pip install -r requirements-build.txt
if ($LASTEXITCODE -ne 0) { throw 'Falló la instalación de herramientas.' }
New-Item -ItemType Directory -Force -Path app/engine | Out-Null
Copy-Item engine/build/bin/x64/Release/boiii.exe app/engine/boiii.exe -Force
if (Test-Path app/engine/data) { Remove-Item app/engine/data -Recurse -Force }
Copy-Item engine/data app/engine/data -Recurse -Force
python -m PyInstaller --noconfirm --clean --windowed --onedir --name BO3CommunityClient --add-data 'app/engine;engine' app/client.py
if ($LASTEXITCODE -ne 0) { throw 'Falló el empaquetado de la app.' }
Copy-Item engine/LICENSE dist/BO3CommunityClient/LICENSE-engine.txt
Copy-Item README.md dist/BO3CommunityClient/README.txt
Copy-Item installer/* dist/BO3CommunityClient/ -Force
Compress-Archive -Path dist/BO3CommunityClient -DestinationPath dist/BO3CommunityClient-Windows-beta1.zip -Force
