#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <cmath>

// Bounded validation of the PARTOUT container and allocations made by partman.
// This validates format compatibility, not the provenance or gameplay of a variant.
class ResourceDatValidation
{
    struct Field { unsigned kind; size_t offset, size; };
    struct Group { std::string name; std::vector<Field> fields; };
    static unsigned U16(const std::vector<unsigned char>& d, size_t p)
    { return unsigned(d[p]) | (unsigned(d[p+1]) << 8); }
    static int S16(const std::vector<unsigned char>& d, size_t p)
    { return static_cast<int16_t>(U16(d,p)); }
    static uint32_t U32(const std::vector<unsigned char>& d, size_t p)
    { return U16(d,p) | (uint32_t(U16(d,p+2)) << 16); }
    static const Field* Find(const Group& group, unsigned kind)
    { for(const auto& field:group.fields)if(field.kind==kind)return &field;return nullptr; }
    static bool Fail(std::string& error, const std::string& message)
    { error=message;return false; }
    static bool Bitmap(const std::vector<unsigned char>& d, size_t p, size_t size,
                       bool ft, unsigned& resolution, int& width, int& height,
                       uint64_t& allocation, std::string& error)
    {
        if(size<14)return Fail(error,"truncated bitmap header");
        resolution=d[p];width=S16(d,p+1);height=S16(d,p+3);
        auto bytes=U32(d,p+9);auto flags=d[p+13];
        if(resolution>2 || (!ft && resolution!=0) || width<=0 || height<=0 || width>4096 || height>4096)
            return Fail(error,"invalid bitmap dimensions or resolution for selected mode");
        if(bytes!=size-14 || (flags&~7)!=0)return Fail(error,"invalid bitmap byte count or flags");
        allocation+=uint64_t(width)*height*7+bytes;
        if((flags&4)==0)
        {
            if(uint64_t((width+3)&~3)*height!=bytes)return Fail(error,"bitmap payload does not match dimensions");
            return true;
        }
        // Mirrors SplitSplicedBitmap, checking both source and destination limits.
        static const int tableWidths[3]={600,752,960};
        size_t cursor=p+14,end=p+size;uint64_t destination=0;
        while(cursor<end)
        {
            if(end-cursor<2)return Fail(error,"truncated spliced bitmap stride");
            int stride=S16(d,cursor);cursor+=2;
            if(stride<0)return cursor==end || Fail(error,"trailing spliced bitmap bytes");
            if(stride>width)stride+=width-tableWidths[resolution];
            if(stride<0 || end-cursor<2)return Fail(error,"invalid spliced bitmap stride");
            auto count=U16(d,cursor);cursor+=2;destination+=unsigned(stride);
            if(destination+count>uint64_t(width)*height || uint64_t(count)*3>end-cursor)
                return Fail(error,"spliced bitmap exceeds source or destination");
            destination+=count;cursor+=size_t(count)*3;
        }
        return Fail(error,"unterminated spliced bitmap");
    }
public:
    static bool Validate(const std::vector<unsigned char>& data, bool fullTilt, std::string& error)
    {
        if(data.size()<183 || data.size()>64*1024*1024 ||
           std::memcmp(data.data(),"PARTOUT(4.0)RESOURCE\0",20)!=0)
            return Fail(error,"unknown or truncated PARTOUT header");
        if(!std::memchr(data.data()+21,0,50) || !std::memchr(data.data()+71,0,100))
            return Fail(error,"unterminated application or description text");
        auto count=U16(data,175);size_t cursor=183+U16(data,181);
        if(count==0 || count>4096 || cursor>data.size())return Fail(error,"invalid group count or header extension");
        std::vector<Group> groups;groups.reserve(count);std::map<std::string,size_t> names;
        uint64_t allocation=0;
        for(unsigned index=0;index<count;++index)
        {
            if(cursor>=data.size())return Fail(error,"truncated group");
            unsigned fields=data[cursor++];Group group;
            int bitmapWidth[3]={},bitmapHeight[3]={},depthWidth[3]={},depthHeight[3]={};
            bool bitmapPresent[3]={},depthPresent[3]={};unsigned previousKind=0;
            for(unsigned i=0;i<fields;++i)
            {
                if(cursor>=data.size())return Fail(error,"truncated field type");
                unsigned kind=data[cursor++];if(kind>12 || (i && kind<previousKind))return Fail(error,"invalid or unsorted field type");previousKind=kind;
                size_t size=2;
                if(kind!=0 && kind!=2){if(data.size()-cursor<4)return Fail(error,"truncated field length");size=U32(data,cursor);cursor+=4;}
                if(size>data.size()-cursor)return Fail(error,"field exceeds file size");
                size_t p=cursor;cursor+=size;group.fields.push_back({kind,p,size});allocation+=size;
                if(kind==3 || kind==9)
                {
                    if(!size || data[p+size-1]!=0)return Fail(error,"unterminated string field");
                    if(kind==3){group.name.assign(reinterpret_cast<const char*>(data.data()+p));if(group.name.empty() || names.count(group.name))return Fail(error,"empty or duplicate group name");}
                }
                if(kind==5 && size!=1024)return Fail(error,"palette must contain 256 RGBA colors");
                if((kind==10 && size%2) || (kind==11 && size%4))return Fail(error,"misaligned numeric array");
                if(kind==11)for(size_t at=p;at<p+size;at+=4){uint32_t bits=U32(data,at);float number;std::memcpy(&number,&bits,4);if(!std::isfinite(number))return Fail(error,"non-finite table number");}
                if(kind==1)
                {
                    unsigned res;int width,height;
                    if(!Bitmap(data,p,size,fullTilt,res,width,height,allocation,error))return false;
                    if(bitmapPresent[res])return Fail(error,"duplicate bitmap resolution");
                    bitmapPresent[res]=true;bitmapWidth[res]=width;bitmapHeight[res]=height;
                    if(data[p+13]&4){depthPresent[res]=true;depthWidth[res]=width;depthHeight[res]=height;}
                }
                if(kind==12)
                {
                    size_t prefix=fullTilt?1:0;if(size<14+prefix)return Fail(error,"truncated depth-map header");
                    unsigned res=fullTilt?data[p++]:0;if(res==255)res=0;
                    int width=S16(data,p),height=S16(data,p+2),stride=S16(data,p+4);
                    if(res>2 || width<0 || height<0 || width>4096 || height>4096 || stride<width || stride>4096)
                        return Fail(error,"invalid depth-map dimensions or resolution for selected mode");
                    auto bytes=size-14-prefix;
                    // The Windows reference contains two zero-header depth maps, deliberately skipped by partman.
                    if(uint64_t(stride)*height*2!=bytes && !(width==0 && height==0 && stride==0 && !fullTilt))
                        return Fail(error,"depth-map payload does not match dimensions or selected mode");
                    if(depthPresent[res])return Fail(error,"duplicate depth-map resolution");
                    depthPresent[res]=true;depthWidth[res]=width;depthHeight[res]=height;
                    allocation+=uint64_t(stride)*height*2;
                }
                if(allocation>256*1024*1024)return Fail(error,"table exceeds decoded resource budget");
            }
            for(unsigned res=0;res<3;++res)if(bitmapPresent[res] && depthPresent[res] &&
                (bitmapWidth[res]!=depthWidth[res] || bitmapHeight[res]!=depthHeight[res]))
                return Fail(error,"bitmap and depth-map dimensions differ");
            if(!group.name.empty())names[group.name]=groups.size();groups.push_back(group);
        }
        if(cursor!=data.size())return Fail(error,"trailing bytes after declared groups");
        for(const auto name:{"background","camera_info","table","table_objects","score1","ballcount1","player_number1"})
            if(!names.count(name))return Fail(error,std::string("required game group missing: ")+name);
        if(!names.count(fullTilt?"ball0":"ball"))return Fail(error,"ball group missing for selected mode");
        // Required component tags declared by control.cpp. Unknown tables still need
        // the component names used by this game's rules, regardless of provenance.
        static const char* requiredTags[] = {
            "attack_bumpers", "launch_bumpers", "v_bloc1", "a_bump1", "a_bump2", "a_bump3", "a_bump4", "a_bump5",
            "a_bump6", "a_bump7", "drain", "a_flag1", "a_flag2", "a_flip1", "a_flip2", "fuel_bargraph",
            "v_gate1", "v_gate2", "info_text_box", "a_kick1", "a_kick2", "a_kout1", "a_kout2", "a_kout3",
            "lite1", "lite2", "lite3", "lite4", "lite5", "lite6", "lite7", "lite8",
            "lite9", "lite10", "lite11", "lite12", "lite13", "lite16", "lite17", "lite18",
            "lite19", "lite20", "lite21", "lite22", "lite23", "lite24", "lite25", "lite26",
            "lite27", "lite28", "lite29", "lite30", "lite38", "lite39", "lite40", "lite54",
            "lite55", "lite56", "lite58", "lite59", "lite60", "lite61", "lite62", "lite67",
            "lite68", "lite69", "lite70", "lite71", "lite72", "lite77", "lite84", "lite85",
            "lite101", "lite102", "lite103", "lite104", "lite105", "lite106", "lite107", "lite108",
            "lite109", "lite110", "lite130", "lite131", "lite132", "lite133", "lite169", "lite170",
            "lite171", "lite195", "lite196", "lite198", "lite199", "lite200", "lite300", "lite301",
            "lite302", "lite303", "lite304", "lite305", "lite306", "lite307", "lite308", "lite309",
            "lite310", "lite311", "lite312", "lite313", "lite314", "lite315", "lite316", "lite317",
            "lite318", "lite319", "lite320", "lite321", "lite322", "literoll179", "literoll180", "literoll181",
            "literoll182", "literoll183", "literoll184", "middle_circle", "lchute_tgt_lights", "l_trek_lights", "goal_lights", "hyperspace_lights",
            "bmpr_inc_lights", "bpr_solotgt_lights", "bsink_arrow_lights", "bumper_target_lights", "outer_circle", "r_trek_lights", "ramp_bmpr_inc_lights", "ramp_tgt_lights",
            "skill_shot_lights", "top_circle_tgt_lights", "top_target_lights", "worm_hole_lights", "mission_text_box", "s_onewy1", "s_onewy4", "s_onewy10",
            "plunger", "ramp_hole", "ramp", "v_rebo1", "v_rebo2", "v_rebo3", "v_rebo4", "a_roll1",
            "a_roll2", "a_roll3", "a_roll4", "a_roll5", "a_roll6", "a_roll7", "a_roll8", "a_roll9",
            "a_roll110", "a_roll111", "a_roll112", "a_roll179", "a_roll180", "a_roll181", "a_roll182", "a_roll183",
            "a_roll184", "v_sink1", "v_sink2", "v_sink3", "v_sink7", "soundwave3", "soundwave7", "soundwave8",
            "soundwave9", "soundwave10", "soundwave14", "soundwave14", "soundwave21", "soundwave23", "soundwave24", "soundwave25",
            "soundwave26", "soundwave27", "soundwave28", "soundwave30", "soundwave35", "soundwave35", "soundwave36", "soundwave36",
            "soundwave38", "soundwave39", "soundwave40", "soundwave41", "soundwave44", "soundwave45", "soundwave46", "soundwave47",
            "soundwave48", "soundwave49D", "soundwave50", "soundwave50", "soundwave52", "soundwave59", "a_targ1", "a_targ2",
            "a_targ3", "a_targ4", "a_targ5", "a_targ6", "a_targ7", "a_targ8", "a_targ9", "a_targ10",
            "a_targ11", "a_targ12", "a_targ13", "a_targ14", "a_targ15", "a_targ16", "a_targ17", "a_targ18",
            "a_targ19", "a_targ20", "a_targ21", "a_targ22", "s_trip1", "s_trip2", "s_trip3", "s_trip4",
            "s_trip5",
        };
        for(const auto tag:requiredTags)if(!names.count(tag))
            return Fail(error,std::string("required game component missing: ")+tag);
        auto background=groups[names["background"]];
        if(!Find(background,1) || !Find(background,5))return Fail(error,"background bitmap or palette missing");
        auto cameraIndex=names["camera_info"];
        for(unsigned resolution=0;resolution<(fullTilt?3u:1u);++resolution)
        {
            if(cameraIndex+resolution>=groups.size())return Fail(error,"camera group missing");
            auto camera=Find(groups[cameraIndex+resolution],11);
            if(!camera || camera->size<60)return Fail(error,"camera projection is incomplete");
        }
        const auto objects=Find(groups[names["table_objects"]],10);
        if(!objects || objects->size<6 || objects->size%4!=2 || U16(data,objects->offset)!=1025)
            return Fail(error,"table object references are incomplete");
        for(size_t at=objects->offset+4;at<objects->offset+objects->size;at+=4)
            if(U16(data,at)>=groups.size())return Fail(error,"table object reference exceeds group count");
        if(fullTilt && !names.count("pbmsg_ft"))return Fail(error,"Full Tilt font group missing");
        if(!fullTilt && names.count("pbmsg_ft"))return Fail(error,"Full Tilt font is incompatible with Windows mode");
        return true;
    }
};
