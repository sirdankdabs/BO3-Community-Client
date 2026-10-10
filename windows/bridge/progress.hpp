#pragma once
#include <game/game.hpp>
#include <charconv>
#include <map>
// Local Ezz profile only. Uses the same native DDL API as currency.cpp.
namespace community_progress {
using namespace game::ddl;
using namespace game::live::storage;
inline std::optional<uint32_t> amount(const char* s) {
 if(!s || !*s)return {};uint32_t n=0;auto e=s+strlen(s);auto r=std::from_chars(s,e,n);
 if(r.ec!=std::errc{} || r.ptr!=e || n>10000000)return {};return n;
}
inline const StringTable* table(const char* name) {
 return game::db::xasset::DB_FindXAssetHeader(game::db::xasset::XAssetType::STRINGTABLE,name,false,0).stringTable;
}
inline const char* cell(const StringTable* t,int row,int col) {
 if(!t || row<0 || row>=t->rowCount || col<0 || col>=t->columnCount)return nullptr;
 game::symbol<const char*(const StringTable*,int,int)> get{0x142251B40,0x1422AE660,0x0};return get(t,row,col);
}
struct profile {
 game::StorageFileType file;DDLContext* context;
 struct change {DDLState state;uint32_t before,after;};std::vector<change> changes;
 explicit profile(bool zombies) {
  bool online=game::com::Com_SessionMode_GetNetworkMode()==game::eNetworkModes::ONLINE;
  file=zombies ? (online ? game::StorageFileType::ZM_STATS_ONLINE:game::StorageFileType::ZM_STATS_OFFLINE):
   (online ? game::StorageFileType::MP_STATS_ONLINE:game::StorageFileType::MP_STATS_OFFLINE);
  context=Storage_GetDDLContext(game::CONTROLLER_INDEX_0,file,0);
 }
 std::optional<DDLState> find(int item,std::initializer_list<const char*> path) const {
  if(!context || !context->buff || !context->def)return {};
  auto root=Storage_GetDDLRootState(file);if(reinterpret_cast<uintptr_t>(root)<0x1000 || root->ddlDef!=context->def)return {};
  DDLState state=*root,next{};
  for(auto name:path) {
   if(!DDL_MoveToName(&state,&next,name))return {};state=next;
   if(std::string_view(name)=="ItemStats") {if(!DDL_MoveToIndex(&state,&next,item))return {};state=next;}
  }
  if(!state.isValid || !state.member || state.member->arraySize!=1 || (state.member->type!=0 && state.member->type!=2))return {};
  return state;
 }
 bool raise(const DDLState& state,uint32_t target) {
  uint32_t before=DDL_GetUInt(&state,context);if(before>=target)return true;
  if(target>state.member->rangeLimit)return false;changes.push_back({state,before,target});return true;
 }
 bool commit() {
  size_t written=0;bool valid=true;
  for(auto& c:changes) {if(!DDL_SetUInt(&c.state,context,c.after)) {valid=false;break;}++written;if(DDL_GetUInt(&c.state,context)!=c.after) {valid=false;break;}}
  if(valid && written==changes.size() && Storage_Write(game::CONTROLLER_INDEX_0,file,0))return true;
  while(written) {auto& c=changes[--written];DDL_SetUInt(&c.state,context,c.before);}return false;
 }
};
inline bool apply(int mode,int& weapons,int& gums) {
 if(game::ugc::UGC_ActiveMod_Loaded() || game::com::Com_IsInGame())return false;
 const bool zm=mode==0;
 const auto levels=table(zm ? "gamedata/weapons/zm/zm_gunlevels.csv":"gamedata/weapons/mp/mp_gunlevels.csv");
 const auto stats=table(zm ? "gamedata/stats/zm/zm_statsTable.csv":"gamedata/stats/mp/mp_statsTable.csv");
 if(!levels || !stats || levels->columnCount<3 || stats->columnCount<5 || levels->rowCount>20000 || stats->rowCount>20000)return false;
 // Gun levels CSV: col 2 weapon reference, col 1 cumulative XP.
 std::map<std::string,uint32_t> targets;
 for(int row=0;row<levels->rowCount;++row) {
  auto ref=cell(levels,row,2);auto xp=amount(cell(levels,row,1));
  if(ref && *ref && strlen(ref)<128 && xp && *xp)targets[ref]=std::max(targets[ref],*xp);
 }
 profile data(zm);
 for(int row=0;row<stats->rowCount;++row) {
  auto index=amount(cell(stats,row,0));auto ref=cell(stats,row,4);auto type=cell(stats,row,2);
  if(!index || *index>=256 || !ref)continue;
  auto found=targets.find(ref);
  if(found!=targets.end()) {
   auto state=data.find(*index,{"ItemStats","xp"});if(!state || !data.raise(*state,found->second))return false;++weapons;
  }
  if(zm && type && std::string_view(type)=="bubblegum_consumable") {
   auto gained=data.find(*index,{"ItemStats","stats","bgbconsumablesgained","statValue"});
   auto used=data.find(*index,{"ItemStats","stats","bgbconsumablesused","statValue"});
   if(!gained || !used)return false;
   const uint32_t consumed=DDL_GetUInt(&*used,data.context);
   if(consumed>UINT_MAX-255 || !data.raise(*gained,consumed+255))return false;++gums;
  }
 }
 return weapons>0 && data.commit();
}
}
