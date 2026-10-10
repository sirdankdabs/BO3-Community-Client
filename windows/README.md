# BO3 Community Client para Windows — beta1

Port de la app Mac: lista de servidores, búsqueda, filtros Zombies/MP/vacíos/llenos/mods, ordenamiento, nombre, conexión, respaldo de perfil e importación de identidad. Interfaz nativa Tk empaquetada como EXE, sin Python instalado en el equipo del jugador.

El motor nativo es Ezz BOIII v3.0.0, fijado al commit 5aa7fac39c51438d9c1e312b99534b7fef3bff44, con un componente propio de comunicación entre app y juego. Esa versión soporta el ejecutable Steam del 10 de septiembre de 2026. Las correcciones que solo aplican a Mac (cURL, audio de Mac, conversión de Havok) se conservan en el código Mac y no se aplican a Windows.

## Uso

1. Descargar el ZIP completo de la compilación Windows y extraerlo. Mantener las carpetas internas. Ejecutar Install.cmd para instalar por usuario y crear el acceso directo; también se puede abrir la app directamente.
2. Abrir BO3CommunityClient.exe, seleccionar la carpeta de BO3 de Steam y actualizar la lista.
3. Seleccionar servidor, nombre y Unirse. Entrar al menú Zombies o Multijugador correspondiente para conectar.
4. Unlock All respalda el perfil y ejecuta la opción del motor Ezz. El bridge solicita rango 997 con XP tomada de las tablas nativas, como en Mac; falta comprobarlo dentro del juego. Esta beta no traslada todavía la prueba de experiencia de armas beta47. Los desbloqueos por hooks requieren el motor activo; los GobbleGums locales de Ezz y su perfil no equivalen al inventario de Steam.

Los datos de la app están en %LOCALAPPDATA%/BO3 Community Client. Ezz conserva su identidad y progreso en %LOCALAPPDATA%/boiii. El botón de respaldo incluye ambos perfiles.

## Compilar

Windows 10/11 x64, Visual Studio Build Tools con Desktop C++, ClangCL, Premake5 y Python 3.12.

```
git clone --recurse-submodules <URL-del-repositorio>
cd BO3-Community-Client/windows
powershell -File build.ps1
```

Salida: dist/BO3CommunityClient-Windows-beta1.zip. Las compilaciones y pruebas en Windows se ejecutan en GitHub Actions. La conexión, mapas, identidad y guardado real necesitan verificación dentro del juego por el probador; compilar no confirma paridad total.

Motor Ezz: GPL-3.0 (LICENSE en el submódulo). El componente bridge y launcher de este directorio se distribuyen bajo GPL-3.0. El código Mac conserva sus licencias originales. No se distribuyen archivos del juego, cuentas ni claves personales.
