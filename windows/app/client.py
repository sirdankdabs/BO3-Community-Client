"""Windows counterpart of the SwiftUI browser; engine IPC is versioned separately."""
import concurrent.futures, ipaddress, json, os, re, shutil, socket, struct
import subprocess, sys, threading, time, uuid
from pathlib import Path
OOB=b'\xff'*4
DATA=Path(os.getenv('LOCALAPPDATA',str(Path.home()))) / 'BO3 Community Client'

def endpoint(value):
    host,port=value.rsplit(':',1)
    ipaddress.IPv4Address(host)
    if not 1<=int(port)<=65535:raise ValueError('Puerto inválido')
    return host,int(port)

def parse_master(data):
    header=OOB+b'getserversResponse'
    if not data.startswith(header):return []
    out=[];i=len(header)
    while i<len(data):
        if data[i:i+4]==b'\\EOT':break
        if data[i:i+1]!=b'\\':i+=1;continue
        if i+7>len(data):break
        host=socket.inet_ntoa(data[i+1:i+5]);port=struct.unpack('!H',data[i+5:i+7])[0]
        if port:out.append(f'{host}:{port}')
        i+=7
    return list(dict.fromkeys(out))

def master():
    for host in ['master.ezz.lol','m.ezz.lol']:
        found=[]
        try:
            with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as s:
                s.connect((host,20810));s.settimeout(.5)
                s.send(OOB+b'getservers\nT7 8 full empty');deadline=time.monotonic()+3
                while time.monotonic()<deadline:
                    try:data=s.recv(65535)
                    except socket.timeout:continue
                    found+=parse_master(data)
                    if b'\\EOT' in data:break
        except OSError:continue
        if found:return list(dict.fromkeys(found))
    raise RuntimeError('No se pudo consultar la lista de servidores.')

def query(address):
    challenge=uuid.uuid4().hex
    try:
        with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as s:
            s.connect(endpoint(address));s.settimeout(1.5);start=time.monotonic()
            s.send(OOB+f'getInfo\n{challenge}'.encode());data=s.recv(65535)
        if not data.startswith(OOB+b'infoResponse\n'):return None
        parts=data.split(b'\n',1)[1].decode('utf-8','replace').strip('\x00\n').split('\\')
        if parts[0]=='':parts=parts[1:]
        info=dict(zip(parts[::2],parts[1::2]))
        if info.get('challenge')!=challenge or info.get('protocol')!='8' or info.get('gamename')!='T7':return None
        info.update(address=address,ping=round((time.monotonic()-start)*1000))
        return info
    except (OSError,ValueError):return None

def atomic_json(path,value):
    path.parent.mkdir(parents=True,exist_ok=True)
    temp=path.with_name(path.name+'.'+uuid.uuid4().hex+'.tmp')
    temp.write_text(json.dumps(value),encoding='utf-8');os.replace(temp,path)

def find_game():
    roots=[]
    if sys.platform=='win32':
        import winreg
        try:
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER,r'Software\Valve\Steam') as k:roots.append(Path(winreg.QueryValueEx(k,'SteamPath')[0]))
        except OSError:pass
    roots.append(Path(os.getenv('ProgramFiles(x86)','C:/Program Files (x86)'))/'Steam')
    for root in list(roots):
        vdf=root/'steamapps/libraryfolders.vdf'
        if vdf.exists():roots += [Path(p.replace('\\\\','\\')) for p in re.findall(r'"path"\s+"([^"]+)"',vdf.read_text(errors='replace'))]
    for root in roots:
        game=root/'steamapps/common/Call of Duty Black Ops III'
        if (game/'BlackOps3.exe').is_file():return str(game)
    return ''

def executable_checksum(path):
    with path.open('rb') as f:
        if f.read(2)!=b'MZ':raise ValueError('El archivo no es un ejecutable Windows.')
        f.seek(60);offset=struct.unpack('<I',f.read(4))[0]
        f.seek(offset)
        if f.read(4)!=b'PE\x00\x00':raise ValueError('Ejecutable PE inválido.')
        f.seek(offset+24+64);return struct.unpack('<I',f.read(4))[0]

