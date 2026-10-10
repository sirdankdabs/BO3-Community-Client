#!/bin/bash
set -euo pipefail
/usr/bin/python3 - <<'PYTHON'
from pathlib import Path
import shutil,time
p=Path.home()/'Library/Application Support/Steam/steamapps/common/Call of Duty Black Ops III/BO3MacFix/bo3macfix.conf'
shutil.copy2(p,p.with_name('bo3macfix.conf.antes-comprobar-'+str(int(time.time()))))
keys={'BO3MACFIX_SPOOF_UNLOCK','BO3MACFIX_UNLOCK_ALL','BO3MACFIX_UNLOCK_PROGRESS','BO3MACFIX_PERMANENT_WEAPONS'}
lines=[line for line in p.read_text().splitlines() if line.split('=',1)[0] not in keys]
lines += [key+'=0' for key in sorted(keys)]
p.write_text('\n'.join(lines)+'\n')
print('Desbloqueo del parche desactivado. Reinicia y revisa accesorios. Para reactivarlo ejecuta Instalar BO3MacFix beta47.command.')
PYTHON
