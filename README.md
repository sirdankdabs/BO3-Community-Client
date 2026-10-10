# BO3 Community Client

Código fuente completo de BO3MacFix y BO3 Mac Client, con modificaciones locales hasta beta47. Este repositorio no incluye Call of Duty: Black Ops III, perfiles personales ni claves de identidad.

## Estado real

- **macOS:** código del parche y aplicación SwiftUI, compilados localmente. Beta47 añade una prueba de experiencia de armas permanente. La persistencia de esos accesorios todavía requiere verificación dentro del juego; los camuflajes por desafíos y GobbleGums siguen dependiendo del desbloqueo del parche.
- **Windows / Steam:** beta2 experimental carga una DLL en el BlackOps3.exe original iniciado por Steam. No arranca boiii.exe. La conexión comunitaria y el regreso a partidas públicas requieren prueba real; Workshop y Unlock All no están habilitados en este motor. La beta1 anterior usaba Ezz y no cumple esa integración.

## Compilar en Mac

Requiere las herramientas de línea de comandos de Xcode.

```sh
cd macos/BO3MacFix
make
make test
```

Para compilar la app con el motor actualizado:

```sh
cd macos/BO3MacClient
./build.sh
```

La app aparece en `macos/BO3MacClient/dist/BO3 Mac Client.app`. Consulta los README de cada componente y `macos/BETA47.md` para las limitaciones experimentales.

## Código y licencias

Se conserva el código de instalación, launcher, motor, tests y documentación de ambos proyectos. Los archivos `lib/` contienen dependencias estáticas necesarias para la compilación de Mac. Las licencias originales y créditos están en cada componente; no se cambia su autoría.

BO3MacFix deriva del proyecto de InvoxiPlayGames y serious, bajo LGPL-2.1. Dobby y LibTomCrypt/LibTomMath conservan sus propias licencias. Las referencias de la adaptación son [T7Patch-src](https://github.com/Scroptss/T7Patch-src) y [BOIII/Ezz](https://github.com/Ezz-lol/boiii-free). No se atribuye a este repositorio la autoría de esos proyectos.

## Windows

Consulta [instalación y compilación Windows](windows/README.md). Ezz v3.0.0 admite el ejecutable Steam del 10 de septiembre de 2026. Las correcciones específicas de macOS no se aplican a Windows.
