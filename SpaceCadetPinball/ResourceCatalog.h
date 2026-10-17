#pragma once
#include "ResourceSha256.h"
#include "ResourceAudioFingerprints.h"
#include "ResourceDatValidation.h"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <cctype>
#include <vector>
#include <set>
#include <codecvt>
#include <locale>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace ResourceFiles
{
inline FILE* Open(const std::string& path, const char* mode)
{
#ifdef _WIN32
    try { std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> cv;return _wfopen(cv.from_bytes(path).c_str(),cv.from_bytes(mode).c_str()); } catch(...) { errno=EINVAL;return nullptr; }
#else
    return fopen(path.c_str(),mode);
#endif
}
inline std::string Join(std::string root, const std::string& name)
{
    if(!root.empty() && root.back()!='/' && root.back()!='\\')root+='/';return root+name;
}
inline bool Absolute(const std::string& path)
{
#ifdef _WIN32
    return (path.size()>2 && std::isalpha(static_cast<unsigned char>(path[0])) && path[1]==':' && (path[2]=='/' || path[2]=='\\')) || (path.size()>2 && path[0]=='\\' && path[1]=='\\');
#else
    return !path.empty() && path[0]=='/';
#endif
}
inline bool Read(const std::string& path, std::vector<unsigned char>& data, std::string& error)
{
    FILE* f=Open(path,"rb");if(!f) { error=path+": "+std::strerror(errno);return false; }
    data.clear();unsigned char buffer[8192];size_t count;
    while((count=fread(buffer,1,sizeof buffer,f))!=0) { if(data.size()+count>64*1024*1024) { fclose(f);error=path+": file exceeds 64 MiB limit";return false; }data.insert(data.end(),buffer,buffer+count); }
    bool ok=!ferror(f);if(!ok)error=path+": read failed";fclose(f);return ok;
}
inline std::string Existing(const std::string& root, std::string relative)
{
    // Original distributions use upper/lower case. Preserve the spelling that opens.
    for(int i=0;i<3;++i) { auto path=Join(root,relative);FILE* f=Open(path,"rb");if(f){fclose(f);return relative;}std::transform(relative.begin(),relative.end(),relative.begin(),[i](unsigned char c){return i==0?std::tolower(c):std::toupper(c);}); }
    return relative;
}
inline bool Replace(const std::string& from, const std::string& to)
{
#ifdef _WIN32
    try { std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> cv;return MoveFileExW(cv.from_bytes(from).c_str(),cv.from_bytes(to).c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0; }catch(...){return false;}
#else
    return std::rename(from.c_str(),to.c_str())==0;
#endif
}
}

struct ResourceInspection
{
    bool valid=false;
    bool fullTilt=false;
    bool knownFingerprint=false;
    bool needsManualMode=false;
    std::string directory, datName, fingerprint, description;
    std::vector<std::string> errors, warnings, observations;
};

// The registry stores read-only source locations, never scores or copies of game data.
// Mode index: 0 = Windows, 1 = Full Tilt. Persisted files are UTF-8, one absolute path per line.
class ResourceCatalog
{
    std::string registryPath;
    bool registryReadable=true;
    std::string directories[2];
    std::string fingerprints[2];
    bool manualModes[2]={false,false};
    static void CheckAudio(bool ft, const std::string& relative, const std::vector<unsigned char>& data, ResourceInspection& result)
    {
        auto key=relative;std::transform(key.begin(),key.end(),key.begin(),[](unsigned char c){return std::tolower(c);});
        for(const auto& entry:ResourceAudioFingerprints)if(entry.fullTilt==ft && key==entry.name)
        {
            ResourceSha256 digest;digest.add(data.data(),data.size());
            if(digest.finish()!=entry.sha256)result.warnings.push_back("Audio differs from documented reference: "+relative);
            return;
        }
    }
    static unsigned Read16(const std::vector<unsigned char>& d,size_t p) {return unsigned(d[p])|(unsigned(d[p+1])<<8);}
    static uint32_t Read32(const std::vector<unsigned char>& d,size_t p) {return Read16(d,p)|(uint32_t(Read16(d,p+2))<<16);}
    static bool Sounds(const std::vector<unsigned char>& data, std::set<std::string>& sounds)
    {
        if(data.size()<183 || std::memcmp(data.data(),"PARTOUT(4.0)RESOURCE",19)!=0)return false;
        size_t p=183+Read16(data,181); unsigned groups=Read16(data,175);
        for(unsigned g=0;g<groups;++g)
        {
            if(p>=data.size())return false;unsigned fields=data[p++];bool sound=false;std::string name;
            for(unsigned f=0;f<fields;++f)
            {
                if(p>=data.size())return false;auto kind=data[p++];if(kind>12)return false;
                uint32_t size=2;if(kind!=0 && kind!=2){if(data.size()-p<4)return false;size=Read32(data,p);p+=4;}
                if(size>data.size()-p)return false;
                if(kind==0 && size==2) sound=Read16(data,p)==202;
                if(kind==9) {name.assign(reinterpret_cast<const char*>(data.data()+p),size);while(!name.empty() && name.back()=='\0')name.pop_back();}
                p+=size;
            }
            if(sound && !name.empty() && name!="...") {if(name.find_first_of("/\\:\r\n")!=std::string::npos || name.find('\0')!=std::string::npos || name=="." || name=="..")return false;sounds.insert(name);}
        }
        return true;
    }
public:
    explicit ResourceCatalog(const std::string& path):registryPath(path){}
    const std::string& Get(bool fullTilt) const {return directories[fullTilt?1:0];}
    ResourceInspection InspectRegistered(bool fullTilt) const
    {
        const int slot=fullTilt?1:0;
        auto result=Inspect(directories[slot],manualModes[slot]?slot:-1);
        if(result.valid && ((!fingerprints[slot].empty() && result.fingerprint!=fingerprints[slot]) || result.fullTilt!=fullTilt))
        {
            result.valid=false;
            result.errors.push_back("The registered table changed. Inspect and confirm the folder again.");
        }
        return result;
    }
    bool Load(std::string& error)
    {
        registryReadable=false;
        FILE* f=ResourceFiles::Open(registryPath,"rb");
        if(!f)
        {
            if(errno==ENOENT){registryReadable=true;return true;}
            error="Cannot read resource registry: "+registryPath;return false;
        }
        fclose(f);
        std::vector<unsigned char> bytes;if(!ResourceFiles::Read(registryPath,bytes,error))return false;
        std::string text(bytes.begin(),bytes.end());
        const bool version2=text.compare(0,21,"PINBALL-RESOURCES-v2\n")==0;
        if(!version2 && text.compare(0,21,"PINBALL-RESOURCES-v1\n")!=0)
        {error="Unrecognized resource registry: "+registryPath;return false;}
        size_t position=21;std::vector<std::string> lines;
        while(position<text.size())
        {
            auto end=text.find('\n',position);
            if(end==std::string::npos){error="Incomplete resource registry";return false;}
            lines.push_back(text.substr(position,end-position));position=end+1;
        }
        if(lines.size()!=(version2?6u:2u)){error="Unexpected resource registry content";return false;}
        for(int slot=0;slot<2;++slot)
        {
            if(lines[slot].find_first_of("\r\n")!=std::string::npos || lines[slot].find('\0')!=std::string::npos ||
               (!lines[slot].empty() && !ResourceFiles::Absolute(lines[slot])))
            {error="Invalid resource path in registry";return false;}
            if(version2)
            {
                auto hash=lines[2+slot];auto mode=lines[4+slot];
                if((!hash.empty() && (hash.size()!=64 || hash.find_first_not_of("0123456789abcdef")!=std::string::npos)) ||
                    (mode!="recognized" && mode!="manual") || (mode=="manual" && hash.empty()))
                {error="Invalid resource provenance in registry";return false;}
            }
        }
        for(int slot=0;slot<2;++slot)
        {
            directories[slot]=lines[slot];fingerprints[slot]=version2?lines[2+slot]:"";
            manualModes[slot]=version2 && lines[4+slot]=="manual";
        }
        registryReadable=true;return true;
    }
    static ResourceInspection Inspect(const std::string& directory, int manualMode=-1)
    {
        ResourceInspection result;result.directory=directory;
        if(!ResourceFiles::Absolute(directory) || directory.find_first_of("\r\n")!=std::string::npos || directory.find('\0')!=std::string::npos){result.errors.push_back("Enter an absolute folder path, without quotes or line breaks.");return result;}
        bool found=false;
        for(const auto candidate:{"PINBALL.DAT","CADET.DAT"})
        {
            auto name=ResourceFiles::Existing(directory,candidate);std::vector<unsigned char> data;std::string error;
            if(!ResourceFiles::Read(ResourceFiles::Join(directory,name),data,error)) {result.errors.push_back(error);continue;}
            ResourceSha256 digest;digest.add(data.data(),data.size());auto hash=digest.finish();
            bool windows=hash=="42defd5d2a339a3953fe643716aff4e35b2cea7a3e54505c82d1e78d5325bc04";
            bool ft=hash=="d4b85b0cc0cb05e4be49bf4a5a56e65be8514c854c0f718180845cb07f272ee3";
            bool recognized=windows || ft;
            if(!recognized)
            {
                std::string windowsError,tiltError;
                if(manualMode==0 || manualMode==1)
                {
                    ft=manualMode==1;
                    if(!ResourceDatValidation::Validate(data,ft,error))
                    {result.errors.push_back(name+": "+error);continue;}
                }
                else
                {
                    bool validWindows=ResourceDatValidation::Validate(data,false,windowsError);
                    bool validTilt=ResourceDatValidation::Validate(data,true,tiltError);
                    if(!validWindows && !validTilt)
                    {
                        result.errors.push_back(name+": unrecognized table fingerprint; invalid structure. Windows: "+windowsError+". Full Tilt: "+tiltError);
                        continue;
                    }
                    result.needsManualMode=true;
                }
            }
            if(found){result.valid=false;result.errors.push_back("Both tables are present. Select a separate folder for each game.");return result;}
            found=true;result.fullTilt=ft;result.datName=name;result.fingerprint=hash;result.knownFingerprint=recognized;
            result.description=ft?"Full Tilt table recognized (shared Windows 95 / Windows 3.1 data; exact edition unverified)":"Windows table recognized (matches XP reference data; exact edition unverified)";
            if(!recognized)
            {
                result.description=result.needsManualMode?
                    "Unrecognized table fingerprint. Choose its game mode explicitly; the exact edition and provenance remain unverified.":
                    std::string("Unrecognized table; ")+(ft?"Full Tilt":"Windows")+" mode confirmed manually. Exact edition and provenance remain unverified.";
                if(result.needsManualMode)continue;
            }
            std::set<std::string> sounds;if(!Sounds(data,sounds)){result.errors.push_back(name+": invalid resource structure");return result;}
            for(const auto& sound:sounds)
            {
                auto file=ResourceFiles::Existing(directory,(ft?"SOUND/":"")+sound);std::vector<unsigned char> content;
                if(!ResourceFiles::Read(ResourceFiles::Join(directory,file),content,error))
                {
                    auto lower=sound;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return std::tolower(c);});
                    bool referenceAbsence=lower=="sound2.wav" || lower=="sound37.wav" || lower=="sound44.wav" || lower=="sound59.wav" || lower=="sound62.wav" || lower==(ft?"sound111.wav":"sound52.wav");
                    if(recognized && referenceAbsence)result.observations.push_back("Also absent from documented reference: "+file);
                    else result.warnings.push_back("Sound unavailable: "+error);
                }
                else if(recognized)CheckAudio(ft,(ft?"sound/":"")+sound,content,result);
            }
            std::vector<std::string> tracks=ft?std::vector<std::string>{"SOUND/TABA1","SOUND/TABA2","SOUND/TABA3"}:std::vector<std::string>{"PINBALL"};
            for(const auto& track:tracks){bool readable=false;for(const auto ext:{".MID",".MDS"}){std::vector<unsigned char> content;if(ResourceFiles::Read(ResourceFiles::Join(directory,ResourceFiles::Existing(directory,track+ext)),content,error)){if(recognized)CheckAudio(ft,track+ext,content,result);readable=true;break;}}if(!readable)result.warnings.push_back("Music unavailable: "+track+".MID or .MDS");}
        }
        if(found){result.errors.clear();result.valid=!result.needsManualMode;}return result;
    }
    bool Register(const ResourceInspection& inspected, std::string& error)
    {
        // Revalidate at commit time: failed import must leave both previous entries intact.
        if(!registryReadable && !Load(error))return false;
        auto current=Inspect(inspected.directory,inspected.knownFingerprint?-1:(inspected.fullTilt?1:0));if(!inspected.valid || !current.valid || current.fullTilt!=inspected.fullTilt || current.fingerprint!=inspected.fingerprint){error="Resources changed or are invalid. Inspect the folder again.";return false;}
        std::string next[2]={directories[0],directories[1]};next[current.fullTilt?1:0]=current.directory;
        std::string nextFingerprints[2]={fingerprints[0],fingerprints[1]};
        bool nextManual[2]={manualModes[0],manualModes[1]};
        nextFingerprints[current.fullTilt?1:0]=current.fingerprint;nextManual[current.fullTilt?1:0]=!current.knownFingerprint;
        auto temporary=registryPath+".tmp";FILE* f=ResourceFiles::Open(temporary,"wb");if(!f){error="Cannot save resource configuration: "+registryPath;return false;}
        auto text=std::string("PINBALL-RESOURCES-v2\n")+next[0]+"\n"+next[1]+"\n"+
            nextFingerprints[0]+"\n"+nextFingerprints[1]+"\n"+(nextManual[0]?"manual":"recognized")+"\n"+(nextManual[1]?"manual":"recognized")+"\n";
        bool ok=fwrite(text.data(),1,text.size(),f)==text.size();if(fflush(f)!=0)ok=false;if(fclose(f)!=0)ok=false;
        if(!ok || !ResourceFiles::Replace(temporary,registryPath)){error="Cannot replace resource configuration; previous imports retained.";return false;}
        for(int slot=0;slot<2;++slot){directories[slot]=next[slot];fingerprints[slot]=nextFingerprints[slot];manualModes[slot]=nextManual[slot];}
        registryReadable=true;return true;
    }
};
