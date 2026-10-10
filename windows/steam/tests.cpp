#include <cstdio>
#include <algorithm>
#include "protocol.hpp"
#include <cassert>
int main(){
 using namespace community::wire;
 auto rewritten=connect("connect \"\\name\\Steam\\xuid\\123\\protocol\\8\"",0xabcdef,"Community");
 auto fields=info(std::string_view(rewritten).substr(9,rewritten.size()-10));
 if(fields["xuid"]!="abcdef"||fields["name"]!="Community"||fields["protocol"]!="8")return 1;
 std::vector<uint8_t> payload(2050,42);auto f=fragments(payload,19);if(f.size()!=3||f.back().size()!=26)return 2;
 uint32_t count,index;uint64_t id;memcpy(&id,f[2].data()+8,8);memcpy(&count,f[2].data()+16,4);memcpy(&index,f[2].data()+20,4);
 if(id!=19||count!=3||index!=2)return 3;
 std::vector<uint8_t> merged;for(auto& p:f)merged.insert(merged.end(),p.begin()+24,p.end());if(merged!=payload)return 4;
 uint8_t raw[]={255,255,255,255,'p'};auto p=packet(raw,5);if(p.offset!=0||p.remote!=4)return 5;
 uint8_t framed[]={0x40,9};p=packet(framed,2);if(p.offset!=1||p.local!=0||p.remote!=4)return 6;
 try{uint8_t bad[]={0x77,9};packet(bad,2);return 7;}catch(const std::runtime_error&){}
 try{connect("connect \"\\name\\truncated",1,"");return 8;}catch(const std::runtime_error&){}
 puts("Wire serialization, fragmentation and native packet framing passed");
}
