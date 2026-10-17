// Link the real options, catalog and selection implementation. No game window is opened.
#define main unused_game_main
#include "../SpaceCadetPinball/SpaceCadetPinball.cpp"
#undef main
#include "../SpaceCadetPinball/ModeSelection.cpp"

int main(int argc,char** argv)
{
    assert(argc==3);
    const std::string root=argv[1], stage=argv[2];
    const auto ini=ResourceFiles::Join(root,"mode.ini");
    ImGui::CreateContext();winmain::ImIO=&ImGui::GetIO();ImGui::GetIO().IniFilename=ini.c_str();
    options::InitPrimary();ResourceImport::Initialize(root);
    std::string error;
    auto& catalog=ResourceImport::Catalog();
    if(stage=="full-tilt-only")
    {
        assert(catalog.Register(ResourceCatalog::Inspect(ResourceFiles::Join(root,"full-tilt")),error));
        Refresh();assert(!available[0].valid && available[1].valid);
        assert(!Select(false));assert(Select(true));assert(pb::FullTiltMode);
    }
    else if(stage=="windows-only")
    {
        assert(catalog.Register(ResourceCatalog::Inspect(ResourceFiles::Join(root,"windows")),error));
        Refresh();assert(available[0].valid && !available[1].valid);
        assert(!Select(true)); // Windows must not substitute for Full Tilt.
        assert(options::GetSetting(Setting,"").empty());
        assert(Select(false));assert(!pb::FullTiltMode);
        assert(options::GetSetting(Setting,"")=="windows");
    }
    else if(stage=="two-sources")
    {
        assert(options::GetSetting(Setting,"")=="windows");
        assert(catalog.Register(ResourceCatalog::Inspect(ResourceFiles::Join(root,"full-tilt")),error));
        Refresh();assert(available[0].valid && available[1].valid);
        assert(ModeSelection::Prepare({}));assert(!pb::FullTiltMode);
        assert(Select(true));assert(pb::FullTiltMode);
        assert(options::GetSetting(Setting,"")=="full-tilt");
    }
    else if(stage=="restart")
    {
        assert(options::GetSetting(Setting,"")=="full-tilt");
        assert(ModeSelection::Prepare({}));assert(pb::FullTiltMode);
    }
    else
    {
        assert(stage=="missing");
        assert(options::GetSetting(Setting,"")=="full-tilt");
        const auto dat=ResourceFiles::Join(root,"full-tilt/CADET.DAT");
        assert(std::rename(dat.c_str(),(dat+".unavailable").c_str())==0);
        Refresh();assert(available[0].valid && !available[1].valid);
        pb::DatFileName="unchanged";
        assert(!Select(true));
        assert(pb::DatFileName=="unchanged");
        assert(options::GetSetting(Setting,"")=="full-tilt");
        // A different choice only takes effect after an explicit selection.
        assert(Select(false));assert(!pb::FullTiltMode);
        assert(options::GetSetting(Setting,"")=="windows");
    }
    options::uninit();ImGui::DestroyContext();
    std::cout<<"Mode selection integration passed: "<<stage<<'\n';
}
