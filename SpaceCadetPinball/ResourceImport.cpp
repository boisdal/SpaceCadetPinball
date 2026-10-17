#include "pch.h"
#include "ResourceImport.h"
#include "ResourceCatalog.h"
#include "winmain.h"
#include "pb.h"
#include <memory>

namespace
{
std::unique_ptr<ResourceCatalog> catalog;
bool showImport=false, registered=false;
char folder[4096]{};
ResourceInspection inspected;
std::string diagnostic;

void DrawDialog(bool initial)
{
    ImGui::SetNextWindowSize(ImVec2(700,480),ImGuiCond_FirstUseEver);
    if(ImGui::Begin(initial?"Configure game resources":"Import game resources",initial?nullptr:&showImport))
    {
        ImGui::TextWrapped("Choose the folder containing PINBALL.DAT (Windows) or CADET.DAT (Full Tilt). Paste its full path below.");
        ImGui::TextWrapped("The game remembers this location. Keep the folder available. Source files and high scores are never modified by importing. No game data is downloaded.");
        ImGui::Separator();
        ImGui::TextWrapped("Windows: %s",catalog->Get(false).empty()?"not configured":catalog->Get(false).c_str());
        ImGui::TextWrapped("Full Tilt: %s",catalog->Get(true).empty()?"not configured":catalog->Get(true).c_str());
        ImGui::SetNextItemWidth(-1);
        if(ImGui::InputText("##resource-folder",folder,sizeof folder))inspected=ResourceInspection{};
        if(ImGui::Button("Inspect folder")) {inspected=ResourceCatalog::Inspect(folder);diagnostic.clear();}
        ImGui::SameLine();
        if(ImGui::Button(initial?"Cancel and quit":"Cancel"))showImport=false;
        if(!diagnostic.empty())ImGui::TextWrapped("%s",diagnostic.c_str());
        if(!inspected.description.empty())ImGui::TextWrapped("%s",inspected.description.c_str());
        for(const auto& error:inspected.errors)ImGui::TextWrapped("%s",error.c_str());
        if(inspected.valid)
        {
            if(!inspected.warnings.empty())ImGui::TextWrapped("Some audio files are missing or unreadable. You can continue without those sounds, or fix the folder and inspect again.");
            if(ImGui::Button(inspected.warnings.empty()?"Remember this folder":"Remember folder with audio warnings"))
            {
                if(catalog->Register(inspected,diagnostic)){registered=true;showImport=false;}
            }
        }
        if(ImGui::BeginChild("resource-diagnostics",ImVec2(0,0),true))
        {
            for(const auto& warning:inspected.warnings)ImGui::TextWrapped("%s",warning.c_str());
            for(const auto& observation:inspected.observations)ImGui::TextWrapped("%s",observation.c_str());
        }
        ImGui::EndChild();
    }
    ImGui::End();
}
}

void ResourceImport::Initialize(const std::string& preferencesDirectory)
{
    auto registry=ResourceFiles::Join(preferencesDirectory,"resources-v1.txt");
#ifdef PINBALL_CADENCE_AUDIT
    if(const char* isolated=std::getenv("PINBALL_AUDIT_RESOURCE_REGISTRY"))registry=isolated;
#endif
    catalog.reset(new ResourceCatalog(registry));
    diagnostic.clear();catalog->Load(diagnostic);
}
ResourceCatalog& ResourceImport::Catalog(){return *catalog;}
bool ResourceImport::HasImports(){return !catalog->Get(false).empty() || !catalog->Get(true).empty();}
bool ResourceImport::SelectPreferred(bool preferWindows)
{
    bool mode=!preferWindows;
    if(catalog->Get(mode).empty())return false;
    auto data=ResourceCatalog::Inspect(catalog->Get(mode));
    if(!data.valid || data.fullTilt!=mode)
    {
        diagnostic="The configured folder is unavailable or changed. Restore it or import a replacement.\n";
        for(const auto& error:data.errors)diagnostic+=error+"\n";
        return false;
    }
    pb::BasePath=ResourceFiles::Join(data.directory,"");pb::DatFileName=data.datName;
    pb::FullTiltMode=mode;pb::FullTiltDemoMode=false;
    for(const auto& warning:data.warnings)printf("Resource warning: %s\n",warning.c_str());
    return true;
}
void ResourceImport::Open(){showImport=true;registered=false;inspected=ResourceInspection{};diagnostic.clear();}
void ResourceImport::Draw(){if(showImport)DrawDialog(false);}
bool ResourceImport::RunSetup()
{
    showImport=true;registered=false;SDL_ShowWindow(winmain::MainWindow);
    while(showImport)
    {
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if(event.type==SDL_QUIT || (event.type==SDL_WINDOWEVENT && event.window.event==SDL_WINDOWEVENT_CLOSE)){showImport=false;break;}
        }
        if(!showImport)break;
        ImGui_ImplSDL2_NewFrame();ImGui_Render_NewFrame();ImGui::NewFrame();
        DrawDialog(true);ImGui::Render();
        SDL_SetRenderDrawColor(winmain::Renderer,0,0,0,255);SDL_RenderClear(winmain::Renderer);
        ImGui_Render_RenderDrawData(ImGui::GetDrawData());SDL_RenderPresent(winmain::Renderer);SDL_Delay(16);
    }
    return registered;
}
