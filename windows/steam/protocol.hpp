#pragma once
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
namespace community::wire {
inline std::map<std::string,std::string> info(std::string_view text) {
 std::map<std::string,std::string> result;
 if (!text.empty() && text.front()=='\\') text.remove_prefix(1);
 while (!text.empty()) { auto pos=text.find('\\'); if(pos==text.npos) break;
  std::string key(text.substr(0,pos));text.remove_prefix(pos+1);pos=text.find('\\');
  result[key]=std::string(text.substr(0,pos));if(pos==text.npos)break;text.remove_prefix(pos+1);
 }return result;
}
template<class T> inline void put(std::vector<uint8_t>& out,T value) {
 const auto* bytes=reinterpret_cast<const uint8_t*>(&value);out.insert(out.end(),bytes,bytes+sizeof(T));
}
inline void put_string(std::vector<uint8_t>& out,std::string_view value) {
 if(value.size()>65536)throw std::runtime_error("Protocol field exceeds limit");
 put(out,static_cast<uint32_t>(value.size()));out.insert(out.end(),value.begin(),value.end());
}
inline std::string connect(std::string_view input,uint64_t xuid,const std::string& name) {
 if(input.size()<10||input.size()>2048||!input.starts_with("connect \""))throw std::runtime_error("Invalid connect command");
 input.remove_prefix(9);if(!input.empty()&&input.back()==0)input.remove_suffix(1);
 if(input.empty()||input.back()!='"')throw std::runtime_error("Unterminated connect command");input.remove_suffix(1);
 auto fields=info(input);char hex[17];snprintf(hex,sizeof(hex),"%llx",static_cast<unsigned long long>(xuid));fields["xuid"]=hex;
 if(!name.empty())fields["name"]=name;
 std::string result="connect \"";for(const auto& [k,v]:fields)result+='\\'+k+'\\'+v;
 result+='"';if(result.size()>2048)throw std::runtime_error("Connect command exceeds limit");return result;
}
inline std::vector<std::vector<uint8_t>> fragments(const std::vector<uint8_t>& data,uint64_t id) {
 if(data.empty()||data.size()>65536)throw std::runtime_error("Invalid fragmented payload");
 const auto count=static_cast<uint32_t>((data.size()+1023)/1024);std::vector<std::vector<uint8_t>> result;
 for(uint32_t i=0;i<count;i++){std::vector<uint8_t> p{'c','o','n','n','e','c','t',' '};put(p,id);put(p,count);put(p,i);
  const size_t first=static_cast<size_t>(i)*1024,last=std::min<size_t>(first+1024,data.size());p.insert(p.end(),data.begin()+first,data.begin()+last);result.push_back(std::move(p));}return result;
}
// Native Sys_GetPacket's postconditions, including its already-consumed socket byte.
struct Packet { uint8_t local,remote;size_t offset; };
inline Packet packet(const uint8_t* bytes,size_t length) {
 if(length>=4&&memcmp(bytes,"\xff\xff\xff\xff",4)==0)return {0,4,0};
 if(length<2||(bytes[0]&15)>4||(bytes[0]>>4)>4)throw std::runtime_error("Invalid socket byte");
 return {static_cast<uint8_t>(bytes[0]&15),static_cast<uint8_t>(bytes[0]>>4),1};
}
}
