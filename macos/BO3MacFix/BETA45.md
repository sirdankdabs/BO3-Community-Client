# Beta45: predicado de menú corregido

El log real de beta44 mostraba mode=0/1, map vacío, connection=11 y local=1, por lo que nunca enviaba comandos de progreso.
El callback Lua Engine.IsRunningUILevel en 0xe657a6 llama directamente a 0xe71737; ese predicado había sido etiquetado Com_IsLocalServerRunning en el mapa anterior. Se añade el alias correcto y se usa como señal positiva de menú, sin depender de la cadena de mapa vacía.

Se conservan guardas contra sesiones comunitarias y conexión de juego sin UI. Pruebas con el estado real observado del menú y con estados de partida/red aprobadas, junto con la suite completa.
Dylib instalada directamente con respaldo. Pendiente reiniciar el juego y verificar comandos encolados, ejecución y guardado del nivel.
