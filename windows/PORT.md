# Estado Windows

Beta3 cambia la integración: Steam inicia el BlackOps3.exe original; una DLL se carga en ese proceso. No se empaqueta ni arranca boiii.exe. Se conserva el código de beta1 en bridge/ y el motor upstream como referencia de protocolo y dependencias de compilación.

Implementado experimentalmente: carga en el proceso Steam seleccionado, fingerprint del ejecutable, app/navegador, identidad P-521, validación getInfo con nonce, connect firmado y fragmentado, recepción y envío UDP solo al servidor activo, delegación a los originales sin sesión comunitaria y restauración de cambios temporales al salir. T7Patch aporta integridad y protecciones.

No verificado en juego: lanzamiento real, handshake completo, mapas originales, salida y vuelta a partidas públicas. Workshop instalado se carga mediante UGC nativo (suscripción/descarga en Steam); Unlock All usa consultas BG e inventario condicionadas por una preferencia. Ambas funciones requieren pruebas reales y Unlock All no cambia permanentemente el perfil ni las compras Steam. No anunciar paridad completa ni conexión confirmada a partir de CI. El amigo del usuario probará la beta.
