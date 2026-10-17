#include "../SpaceCadetPinball/ResourceCatalog.h"
#include <iostream>
#include <cstdlib>
static void check(bool condition,const char* message){if(!condition){std::cerr<<message<<"\n";std::exit(1);}}
int main(int argc,char** argv)
{
    ResourceSha256 empty;check(empty.finish()=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855","empty SHA-256");
    ResourceSha256 abc;abc.add(reinterpret_cast<const unsigned char*>("abc"),3);check(abc.finish()=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","abc SHA-256");
    ResourceSha256 million;unsigned char a='a';for(int i=0;i<1000000;++i)million.add(&a,1);check(million.finish()=="cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0","streaming SHA-256");
    if(argc==3 && std::string(argv[1])=="inspect") {auto r=ResourceCatalog::Inspect(argv[2]);std::cout<<r.valid<<" "<<r.fullTilt<<" "<<r.warnings.size()<<"\n";for(auto& e:r.errors)std::cout<<e<<"\n";for(auto& e:r.warnings)std::cout<<e<<"\n";return r.valid?0:2;}
    check(argc==4,"expected Windows folder, Full Tilt folder, fresh registry file");
    auto xp=ResourceCatalog::Inspect(argv[1]);auto ft=ResourceCatalog::Inspect(argv[2]);
    check(xp.valid && !xp.fullTilt,"Windows identification");check(ft.valid && ft.fullTilt,"Full Tilt identification");
    check(xp.warnings.empty(),"Windows reference audio missing");check(ft.warnings.empty(),"Full Tilt reference audio missing");
    check(!ResourceCatalog::Inspect("relative/path").valid,"relative path accepted");
    ResourceCatalog catalog(argv[3]);std::string error;check(catalog.Load(error),"empty registry failed");
    check(catalog.Register(xp,error),"Windows registration failed");check(catalog.Register(ft,error),"FT registration failed");
    check(catalog.Get(false)==argv[1] && catalog.Get(true)==argv[2],"second import lost first");
    check(catalog.Register(xp,error),"reimport failed");
    ResourceCatalog restored(argv[3]);check(restored.Load(error),"registry reload failed");check(restored.Get(false)==argv[1] && restored.Get(true)==argv[2],"persistent mode paths incorrect");
    auto bad=xp;bad.directory="relative";check(!restored.Register(bad,error),"invalid registration succeeded");check(restored.Get(false)==argv[1] && restored.Get(true)==argv[2],"invalid registration lost imports");
    ResourceCatalog unwritable(ResourceFiles::Join(argv[3],"nonexistent/registry"));check(!unwritable.Register(xp,error),"save failure ignored");check(unwritable.Get(false).empty(),"failed save mutated memory");
    auto unicodePath=std::string(argv[3])+u8"-\u00e9.txt";
    ResourceCatalog unicode(unicodePath);check(unicode.Register(xp,error),"UTF-8 registry save failed");ResourceCatalog unicodeReload(unicodePath);check(unicodeReload.Load(error) && unicodeReload.Get(false)==argv[1],"UTF-8 registry reload failed");
    auto malformedPath=std::string(argv[3])+"-malformed";FILE* damaged=ResourceFiles::Open(malformedPath,"wb");check(damaged!=nullptr,"cannot create test registry");fputs("damaged",damaged);fclose(damaged);
    ResourceCatalog malformed(malformedPath);check(!malformed.Load(error),"damaged registry accepted");check(!malformed.Register(xp,error),"damaged registry overwritten");
    std::cout<<"Resource catalog tests passed\n";
}
