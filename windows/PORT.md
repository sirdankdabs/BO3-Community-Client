# Estado del port Windows

La app está implementada y usa el motor nativo Ezz v3.0.0 mediante un componente de comunicación propio. El motor y dependencias se fijan por commit en un submódulo, manteniendo su fuente y GPL-3.0. Las correcciones exclusivamente Mac no corresponden a Windows.

Implementado: navegador UDP, filtros, ordenamiento, nombre, solicitud de unión a servidor, detección de Steam y bibliotecas, respaldo, importación de identidad, Unlock All con respaldo, empaquetado EXE y compilación Windows en CI.

Pendiente de prueba en el juego: conexión, mapas, importación real de identidad, guardado y compatibilidad con la instalación del probador. La beta1 solicita rango 997 con las tablas nativas. El módulo de XP de armas usa las tablas de niveles y la API DDL nativa Windows. También solicita guardado de GobbleGums locales; ambas funciones requieren verificación dentro del juego. No confundir el inventario local de Ezz con el inventario de Steam.

Ver windows/README.md y el estado de GitHub Actions antes de descargar. No anunciar paridad completa solo porque compile.
