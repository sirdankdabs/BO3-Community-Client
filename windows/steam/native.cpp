// GPL-3.0. Native Steam integration; no Steam API emulation or executable replacement.
// September 2026 symbols/ABI: pinned Ezz game/symbols and game/structs.
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <ctime>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>
#include <stdexcept>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include "GameBuild.h"
#include "Arxan.h"
#include "minhook/include/MinHook.h"
#include "protocol.hpp"
extern "C" {
#include <tomcrypt.h>
}
namespace community {
namespace fs=std::filesystem;
struct Address { uint32_t ip;uint16_t port;uint16_t pad;int type;int local; };
static_assert(sizeof(Address)==16);
struct Message { int overflow,readonly;uint8_t* data;uint8_t* split;int maxsize,cursize,splitSize,readcount,bit,lastEntity,flush,targetLocal; };
static_assert(sizeof(Message)==0x38);
#pragma pack(push,1)
struct SessionInfo { uint8_t id[8],host[37],key[16]; };
#pragma pack(pop)
static_assert(sizeof(SessionInfo)==61);
uintptr_t address(uintptr_t va) { return reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))+va-0x140000000ULL; }
template<class T>T at(uintptr_t va){return reinterpret_cast<T>(address(va));}
bool accessible(const void* ptr,size_t length,bool writable=false) {
 MEMORY_BASIC_INFORMATION m{};if(!ptr||!VirtualQuery(ptr,&m,sizeof(m))||m.State!=MEM_COMMIT||(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
 if(reinterpret_cast<uintptr_t>(ptr)+length>reinterpret_cast<uintptr_t>(m.BaseAddress)+m.RegionSize)return false;
 return !writable||(m.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY));
}
fs::path folder(){PWSTR p=nullptr;if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&p)))throw std::runtime_error("Cannot find LocalAppData");fs::path result=fs::path(p)/L"BO3 Community Client";CoTaskMemFree(p);fs::create_directories(result);return result;}
std::mutex status_mutex;std::string status_message="Preparando parche para BO3 de Steam";
void status(const std::string& message="") {
 std::lock_guard lock(status_mutex);if(!message.empty())status_message=message;
 rapidjson::StringBuffer buf;rapidjson::Writer<rapidjson::StringBuffer>w(buf);w.StartObject();w.Key("time");w.Int64(time(nullptr));w.Key("pid");w.Uint(GetCurrentProcessId());w.Key("backend");w.String("steam-native");w.Key("message");w.String(status_message.c_str());w.EndObject();
 auto temp=folder()/L"status.native.tmp",dest=folder()/L"status.json";
 {std::ofstream out(temp,std::ios::binary);out<<buf.GetString();}
 MoveFileExW(temp.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING);
}
void log(const std::string& message){std::ofstream out(folder()/L"steam-engine.log",std::ios::app);out<<message<<'\n';}
int& state(int client=0){return *at<int*>(0x145359BC0+client*0x1078+8);}
uint8_t* connection(){auto p=*at<uint8_t**>(0x145359BB8);return accessible(p,0x25780,true)?p:nullptr;}
Address* peer(){auto p=connection();return p?reinterpret_cast<Address*>(p+0x10):nullptr;}
struct Change {
 uintptr_t va;std::vector<uint8_t> old,replacement;
 void apply(std::vector<uint8_t> value){auto p=at<uint8_t*>(va);if(!accessible(p,value.size()))throw std::runtime_error("Patch site unreadable");old.assign(p,p+value.size());replacement=std::move(value);write(replacement);}
 void write(const std::vector<uint8_t>& value){DWORD oldProtect;if(!VirtualProtect(at<void*>(va),value.size(),PAGE_EXECUTE_READWRITE,&oldProtect))throw std::runtime_error("Patch protection failed");memcpy(at<void*>(va),value.data(),value.size());DWORD ignored;VirtualProtect(at<void*>(va),value.size(),oldProtect,&ignored);FlushInstructionCache(GetCurrentProcess(),at<void*>(va),value.size());}
 void restore(){if(old.empty())return;auto p=at<uint8_t*>(va);if(memcmp(p,replacement.data(),replacement.size())==0)write(old);old.clear();replacement.clear();}
};
// The two instructions are restored before returning to Steam matchmaking.
Change initial_state{0x14134C590+0x170},dw_connection_check{0x1421F137D};
struct Target { bool active=false,launched=false,queried=false;Address adr{};SOCKET socket=INVALID_SOCKET;uint64_t started=0,lastQuery=0,lastChallenge=0;std::string nonce,name,map,gametype;int mode=0;uint64_t serverXuid=0,steamXuid=0; } target;
std::array<uint8_t,32> challenge{};bool haveChallenge=false,usedChallenge=false;
ecc_key identity{};uint64_t identityXuid=0;std::string publicKey;bool keyReady=false;int prng=-1;
void require(int result,const char* reason){if(result!=CRYPT_OK)throw std::runtime_error(reason);}
void load_key(){
 if(keyReady)return;ltc_mp=ltm_desc;prng=register_prng(&sprng_desc);register_hash(&sha1_desc);register_hash(&sha512_desc);
 auto path=folder()/L"identity.key";std::vector<unsigned char> saved;
 if(fs::exists(path)){std::ifstream in(path,std::ios::binary);saved.assign(std::istreambuf_iterator<char>(in),{});if(saved.size()<64||saved.size()>4096)throw std::runtime_error("Invalid identity key size");require(ecc_import(saved.data(),saved.size(),&identity),"Identity key import failed");if(identity.type!=PK_PRIVATE)throw std::runtime_error("A private identity key is required");}
 else{require(ecc_make_key(nullptr,prng,64,&identity),"P-521 key generation failed");unsigned char data[2048];unsigned long len=sizeof(data);require(ecc_export(data,&len,PK_PRIVATE,&identity),"Private key export failed");auto tmp=path;tmp+=L".new";{std::ofstream out(tmp,std::ios::binary);out.write(reinterpret_cast<char*>(data),len);if(!out)throw std::runtime_error("Identity key write failed");}if(!MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Identity key commit failed");}
 unsigned char data[2048],point[256],digest[20];unsigned long len=sizeof(data),pointLen=sizeof(point),digestLen=sizeof(digest);
 require(ecc_export(data,&len,PK_PUBLIC,&identity),"Public key export failed");publicKey.assign(reinterpret_cast<char*>(data),len);
 require(ecc_ansi_x963_export(&identity,point,&pointLen),"P-521 public point export failed");require(hash_memory(find_hash("sha1"),point,pointLen,digest,&digestLen),"Identity hash failed");memcpy(&identityXuid,digest,8);keyReady=true;
}
using Frame=void(*)();Frame originalFrame=nullptr;
using Resend=void(*)(int);Resend originalResend=nullptr;
using Send=bool(*)(int,int,const void*,const Address*);Send originalSend=nullptr;
using Oob=bool(*)(int,const Address*,const void*,int);Oob originalOob=nullptr;
using Receive=int(*)(Address*,Message*);Receive originalReceive=nullptr;
using Lookup=int(*)(uint64_t);Lookup originalLookup=nullptr;
using Disconnect=void(*)(int,bool);Disconnect originalDisconnect=nullptr;
using Cache=void(*)(int);Cache originalCache=nullptr;
bool matches(const Address* adr){return adr&&target.active&&adr->ip==target.adr.ip&&adr->port==target.adr.port;}
bool send_bytes(const uint8_t* data,size_t size){sockaddr_in to{};to.sin_family=AF_INET;to.sin_addr.s_addr=target.adr.ip;to.sin_port=htons(target.adr.port);return sendto(target.socket,reinterpret_cast<const char*>(data),static_cast<int>(size),0,reinterpret_cast<sockaddr*>(&to),sizeof(to))==static_cast<int>(size);}
bool raw(std::string_view command){std::vector<uint8_t> data{255,255,255,255};data.insert(data.end(),command.begin(),command.end());return send_bytes(data.data(),data.size());}
void end(const std::string& reason){
 target.active=false;initial_state.restore();dw_connection_check.restore();if(target.socket!=INVALID_SOCKET){closesocket(target.socket);target.socket=INVALID_SOCKET;}haveChallenge=usedChallenge=false;status(reason);log(reason);
}
bool send_hook(int sock,int length,const void* bytes,const Address* to){
 if(!matches(to))return originalSend(sock,length,bytes,to);
 if(length<0||length>65535||!bytes)return false;
 std::vector<uint8_t> packet(static_cast<size_t>(length)+1);packet[0]=static_cast<uint8_t>((sock&15)|((to->local&15)<<4));memcpy(packet.data()+1,bytes,length);
 if(length>=17&&memcmp(bytes,"\xff\xff\xff\xff",4)!=0){uint64_t xuid;memcpy(&xuid,packet.data()+10,8);if(xuid==target.steamXuid)memcpy(packet.data()+10,&identityXuid,8);}
 return send_bytes(packet.data(),packet.size());
}
bool oob_impl(int sock,const Address* to,const void* bytes,int length){
 if(!matches(to))return originalOob(sock,to,bytes,length);
 if(!bytes||length<0||length>8192)return false;
 std::string_view data(static_cast<const char*>(bytes),static_cast<size_t>(length));
 if(data.starts_with("connect \"")){
  if(!haveChallenge||usedChallenge){haveChallenge=false;target.lastChallenge=0;return true;}
  usedChallenge=true;auto command=wire::connect(data,identityXuid,target.name);unsigned char digest[64],signature[256];unsigned long digestLen=64,sigLen=256;
  require(hash_memory(find_hash("sha512"),challenge.data(),challenge.size(),digest,&digestLen),"Challenge hash failed");
  ltc_ecc_sig_opts opts{};opts.type=LTC_ECCSIG_ANSIX962;opts.wprng=prng;
  require(ecc_sign_hash_v2(digest,digestLen,signature,&sigLen,&opts,&identity),"Challenge signature failed");
  std::vector<uint8_t> payload;wire::put_string(payload,publicKey);wire::put_string(payload,{reinterpret_cast<char*>(signature),sigLen});wire::put(payload,int32_t(3));wire::put_string(payload,"");wire::put_string(payload,command);
  static uint64_t fragmentId=0;bool ok=true;for(auto& p:wire::fragments(payload,++fragmentId))ok=raw({reinterpret_cast<char*>(p.data()),p.size()})&&ok;
  log("Signed P-521 connect sent");return ok;
 }
 std::vector<uint8_t> copy(data.begin(),data.end());
 // Stock Steam XUID remains untouched in the local controller and profile.
 if(copy.size()>=12&&memcmp(copy.data(),"cfl\0",4)==0)memcpy(copy.data()+4,&identityXuid,8);
 if(copy.size()>=14&&memcmp(copy.data(),"stats\0",6)==0)memcpy(copy.data()+6,&identityXuid,8);
 return originalOob(sock,to,copy.data(),static_cast<int>(copy.size()));
}
bool oob_hook(int sock,const Address* to,const void* bytes,int length){try{return oob_impl(sock,to,bytes,length);}catch(const std::exception& e){end(e.what());return false;}}
void resend_hook(int client){
 if(target.active&&target.launched&&client==0){auto p=peer();if(p&&state()>=4&&state()<=6){*p=target.adr;if(!haveChallenge){state()=4;auto now=GetTickCount64();if(now-target.lastChallenge>1500){target.lastChallenge=now;raw("getchallenge");}return;}}}
 originalResend(client);
}
int lookup_hook(uint64_t xuid){return target.active&&xuid==identityXuid?0:originalLookup(xuid);}
void disconnect_hook(int client,bool deactivate){if(client==0&&target.active)end("Sesión comunitaria terminada; Steam restaurado");originalDisconnect(client,deactivate);}
void cache_hook(int client){if(!target.active)originalCache(client);}
bool safe_token(const std::string& s){return !s.empty()&&s.size()<64&&s.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")==s.npos;}
void info_response(std::string_view data){
 auto pos=data.find_first_of(" \n");if(pos==data.npos)return;auto fields=wire::info(data.substr(pos+1));
 if(fields["challenge"]!=target.nonce||fields["protocol"]!="8"||fields["gamename"]!="T7")return;
 if(fields["playmode"]!=std::to_string(target.mode))throw std::runtime_error("Server mode differs from request");
 for(auto name:{"usermap_id","mod_id"})if(!fields[name].empty()&&fields[name]!="0")throw std::runtime_error("Workshop content needs native Steam integration testing; use a stock-map server for this beta");
 if(!safe_token(fields["mapname"])||!safe_token(fields["gametype"]))throw std::runtime_error("Invalid map or gametype");
 target.map=fields["mapname"];target.gametype=fields["gametype"];target.queried=true;
 auto xuid=fields["xuid"];if(!xuid.empty()){size_t used;target.serverXuid=std::stoull(xuid,&used,16);if(used!=xuid.size())throw std::runtime_error("Invalid server XUID");}
 status("Servidor validado; entra al menú Zombies/Multijugador seleccionado");
}
int receive_hook(Address* from,Message* msg){
 if(!target.active)return originalReceive(from,msg);
 for(int i=0;i<32;i++){
  std::array<uint8_t,65536> buffer{};sockaddr_in source{};int sourceLength=sizeof(source);int length=recvfrom(target.socket,reinterpret_cast<char*>(buffer.data()),static_cast<int>(buffer.size()),0,reinterpret_cast<sockaddr*>(&source),&sourceLength);
  if(length<=0)break;if(source.sin_addr.s_addr!=target.adr.ip||ntohs(source.sin_port)!=target.adr.port)continue;
  try{
   const auto p=wire::packet(buffer.data(),static_cast<size_t>(length));auto data=buffer.data()+p.offset;size_t size=static_cast<size_t>(length)-p.offset;
   if(size>=4&&memcmp(data,"\xff\xff\xff\xff",4)==0){std::string_view command(reinterpret_cast<char*>(data+4),size-4);
    if(command.starts_with("infoResponse")){info_response(command);continue;}
    if(command.starts_with("getChallengeResponse ")&&command.size()==21+32){memcpy(challenge.data(),command.data()+21,32);haveChallenge=true;usedChallenge=false;if(target.launched&&state()==4)state()=5;continue;}
    if(command.starts_with("profileInfo")||command.starts_with("playerXuid"))continue;
    if(command.starts_with("error Bad signature")){haveChallenge=false;target.lastChallenge=0;continue;}
    if(command.starts_with("error ")||command.starts_with("error\n")){end("Servidor: "+std::string(command.substr(6,256)));break;}
   }
   if(!target.launched||!msg||!msg->data||msg->maxsize<static_cast<int>(size)||size>65535)continue;
   // Match the postconditions of native Sys_GetPacket without relaxing Steam packet validation.
   memcpy(msg->data,data,size);msg->cursize=static_cast<int>(size);msg->readcount=1;msg->bit=0;msg->targetLocal=p.remote;msg->overflow=0;msg->splitSize=0;*from=target.adr;from->local=p.local;return 1;
  }catch(const std::exception& e){end(e.what());break;}
 }
 return originalReceive(from,msg);
}
void begin(const rapidjson::Document& request){
 if(target.active)throw std::runtime_error("Disconnect from the current community server first");
 if(!request.HasMember("server")||!request["server"].IsString())throw std::runtime_error("Missing server address");
 std::string value=request["server"].GetString();auto colon=value.find(':');if(colon==value.npos)throw std::runtime_error("Missing port");auto ip=value.substr(0,colon),portText=value.substr(colon+1);IN_ADDR ipAddress{};if(InetPtonA(AF_INET,ip.c_str(),&ipAddress)!=1)throw std::runtime_error("Invalid IPv4");size_t used;auto port=std::stoul(portText,&used);if(used!=portText.size()||!port||port>65535)throw std::runtime_error("Invalid port");
 Target next{};next.adr={ipAddress.S_un.S_addr,static_cast<uint16_t>(port),0,4,4};next.mode=std::stoi(request["mode"].GetString());if(next.mode!=0&&next.mode!=1)throw std::runtime_error("Invalid mode");
 if(request.HasMember("name")&&request["name"].IsString())next.name=request["name"].GetString();if(next.name.size()>16||next.name.find_first_of("\\\";\r\n%")!=next.name.npos)throw std::runtime_error("Invalid player name");
 if(!at<bool(*)()>(0x1420EF8F0)()||at<bool(*)()>(0x1420EF860)())throw std::runtime_error("Return to the game menu before joining");
 load_key();next.socket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);if(next.socket==INVALID_SOCKET)throw std::runtime_error("Cannot create community socket");sockaddr_in local{};local.sin_family=AF_INET;
 if(bind(next.socket,reinterpret_cast<sockaddr*>(&local),sizeof(local))!=0){closesocket(next.socket);throw std::runtime_error("Cannot bind community socket");}
 u_long nonblocking=1;ioctlsocket(next.socket,FIONBIO,&nonblocking);next.started=GetTickCount64();next.nonce=std::to_string(next.started)+"-"+std::to_string(GetCurrentProcessId());next.active=true;target=std::move(next);haveChallenge=usedChallenge=false;status("Consultando servidor desde BO3 de Steam");
}
void launch(){
 if(at<int(*)()>(0x1420EA5B0)()!=target.mode)return;
 auto p=peer();if(!p)return;target.steamXuid=at<uint64_t(*)(int)>(0x141EBA880)(0);
 auto initial=at<uint32_t*>(initial_state.va);if(!accessible(initial,4)||*initial>12)throw std::runtime_error("Unexpected CL_ConnectFromLobby state instruction");
 auto branch=at<uint8_t*>(dw_connection_check.va);if(!accessible(branch,1)||(*branch<0x70||*branch>0x7f))throw std::runtime_error("Unexpected native connection check instruction");
 try{initial_state.apply({4,0,0,0});dw_connection_check.apply({0xeb});SessionInfo info{};target.launched=true;
 at<void(*)(int,SessionInfo*,const Address*,int,int,const char*,const char*,const char*)>(0x14134C590)(0,&info,&target.adr,1,0,target.map.c_str(),target.gametype.c_str(),"");
 status("Conectando al servidor dentro del BO3 original de Steam");
 }catch(...){end("No se pudo preparar la conexión nativa");throw;}
}
void tick(){
 static uint64_t nextStatus=0,nextRequest=0;auto now=GetTickCount64();
 if(now>=nextStatus){status();nextStatus=now+1000;}
 if(target.active){if(now-target.started>45000&&(!target.launched||state()<=5)){end("Conexión agotada; Steam restaurado");return;}
  if(!target.queried&&now-target.lastQuery>2000){raw("getInfo\n"+target.nonce);target.lastQuery=now;}
  if(target.queried&&!target.launched)launch();
  if(target.launched&&state()>5){auto p=connection();if(p&&target.serverXuid){auto remote=reinterpret_cast<Address*>(p+0x2455c);*remote=target.adr;memcpy(p+0x24b68,&target.serverXuid,8);} }
  if(target.launched&&state()<4)end("Sesión terminada; Steam restaurado");
 }
 if(now<nextRequest)return;nextRequest=now+500;auto path=folder()/L"join.request";if(!fs::exists(path))return;
 if(!at<bool(*)()>(0x1420EF8F0)()||at<bool(*)()>(0x1420EF860)())return;
 if(fs::file_size(path)>8192){fs::remove(path);throw std::runtime_error("Request exceeds limit");}
 std::ifstream in(path,std::ios::binary);std::string text((std::istreambuf_iterator<char>(in)),{});in.close();rapidjson::Document d;d.Parse(text.c_str());fs::remove(path);
 if(d.HasParseError()||!d.IsObject()||!d.HasMember("version")||!d["version"].IsInt()||d["version"].GetInt()!=2||!d.HasMember("mode")||!d["mode"].IsString())throw std::runtime_error("Unsupported Steam request");
 if(d.HasMember("action"))throw std::runtime_error("Los cambios permanentes de perfil de Steam están desactivados en esta prueba de conexión");
 begin(d);
}
void frame_hook(){originalFrame();try{tick();}catch(const std::exception& e){if(target.active)end(e.what());else status(e.what());log(e.what());}}
template<class T>void hook(uintptr_t va,T callback,T* original){auto p=at<void*>(va);if(!accessible(p,16))throw std::runtime_error("Native hook target unreadable");auto result=MH_CreateHook(p,reinterpret_cast<void*>(callback),reinterpret_cast<void**>(original));if(result!=MH_OK)throw std::runtime_error("Native hook creation failed");}
DWORD WINAPI initialize(void*){
 try{
  status();if(bo3::current_build()!=bo3::Build::September2026)throw std::runtime_error("This Steam executable is not supported");
  auto base=GetModuleHandleW(nullptr);auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(reinterpret_cast<uint8_t*>(base)+dos->e_lfanew);if(nt->OptionalHeader.CheckSum!=0x6531394)throw std::runtime_error("Steam executable checksum differs");
  std::string message;if(!arxan_bypass::install(message))throw std::runtime_error(message);log(message);
  WSADATA wsa{};if(WSAStartup(MAKEWORD(2,2),&wsa)!=0)throw std::runtime_error("Winsock unavailable");
  auto result=MH_Initialize();if(result!=MH_OK&&result!=MH_ERROR_ALREADY_INITIALIZED)throw std::runtime_error("MinHook unavailable");
  hook(0x1420EC680,frame_hook,&originalFrame);hook(0x14134B990,resend_hook,&originalResend);hook(0x1422B9240,send_hook,&originalSend);hook(0x14211AB40,oob_hook,&originalOob);hook(0x1422B9040,receive_hook,&originalReceive);hook(0x141EBA340,lookup_hook,&originalLookup);hook(0x14135D880,disconnect_hook,&originalDisconnect);hook(0x1400A7570,cache_hook,&originalCache);
  // Enable only this module's hooks, leaving any other MinHook users untouched.
  for(auto va:{0x1420EC680ULL,0x14134B990ULL,0x1422B9240ULL,0x14211AB40ULL,0x1422B9040ULL,0x141EBA340ULL,0x14135D880ULL,0x1400A7570ULL})if(MH_EnableHook(at<void*>(va))!=MH_OK)throw std::runtime_error("Native hook activation failed");
  status("BO3 original de Steam conectado a la app; partidas públicas conservan su transporte");return 0;
 }catch(const std::exception& e){status(e.what());log(e.what());return 1;}
}
}
extern "C" __declspec(dllexport) DWORD WINAPI BO3Community_Initialize(void*) {
 static std::atomic_bool started=false;if(started.exchange(true))return 0;
 HANDLE worker=CreateThread(nullptr,0,community::initialize,nullptr,0,nullptr);if(!worker){started=false;return 1;}CloseHandle(worker);return 0;
}
