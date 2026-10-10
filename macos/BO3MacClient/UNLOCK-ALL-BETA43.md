# BO3MacFix 1.2-beta43-t7patch3.09-unlock2 — Unlock All MP/Zombies

Adaptación experimental de las consultas de loot y los comandos de progreso de BOIII/Ezz para el ejecutable Mac v98.
Referencia: https://github.com/Ezz-lol/boiii-free/blob/main/src/client/component/loot.cpp
Tablas de rango: https://github.com/Ezz-lol/boiii-free/blob/main/data/ui_scripts/stats/__init__.lua

## Instalación y uso
1. Cierra BO3.
2. Haz doble clic en `Instalar Unlock All beta43.command` dentro de este paquete (o en `Instalar BO3MacFix Unlock All beta43.command` en la raíz de la copia de seguridad).
3. El instalador verifica los archivos y la versión de BO3, respalda el parche, `players` y las copias locales de Steam de la aplicación 311210. Conserva las demás opciones de configuración. No modifica las estadísticas durante la instalación.
4. Abre BO3 desde Steam. Entra en Multiplayer y permanece en su menú durante al menos 10 segundos. Haz lo mismo en Zombies. No entres en partida hasta que termine ese tiempo.
5. Revisa nivel/prestigio y los accesorios/camos. Cierra el juego normalmente, vuelve a abrirlo y comprueba que el progreso se conserva.

El parche usa los comandos internos de estadísticas y `uploadStats` del juego; **no se ha confirmado en una sesión real que los acepte y guarde**. Los logs distinguen activación de hooks y comandos encolados, sin afirmar que el guardado esté confirmado.

## Qué incluye
- Solicita prestigio maestro (plevel 11) y nivel 1000 en MP y Zombies, una vez por modo. Usa las tablas de XP del propio juego, no cantidades tomadas del ejecutable Windows. MP: paragon_rank 944; Zombies: paragon_rank 964 (la interfaz suma 56/36 respectivamente).
- Armas y artículos adquiribles con tokens: consulta de compra disponible.
- Accesorios de armas y slots de accesorios: consulta de bloqueo desactivada.
- Camuflajes y retículas: consulta de bloqueo de opciones desactivada.
- Consulta de desafíos: responde completado, para las recompensas que dependen de ella. No escribe el historial real de todos los desafíos.
- Atuendos de especialistas: consulta de bloqueo desactivada.
- 10 clases personalizadas y slots adicionales.
- Loot/Black Market y GobbleGums: cantidades simuladas.
- Incentivos de inventario de beta42, excepto IDs inválidos/duplicados 29, 30 y 34.

Los accesorios y cosméticos funcionan mediante consultas modificadas mientras el parche esté activo, igual que esa parte de BOIII. El cambio de progreso se realiza por comandos del perfil. No incluye archivos de DLC/mapas ausentes, licencias de Steam, desbloqueo de propiedad, progreso de servidores administrado por terceros ni progreso de campaña. La consulta de opciones cosméticas se modifica en los tres modos porque el menú Mac comprueba los tres para decidir el bloqueo. No se afirma que todas las restricciones de todos los menús o servidores estén cubiertas.

## Verificación
En `BO3MacFix/t7-status.log` deberían aparecer:
- `beta43-t7patch3.09-unlock2` en la versión.
- `Full unlock: 8 hooks active`.
- `Progress unlock: armed`.
- `Progress unlock MP: commands queued` y su equivalente ZM tras entrar en cada menú.

`commands queued` significa que el juego recibió comandos para procesar, **no** que el guardado ya esté confirmado.
Si no se encolan: permanece en `core_frontend`, evita conexión automática a servidores, revisa que los hooks opcionales estén habilitados y que el respaldo siga existiendo. Si la tabla de rango no ofrece la fila de nivel 1000, se omite el cambio de progreso.

## Opciones
El instalador establece en `BO3MacFix/bo3macfix.conf`:
```
BO3MACFIX_SPOOF_UNLOCK=1
BO3MACFIX_SPOOF_GUM_COUNT=255
BO3MACFIX_UNLOCK_ALL=1
BO3MACFIX_UNLOCK_PROGRESS=1
BO3MACFIX_UNLOCK_BACKUP=/ruta/al/respaldo
```
Las variables del entorno tienen prioridad. `BO3MACFIX_DISABLE_OPTIONAL_HOOKS=1` impide la activación.
Los archivos `unlock-mp.queued` y `unlock-zm.queued` evitan repetir automáticamente los cambios después de reiniciar. Para reintentarlos, con el juego cerrado elimina solo el marcador del modo correspondiente. Se usan los mismos nombres en el directorio de datos configurado si existe `BO3MACFIX_DATA_DIR`.

Para desactivar las consultas, establece `BO3MACFIX_UNLOCK_ALL=0` y `BO3MACFIX_SPOOF_UNLOCK=0`, y reinicia. Eso no revierte las estadísticas que ya se hayan guardado. Para cancelar cambios de progreso pendientes, usa `BO3MACFIX_UNLOCK_PROGRESS=0`.

## Respaldo
El instalador muestra su ruta y crea `backup.ready` solo después de copiar `players` y las copias locales de Steam disponibles. El motor exige ese marcador antes de modificar progreso. No copia claves privadas a este paquete ni al archivo ZIP.
Para recuperar archivos locales, cierra el juego y respalda primero el estado actual; después restaura `players` y el parche desde la carpeta de respaldo. Un respaldo local no garantiza deshacer estadísticas ya sincronizadas a un servicio remoto o a un servidor comunitario.

## Validación y límites
- Compilados los motores x86_64 de BO3MacFix y BO3 Mac Client.
- Suite completa de BO3MacFix aprobada, incluyendo protección de modo/campaña, prohibición en conexión comunitaria, espera en menú, comandos de ambos modos, XP obtenida de tablas y prevención de duplicados.
- Dirección y primeros 16 bytes de cada función verificados estáticamente para SHA-1 `5e42371b6a16f2b11e637f48e38554625570aa9f`. Si faltan equivalencias se omite la función; las consultas solo se activan al instalar todas las del grupo.
- Instalador verificado con archivos de prueba, incluyendo conservación de opciones y creación del respaldo; no ejecutado contra Steam real.
- Pendiente: prueba dentro del juego, comprobación de que Dobby engancha estas ocho funciones reales y confirmación del guardado después de reiniciar.

Esta instalación actualiza el parche que carga **Steam**. BO3 Mac Client contiene otro motor en su app; su fuente también está adaptada y compilada, pero el .command no reemplaza esa aplicación.
