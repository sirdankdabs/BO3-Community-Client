# BO3MacFix 1.2-beta42-t7patch3.09-unlock1 — inventario experimental

## Alcance real
Porta cuatro consultas de `SPOOF_UNLOCK` de Scroptss/T7Patch-src a Mac v98:
- Cantidad de inventario: 1 para artículos y 255 para GobbleGums (configurable de 1 a 255).
- Slots adicionales: disponibles.
- Inventario: se presenta como válido.
- Incentivos: activos salvo 29, 30 y 34, igual que upstream con SPOOF_SKIP_CWL=false.

**No es un unlock all permanente.** No modifica rango, prestigio, desafíos ni archivos de estadísticas. No incluye `UserHasLicenseForApp` ni `SPOOF_OWNERSHIP`; no replica el desbloqueo de licencias de mapas/DLC. Tampoco descarga assets ausentes. Los cambios del inventario son respuestas simuladas mientras el parche está cargado; el servidor puede mantener sus propias restricciones.

## Activación
Cierra el juego. En `BO3MacFix/bo3macfix.conf`, junto a la dylib que carga el juego, añade:

```
BO3MACFIX_SPOOF_UNLOCK=1
BO3MACFIX_SPOOF_GUM_COUNT=255
```

Reinicia el juego. Para desactivar, elimina esas líneas y reinicia. Por defecto está desactivado. `BO3MACFIX_DISABLE_OPTIONAL_HOOKS=1` también impide instalarlo.

## Instalación del paquete experimental
Este paquete se mantiene separado del instalador original de la copia de seguridad.
Con el juego cerrado, respalda la carpeta `BO3MacFix` que está junto a `CoDBlkOps3.app`. Sustitúyela por esta carpeta y conserva tus opciones del archivo de configuración. Si todavía no tienes BO3MacFix instalado, copia esta carpeta junto al .app y ejecuta `Install_BO3MacFix` (opción Install). Si ya lo tienes instalado, el lanzador existente puede cargar la nueva dylib sin reinstalar el lanzador.

Comprueba `BO3MacFix/t7-status.log` (o `engine.log` si usas BO3 Mac Client). La activación correcta registra:
`Inventory spoof: 4 hooks active, GobbleGums=255 (profile unchanged).`
Si aparece `disabled`, `missing`, `unsupported` o `failed`, no se activó.

No se ha ejecutado el juego ni validado la respuesta de los menús/servidores en esta sesión. Este paquete es para pruebas y no se ha instalado en Steam.

## Validación
- Compilación x86_64 de BO3MacFix y del motor de BO3 Mac Client.
- Suite completa `make test` de BO3MacFix: aprobada, incluyendo ocho escenarios nuevos para inventario.
- Prueba Dobby con el patrón de prólogo de GetItemQuantity: aprobada (call relativo y salto condicional corto).
- Se verifica el SHA-1 del ejecutable y los primeros 16 bytes de cada función antes de instalar los hooks. Se resuelven las cuatro direcciones antes de modificar código; si falla la instalación, las consultas conservan el comportamiento original.

## Petición adicional: progreso permanente con respaldo
Pendiente, **no implementado**. SPOOF_UNLOCK no contiene esa función. El ejecutable de Mac tiene comandos de rango (`setRank`, `setParagonRank`, `PrestigeStatsMaster`), pero no se ha verificado su comportamiento de guardado ni el esquema completo de desafíos. No se ejecutaron esos comandos ni se modificó el perfil. Un respaldo de `players` por sí solo no garantiza restaurar estadísticas almacenadas en Steam o en un servidor comunitario.

## Referencia y direcciones verificadas
Fuente: https://github.com/Scroptss/T7Patch-src/blob/main/Hooks.cpp
Mac SHA-1: `5e42371b6a16f2b11e637f48e38554625570aa9f`.

| Función | RVA | Evidencia |
| --- | --- | --- |
| LiveInventory_GetItemQuantity | 0x14d3b27 | GetInventoryItemQuantity de Lua; devuelve int32 desde item+4 |
| LiveInventory_AreExtraSlotsPurchased | 0x14d3af3 | AreExtraSlotsPurchased de Lua; consulta extra_slots |
| LiveInventory_IsValid | 0x14d291f | Comprueba inventario=4 y balance=3; IsLootReady llega por 0x122cb29 |
| LiveEntitlements_IsEntitlementActiveForController | 0x149e102 | controller + ID; tabla de incentives.csv en 0x102d6d68 |

## Identificar la instalación
La versión compilada es **1.2-beta42-t7patch3.09-unlock1**. Después de iniciar desde Steam, el registro `BO3MacFix/t7-status.log` debe mostrar esa versión en el mensaje de inicio. Busca además `Inventory spoof: 4 hooks active` para confirmar la activación del inventario.
