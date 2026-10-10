#include <std_include.hpp>
#include <loader/component_loader.hpp>
#include "scheduler.hpp"
#include <game/game.hpp>
#include <utils/properties.hpp>
#include <fstream>
#include <regex>
#include <charconv>
namespace community_client {
namespace {
std::filesystem::path data_dir() {
  auto base=utils::properties::get_appdata_path().parent_path()/"BO3 Community Client";
  std::filesystem::create_directories(base);return base;
}
void status(const std::string& message) {
  rapidjson::Document doc;doc.SetObject();auto& a=doc.GetAllocator();
  doc.AddMember("time",static_cast<int64_t>(std::time(nullptr)),a);
  doc.AddMember("pid",static_cast<unsigned>(GetCurrentProcessId()),a);
  doc.AddMember("message",rapidjson::Value(message.c_str(),a),a);
  rapidjson::StringBuffer buffer;rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);doc.Accept(writer);
  auto file=data_dir()/"status.json",tmp=data_dir()/"status.tmp";
  {std::ofstream out(tmp,std::ios::binary);out<<buffer.GetString();}
  MoveFileExW(tmp.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
}
std::optional<int> rank_xp(const char* name,int row,int increment=0) {
  auto* table=game::db::xasset::DB_FindXAssetHeader(game::db::xasset::XAssetType::STRINGTABLE,name,false,0).stringTable;
  if(!table || row<0 || row>=table->rowCount || table->columnCount<=7)return {};
  game::symbol<const char*(const game::ddl::StringTable*,int,int)> cell{0x142251B40,0x1422AE660,0x0};
  const char* value=cell(table,row,7);if(!value)return {};
  int xp=0;const auto end=value+strlen(value);auto parsed=std::from_chars(value,end,xp);
  if(parsed.ec!=std::errc{} || parsed.ptr!=end || xp<0 || xp>INT_MAX-increment)return {};
  return xp+increment;
}
std::optional<std::string> rank997(int mode) {
  auto base=rank_xp(mode ? "gamedata/tables/mp/mp_ranktable.csv":"gamedata/tables/zm/zm_ranktable.csv",mode ? 56:35,mode ? 55600:54244);
  auto paragon=rank_xp(mode ? "gamedata/tables/mp/mp_paragonranktable.csv":"gamedata/tables/zm/zm_paragonranktable.csv",mode ? 942:962);
  if(!base || !paragon)return {};
  return "statsetbyname rankxp "+std::to_string(*base)+"\nstatsetbyname plevel 11\nstatsetbyname hasprestiged 1\nstatsetbyname rank "+
    std::to_string(mode ? 54:34)+"\nstatsetbyname paragon_rank "+std::to_string(mode ? 941:961)+
    "\nstatsetbyname paragon_rankxp "+std::to_string(*paragon)+"\nuploadStats "+std::to_string(mode)+"\n";
}
void tick() {
  const auto file=data_dir()/"join.request";
  if(!std::filesystem::exists(file)) {status("Motor Windows activo");return;}
  std::ifstream input(file,std::ios::binary);
  const std::string text{std::istreambuf_iterator<char>(input),{}};input.close();
  rapidjson::Document doc;doc.Parse(text.c_str());
  if(text.size()>4096 || doc.HasParseError() || !doc.IsObject() ||
     !doc.HasMember("version") || !doc["version"].IsInt() || doc["version"].GetInt()!=1 ||
     !doc.HasMember("mode") || !doc["mode"].IsString()) {
    std::filesystem::remove(file);status("Solicitud inválida");return;
  }
  const std::string mode=doc["mode"].GetString();
  if(mode!="0" && mode!="1") {std::filesystem::remove(file);status("Modo no compatible");return;}
  if(!game::com::Com_IsRunningUILevel() || static_cast<int>(game::com::Com_SessionMode_GetMode())!=std::stoi(mode)) {
    status(mode=="0" ? "Entra al menú de Zombies para aplicar la solicitud" : "Entra al menú multijugador para aplicar la solicitud");return;
  }
  std::string commands;
  if(doc.HasMember("action") && doc["action"].IsString() && std::string(doc["action"].GetString())=="unlock") {
    if(!doc.HasMember("backup") || !doc["backup"].IsString() ||
       !std::filesystem::is_regular_file(std::filesystem::path(doc["backup"].GetString())/"backup.ready")) {
      std::filesystem::remove(file);status("Falta respaldo: desbloqueo omitido");return;
    }
    const int selected_mode=std::stoi(mode);auto rank=rank997(selected_mode);
    if(!rank) {status("Tablas de XP no disponibles; vuelve a intentarlo en el menú");return;}
    commands=*rank+"unlockall\n";
    // unlockall appends its own rank commands. Apply 997 after that queue drains.
    scheduler::once([selected_mode,rank=*rank] {
      if(game::com::Com_IsRunningUILevel() && static_cast<int>(game::com::Com_SessionMode_GetMode())==selected_mode) {
        game::cbuf::Cbuf_AddText(game::LOCAL_CLIENT_0,rank.c_str());
        status("Rango 997 solicitado; verifica el guardado tras reiniciar");
      } else status("Unlock All aplicado; vuelve al menú para ajustar el rango a 997");
    },scheduler::main,2s);
  } else {
    if(!doc.HasMember("server") || !doc["server"].IsString()) {std::filesystem::remove(file);return;}
    const std::string server=doc["server"].GetString();
    if(!std::regex_match(server,std::regex(R"((\d{1,3}\.){3}\d{1,3}:\d{1,5})"))) {std::filesystem::remove(file);status("Dirección inválida");return;}
    if(doc.HasMember("name") && doc["name"].IsString()) {
      const std::string name=doc["name"].GetString();
      if(name.size()>16 || name.find_first_of("\\\";\n\r%")!=std::string::npos) {std::filesystem::remove(file);status("Nombre inválido");return;}
      if(!name.empty()) commands="name \""+name+"\"\n";
    }
    commands+="connect "+server+"\n";
  }
  std::filesystem::remove(file);
  game::cbuf::Cbuf_AddText(game::LOCAL_CLIENT_0,commands.c_str());
  status("Solicitud enviada al juego; verifica el resultado dentro de BO3");
}
}
struct component final : generic_component {
#ifndef NDEBUG
 std::string name() override {return "community_client";}
#endif
 void post_unpack() override {
   if(game::is_server())return;
   scheduler::on_game_initialized([] {scheduler::loop([] {
     try {tick();} catch(const std::exception& e) {printf("Community Client: %s\n",e.what());}
   },scheduler::main,1s);},scheduler::main);
 }
};
}
REGISTER_COMPONENT(community_client::component)
