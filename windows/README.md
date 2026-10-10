# BO3 Community Client para Windows — Steam beta3

Esta versión abre **BlackOps3.exe original mediante Steam** y carga BO3SteamFix.dll dentro de ese proceso. No incluye ni ejecuta boiii.exe. La app puede adjuntarse al juego que ya abriste desde Steam. El transporte comunitario usa una identidad P-521 y un socket propio solo mientras conecta al servidor elegido. Al desconectar restaura las instrucciones temporales; el perfil y la identidad Steam permanecen intactos.

## Instalar y probar

1. Extrae el ZIP completo y ejecuta Install.cmd. Mantén las carpetas internas.
2. Selecciona la carpeta que contiene BlackOps3.exe. Se admite únicamente el ejecutable Steam de septiembre de 2026, identificado por timestamp y checksum.
3. Abre BO3 desde Steam normalmente. En la app selecciona un servidor y pulsa Unirse.
4. La app carga la DLL; entra al menú Zombies o Multijugador que corresponda. La DLL valida el servidor y solicita la conexión nativa.
5. Al salir del servidor, comprueba una partida pública desde el mismo juego. Cierra y reinicia BO3 si la prueba falla; no se reemplaza el ejecutable ni se modifica el archivo del juego.

También puedes pulsar Unirse con el juego cerrado: la app pide a Steam que lo abra y espera para cargar la DLL. Se necesitan Steam y BO3 legítimamente instalados. Instala app y juego con el mismo usuario de Windows.

**Estado experimental:** las pruebas de protocolo y compilación no demuestran que la conexión completa o las partidas públicas funcionen dentro de BO3. Eso debe verificarlo el probador. La transición Steam ↔ comunidad, mods/Workshop, transporte y autenticación necesitan pruebas reales. Esta beta habilita carga de Workshop instalado y Unlock All temporal. No modifica permanentemente el perfil Steam. Beta1 utilizaba otro motor; no prueba esta integración.

La DLL usa el soporte de integridad y las protecciones de T7Patch actualizado. Las direcciones y estructuras de la conexión se toman del commit fijado de Ezz; se adaptan a hooks que delegan al juego original cuando no hay una sesión comunitaria. No se carga la emulación Steam de Ezz, sus servicios ni su motor.

Datos y diagnóstico: %LOCALAPPDATA%/BO3 Community Client (steam-engine.log, status.json, identity.key). La clave es privada; no la publiques. El botón de respaldo copia players y la identidad local antes de importar otra clave.

## Compilar

Windows x64, Visual Studio Build Tools con C++, CMake y Python 3.12. Clona con submódulos recursivos y ejecuta windows/build.ps1. Salida: dist/BO3CommunityClient-Windows-beta3.zip. GitHub Actions compila la DLL, el cargador y la app; prueba serialización, fragmentación, encuadre de paquetes y arranque empaquetado.

Fuentes externas fijadas: Scroptss/T7Patch-src y Ezz-lol/boiii-free. Conservan sus créditos y condiciones originales. El código propio está bajo GPL-3.0. No se distribuyen archivos del juego, cuentas ni claves personales.

## Workshop y Unlock All

Selecciona un servidor y pulsa **Workshop del servidor**. Se abren sus páginas en Steam: suscríbete al mapa y al mod indicados y espera a que Steam termine de descargarlos. Pulsa **Unirse**: la DLL actualiza el catálogo nativo, comprueba los IDs y carga el mod o el modo usermaps antes de conectar. La descarga corresponde a Steam; la app no usa SteamCMD ni otro ejecutable del juego. Si falta contenido, detiene la conexión con un mensaje.

Pulsa **Activar Unlock All** y entra al menú de BO3. La app respalda players y la identidad antes de pedir el desbloqueo. Se habilitan armas, accesorios, ranuras de accesorios, camuflajes/retículas, consultas de desafíos, atuendos de especialistas, inventario y ranuras adicionales. La preferencia se guarda y se restaura al cargar la DLL en otra sesión. **Desactivar Unlock All** devuelve las consultas a sus funciones originales.

Es un desbloqueo local mientras la DLL está cargada: no cambia el nivel ni convierte desafíos en progreso completado, no concede compras ni DLC de Steam, y los GobbleGums mostrados no son cantidades permanentes del servidor de Steam. Abrir BO3 sin cargar la DLL muestra el inventario original. Cierra el juego antes de cambiar de beta para cargar la DLL nueva. Todo requiere validación en el juego por el probador.
