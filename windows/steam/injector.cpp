// GPL-3.0. Loads our DLL only into the selected, running Steam BO3 executable.
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
struct Handle {HANDLE value=nullptr;explicit Handle(HANDLE v):value(v){}~Handle(){if(value&&value!=INVALID_HANDLE_VALUE)CloseHandle(value);}operator HANDLE() const{return value;}};
namespace fs=std::filesystem;
uintptr_t module(DWORD pid,const std::wstring& name){
 Handle snap(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid));MODULEENTRY32W entry{};entry.dwSize=sizeof(entry);
 if(Module32FirstW(snap,&entry))do{if(_wcsicmp(entry.szModule,name.c_str())==0)return reinterpret_cast<uintptr_t>(entry.modBaseAddr);}while(Module32NextW(snap,&entry));return 0;
}
DWORD find_game(const fs::path& expected){Handle snap(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0));PROCESSENTRY32W e{};e.dwSize=sizeof(e);
 if(Process32FirstW(snap,&e))do{if(_wcsicmp(e.szExeFile,L"BlackOps3.exe")!=0)continue;Handle p(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,e.th32ProcessID));if(!p)continue;wchar_t path[32768];DWORD length=32768;if(QueryFullProcessImageNameW(p,0,path,&length)&&fs::equivalent(fs::path(path),expected))return e.th32ProcessID;}while(Process32NextW(snap,&e));return 0;
}
void check_binary(const fs::path& path){std::ifstream in(path,std::ios::binary);IMAGE_DOS_HEADER dos{};in.read(reinterpret_cast<char*>(&dos),sizeof(dos));if(!in||dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<64||dos.e_lfanew>1048576)throw std::runtime_error("Invalid Steam PE header");in.seekg(dos.e_lfanew);IMAGE_NT_HEADERS64 nt{};in.read(reinterpret_cast<char*>(&nt),sizeof(nt));
 if(!in||nt.Signature!=IMAGE_NT_SIGNATURE||nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||nt.OptionalHeader.CheckSum!=0x6531394||nt.FileHeader.TimeDateStamp!=0x6A7B6355)throw std::runtime_error("Unsupported Steam BO3 version; no process was modified");
}
DWORD run_remote(HANDLE process,uintptr_t function,void* arg){Handle thread(CreateRemoteThread(process,nullptr,0,reinterpret_cast<LPTHREAD_START_ROUTINE>(function),arg,0,nullptr));if(!thread)throw std::runtime_error("Cannot start DLL loader in Steam BO3");
 if(WaitForSingleObject(thread,20000)!=WAIT_OBJECT_0)throw std::runtime_error("DLL loader timed out; restart BO3 before retrying");DWORD code=0;if(!GetExitCodeThread(thread,&code))throw std::runtime_error("Cannot read DLL loader result");return code;
}
void attach(DWORD pid,const fs::path& dll){
 Handle process(OpenProcess(PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ,FALSE,pid));if(!process)throw std::runtime_error("Cannot access selected BO3 process; launch both apps under the same Windows user");
 uintptr_t loaded=module(pid,dll.filename().wstring());
 if(!loaded){auto loader=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");HMODULE owner=nullptr;if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(loader),&owner))throw std::runtime_error("Cannot locate system DLL loader");wchar_t ownerPath[32768];GetModuleFileNameW(owner,ownerPath,32768);auto remoteOwner=module(pid,fs::path(ownerPath).filename().wstring());if(!remoteOwner)throw std::runtime_error("System DLL loader is unavailable in BO3");
  auto text=dll.wstring();auto bytes=(text.size()+1)*sizeof(wchar_t);void* remote=VirtualAllocEx(process,nullptr,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!remote)throw std::runtime_error("Cannot allocate DLL path in BO3");
  SIZE_T wrote=0;if(!WriteProcessMemory(process,remote,text.c_str(),bytes,&wrote)||wrote!=bytes){VirtualFreeEx(process,remote,0,MEM_RELEASE);throw std::runtime_error("Cannot write DLL path");}
  // Keep the path allocated on timeout: a still-running loader may reference it.
  run_remote(process,remoteOwner+reinterpret_cast<uintptr_t>(loader)-reinterpret_cast<uintptr_t>(owner),remote);VirtualFreeEx(process,remote,0,MEM_RELEASE);
  loaded=module(pid,dll.filename().wstring());if(!loaded)throw std::runtime_error("Steam integration DLL could not be loaded");
 }
 HMODULE local=LoadLibraryExW(dll.c_str(),nullptr,DONT_RESOLVE_DLL_REFERENCES);if(!local)throw std::runtime_error("Cannot inspect integration DLL");auto init=GetProcAddress(local,"BO3Community_Initialize");if(!init){FreeLibrary(local);throw std::runtime_error("Integration export missing");}auto offset=reinterpret_cast<uintptr_t>(init)-reinterpret_cast<uintptr_t>(local);FreeLibrary(local);if(run_remote(process,loaded+offset,nullptr)!=0)throw std::runtime_error("Steam integration initialization failed");
}
int wmain(int argc,wchar_t** argv){try{
 if(argc==2&&std::wstring(argv[1])==L"--smoke-test"){std::cout<<"Steam attach helper x64 ready\n";return 0;}
 if(argc!=5||std::wstring(argv[1])!=L"--game"||std::wstring(argv[3])!=L"--dll")throw std::runtime_error("Usage: BO3SteamAttach --game BlackOps3.exe --dll BO3SteamFix.dll");
 auto game=fs::canonical(argv[2]),dll=fs::canonical(argv[4]);check_binary(game);if(_wcsicmp(dll.filename().c_str(),L"BO3SteamFix.dll")!=0)throw std::runtime_error("Unexpected integration DLL");
 DWORD pid=0;for(int i=0;i<90;i++){pid=find_game(game);if(pid)break;Sleep(1000);}if(!pid)throw std::runtime_error("BO3 did not start; open the selected game in Steam");
 // Wait for the original game's startup/unpacking before inspecting hooks.
 Sleep(12000);attach(pid,dll);std::cout<<"Steam BO3 integration loaded\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