class App:
    def __init__(self,root):
        import tkinter as tk
        from tkinter import ttk
        self.root=root;self.tk=tk;self.ttk=ttk;self.servers=[];self.busy=False
        DATA.mkdir(parents=True,exist_ok=True)
        try:self.settings=json.loads((DATA/'settings.json').read_text())
        except (OSError,ValueError):self.settings={}
        root.title('BO3 Community Client · Windows beta1');root.geometry('1000x660')
        self.game=tk.StringVar(value=self.settings.get('game',find_game()))
        self.name=tk.StringVar(value=self.settings.get('name',''))
        self.search=tk.StringVar();self.mode=tk.StringVar(value='Todos')
        self.empty=tk.BooleanVar();self.full=tk.BooleanVar();self.mods=tk.BooleanVar()
        self.status=tk.StringVar(value='Selecciona la instalación de BO3 de Steam.')
        bar=ttk.Frame(root,padding=10);bar.pack(fill='x')
        ttk.Entry(bar,textvariable=self.game,width=65).pack(side='left',fill='x',expand=True)
        ttk.Button(bar,text='Carpeta del juego',command=self.choose).pack(side='left')
        ttk.Button(bar,text='Abrir Steam',command=lambda:os.startfile('steam://run/311210')).pack(side='left')
        filters=ttk.Frame(root,padding=10);filters.pack(fill='x')
        ttk.Entry(filters,textvariable=self.search,width=32).pack(side='left')
        ttk.Combobox(filters,textvariable=self.mode,values=['Todos','Zombies','Multijugador'],state='readonly',width=15).pack(side='left')
        for label,var in [('Ocultar vacíos',self.empty),('Ocultar llenos',self.full),('Ocultar mods',self.mods)]:ttk.Checkbutton(filters,text=label,variable=var,command=self.render).pack(side='left')
        ttk.Button(filters,text='Actualizar',command=self.refresh).pack(side='right')
        columns=('name','mode','map','players','ping','address')
        self.table=ttk.Treeview(root,columns=columns,show='headings',selectmode='browse')
        for c,title in zip(columns,['Servidor','Modo','Mapa','Jugadores','Ping','Dirección']):
            self.table.heading(c,text=title,command=lambda c=c:self.sort(c));self.table.column(c,width=150)
        self.table.pack(fill='both',expand=True,padx=10)
        controls=ttk.Frame(root,padding=10);controls.pack(fill='x')
        ttk.Label(controls,text='Nombre:').pack(side='left');ttk.Entry(controls,textvariable=self.name,width=18).pack(side='left')
        for text,action in [('Unirse',self.join),('Respaldar perfil',self.backup),('Importar identidad',self.import_key),('Unlock All',self.unlock),('Abrir carpeta de datos',lambda:os.startfile(str(DATA)))]:ttk.Button(controls,text=text,command=action).pack(side='left',padx=4)
        ttk.Label(root,textvariable=self.status,wraplength=950,padding=10).pack(fill='x')
        self.search.trace_add('write',lambda *args:self.render());self.mode.trace_add('write',lambda *args:self.render())
        root.after(1000,self.heartbeat)
        if '--smoke-test' not in sys.argv:root.after(100,self.refresh)
    def error(self,text):
        from tkinter import messagebox
        messagebox.showerror('BO3 Community Client',str(text))
    def choose(self):
        from tkinter import filedialog
        value=filedialog.askdirectory(title='Carpeta que contiene BlackOps3.exe')
        if value:self.game.set(value)
    def render(self):
        self.table.delete(*self.table.get_children())
        for i,s in enumerate(self.servers):
            mode='Zombies' if s.get('playmode')=='0' else 'Multijugador'
            try:clients=int(s.get('clients',0));maximum=int(s.get('sv_maxclients',0))
            except ValueError:continue
            if self.search.get().casefold() not in (s.get('hostname','')+' '+s.get('mapname','')).casefold():continue
            if self.mode.get()!='Todos' and mode!=self.mode.get():continue
            if self.empty.get() and not clients:continue
            if self.full.get() and maximum and clients>=maximum:continue
            if self.mods.get() and (s.get('modId','0') not in ('','0') or s.get('usermapId','0') not in ('','0')):continue
            self.table.insert('', 'end',iid=str(i),values=(re.sub(r'\^[0-9]','',s.get('hostname','')),mode,s.get('mapname',''),f'{clients}/{maximum}',s['ping'],s['address']))
    def sort(self,column):
        keys={'name':'hostname','mode':'playmode','map':'mapname','players':'clients','ping':'ping','address':'address'}
        self.servers.sort(key=lambda s:int(s.get(keys[column],0)) if column in ('players','ping') else s.get(keys[column],''));self.render()
    def refresh(self):
        if self.busy:return
        self.busy=True;self.status.set('Consultando servidores…')
        def worker():
            try:
                with concurrent.futures.ThreadPoolExecutor(max_workers=24) as pool:servers=[s for s in pool.map(query,master()) if s]
                self.root.after(0,lambda:self.loaded(servers))
            except Exception as e:self.root.after(0,lambda e=e:self.loaded([],str(e)))
        threading.Thread(target=worker,daemon=True).start()
    def loaded(self,servers,error=''):
        self.busy=False;self.servers=servers;self.render();self.status.set(error or f'{len(servers)} servidores disponibles.')
    def backup(self):
        folder=DATA/'backups'/(time.strftime('%Y%m%d-%H%M%S')+'-'+uuid.uuid4().hex[:6]);folder.mkdir(parents=True,exist_ok=False)
        for name,src in [('players',Path(self.game.get())/'players'),('boiii',Path(os.getenv('LOCALAPPDATA',''))/'boiii')]:
            if src.is_dir():shutil.copytree(src,folder/name)
        self.status.set(f'Respaldo: {folder}');return folder
    def import_key(self):
        from tkinter import filedialog,messagebox
        selected=filedialog.askopenfilename(title='Clave privada de identidad BOIII/Ezz')
        if not selected:return
        try:
            status=json.loads((DATA/'status.json').read_text())
            if time.time()-status.get('time',0)<8:raise ValueError('Cierra el juego antes de importar una identidad.')
        except (OSError,json.JSONDecodeError):pass
        except ValueError as e:self.error(e);return
        if not messagebox.askyesno('Identidad','Cierra BO3 primero. ¿Reemplazar la identidad local después de respaldarla?'):return
        try:
            self.backup();target=Path(os.getenv('LOCALAPPDATA',''))/'boiii/user/ezz-private.key';target.parent.mkdir(parents=True,exist_ok=True)
            data=Path(selected).read_bytes()
            if not 64<=len(data)<=4096:raise ValueError('Formato o tamaño de clave inválido.')
            temp=target.with_suffix('.tmp');temp.write_bytes(data);os.replace(temp,target)
            self.status.set('Identidad importada; el motor validará su formato al arrancar.')
        except Exception as e:self.error(e)
    def join(self):
        selection=self.table.selection()
        if not selection:return
        try:
            s=self.servers[int(selection[0])];endpoint(s['address']);game=Path(self.game.get())
            name=self.name.get().strip()
            if name and (len(name.encode('utf-8'))>16 or any(c in name for c in '\\";\n\r%')):raise ValueError('Nombre inválido (máximo 16 caracteres).')
            if s.get('isPrivate')=='1':raise ValueError('Este servidor requiere contraseña; no se admite aún.')
            if not (game/'BlackOps3.exe').is_file():raise ValueError('Selecciona tu instalación de BO3.')
            if executable_checksum(game/'BlackOps3.exe')!=0x6531394:raise ValueError('La versión de Steam instalada no es compatible con el motor comunitario incluido. No se cambiará tu ejecutable. Puedes abrir Steam para jugar normalmente.')
            engine=Path(getattr(sys,'_MEIPASS',Path(__file__).parent))/'engine/boiii.exe'
            if not engine.is_file():raise ValueError('Falta el motor Windows en el paquete. Descarga la distribución completa.')
            atomic_json(DATA/'settings.json',{'game':str(game),'name':name})
            atomic_json(DATA/'join.request',{'version':1,'server':s['address'],'mode':s.get('playmode','0'),'name':name})
            self.start_engine(engine,game)
            self.status.set('Abriendo BO3; entra al modo seleccionado para conectar.')
        except Exception as e:self.error(e)
    def start_engine(self,engine,game):
        try:
            status=json.loads((DATA/'status.json').read_text())
            if time.time()-status.get('time',0)<5:return
        except (OSError,ValueError):pass
        assets=engine.parent/'data'
        if not assets.is_dir():raise ValueError('Faltan los archivos del motor: descarga el ZIP completo.')
        target=Path(os.getenv('LOCALAPPDATA',''))/'boiii/data'
        marker=DATA/'engine-assets-v3.0.0.ready'
        if not marker.exists():
            if target.exists():
                backup=DATA/'backups'/('engine-data-'+uuid.uuid4().hex[:8]);backup.parent.mkdir(parents=True,exist_ok=True);shutil.copytree(target,backup)
            shutil.copytree(assets,target,dirs_exist_ok=True)
            marker.write_text('Ezz v3.0.0 assets installed.\n')
        subprocess.Popen([str(engine),'-launch','-noupdate'],cwd=game)
    def unlock(self):
        from tkinter import messagebox
        mode=self.mode.get()
        if mode=='Todos':self.error('Elige Zombies o Multijugador en el filtro antes de aplicar Unlock All.');return
        if not messagebox.askyesno('Unlock All','Se respaldará el perfil. Ezz Unlock All ajusta el rango a 997 y activa accesorios, camuflajes y GobbleGums del cliente. El inventario de Steam no cambia. ¿Aplicar?'):return
        try:
            game=Path(self.game.get());engine=Path(getattr(sys,'_MEIPASS',Path(__file__).parent))/'engine/boiii.exe'
            if not engine.is_file():raise ValueError('Falta el motor en el paquete completo.')
            if executable_checksum(game/'BlackOps3.exe')!=0x6531394:raise ValueError('Ejecutable no compatible con Ezz v3.0.0.')
            backup=self.backup();(backup/'backup.ready').write_text('Respaldo previo a Unlock All.\n')
            atomic_json(DATA/'join.request',{'version':1,'action':'unlock','mode':'0' if mode=='Zombies' else '1','backup':str(backup)})
            self.start_engine(engine,game);self.status.set('Entra al menú elegido para aplicar Unlock All con respaldo.')
        except Exception as e:self.error(e)
    def heartbeat(self):
        try:
            status=json.loads((DATA/'status.json').read_text())
            if time.time()-status.get('time',0)<8:self.status.set(status.get('message','Motor activo'))
        except (OSError,ValueError):pass
        self.root.after(1000,self.heartbeat)

if __name__=='__main__':
    import tkinter as tk
    root=tk.Tk();app=App(root)
    if '--smoke-test' in sys.argv:root.after(500,root.destroy)
    root.mainloop()
