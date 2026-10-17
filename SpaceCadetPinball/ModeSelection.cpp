#include "pch.h"
#include "ModeSelection.h"
#include "ResourceImport.h"
#include "ResourceCatalog.h"
#include "options.h"
#include "pb.h"
#include "winmain.h"

namespace
{
const char* Setting = "Selected historical mode";
const char* ModeName(bool ft) { return ft ? "Full Tilt" : "Windows"; }
const char* ModeId(bool ft) { return ft ? "full-tilt" : "windows"; }
ResourceInspection available[2];
bool requestSwitch=false, requestedMode=false, wasPaused=false;
std::string failure;

void Refresh()
{
    for(int mode=0;mode<2;++mode)
    {
        const auto& path=ResourceImport::Catalog().Get(mode!=0);
        available[mode]=path.empty()?ResourceInspection{}:ResourceImport::Catalog().InspectRegistered(mode!=0);
        if(available[mode].fullTilt!=(mode!=0))available[mode].valid=false;
    }
}
void Remember(bool ft)
{
    options::SetSetting(Setting,ModeId(ft));
    // Keep compatibility with old settings readers, without using it as a fallback.
    options::Options.Prefer3DPBGameData=!ft;
}
bool Select(bool ft)
{
    if(!ResourceImport::SelectPreferred(!ft))
    {
        failure="The requested resources are unavailable or changed. Restore the folder or import a replacement.";
        Refresh();
        return false;
    }
    Remember(ft);
    return true;
}
void ExplainMissing(int mode)
{
    if(!available[mode].valid)
        ImGui::TextWrapped("%s unavailable. Import its original resource folder, or restore the configured folder.",ModeName(mode!=0));
}
}

bool ModeSelection::Prepare(const std::vector<const char*>& legacySearchPaths)
{
    const std::string selected=options::GetSetting(Setting,"");
    const bool known=selected=="windows" || selected=="full-tilt";
    failure.clear();requestSwitch=false;
    if(known && Select(selected=="full-tilt"))return true;

    // Existing portable installs remain usable. Once remembered, the mode must match:
    // losing Full Tilt resources must never silently start the Windows table.
    if(!ResourceImport::HasImports())
    {
        pb::SelectDatFile(legacySearchPaths);
        if(!pb::DatFileName.empty() && (selected.empty() || (known && selected==ModeId(pb::FullTiltMode))))
        {
            Remember(pb::FullTiltMode);
            return true;
        }
        pb::DatFileName.clear();
    }
    Refresh();
    SDL_ShowWindow(winmain::MainWindow);
    bool running=true, chosen=false;
    while(running && !chosen)
    {
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if(event.type==SDL_QUIT || (event.type==SDL_WINDOWEVENT && event.window.event==SDL_WINDOWEVENT_CLOSE))running=false;
        }
        if(!running)break;
        winmain::UpdateUiScale();
        ImGui_ImplSDL2_NewFrame();ImGui_Render_NewFrame();ImGui::NewFrame();
        bool import=false;
        winmain::ConstrainNextDialog(620,360);
        if(ImGui::Begin("Choose game mode",nullptr,ImGuiWindowFlags_NoCollapse))
        {
            if(!selected.empty())ImGui::TextWrapped("Saved mode: %s. It will not be replaced automatically.",known?ModeName(selected=="full-tilt"):selected.c_str());
            if(!failure.empty())ImGui::TextWrapped("%s",failure.c_str());
            for(int mode=0;mode<2;++mode)
            {
                if(ImGui::MenuItem(ModeName(mode!=0),nullptr,false,available[mode].valid))chosen=Select(mode!=0);
                ExplainMissing(mode);
            }
            ImGui::Separator();
            import=ImGui::Button("Import resource folder...");ImGui::SameLine();
            if(ImGui::Button("Refresh folders"))Refresh();ImGui::SameLine();
            if(ImGui::Button("Quit"))running=false;
        }
        ImGui::End();ImGui::Render();
        SDL_SetRenderDrawColor(winmain::Renderer,0,0,0,255);SDL_RenderClear(winmain::Renderer);
        ImGui_Render_RenderDrawData(ImGui::GetDrawData());SDL_RenderPresent(winmain::Renderer);
        if(import){ResourceImport::RunSetup();Refresh();}
        SDL_Delay(16);
    }
    return chosen;
}

void ModeSelection::DrawMenu()
{
    ImGui::Text("Current mode: %s",ModeName(pb::FullTiltMode));
    // Refresh once when opening the submenu; revalidate again when committing a switch.
    if(ImGui::IsWindowAppearing())Refresh();
    for(int mode=0;mode<2;++mode)
    {
        const bool active=pb::FullTiltMode==(mode!=0);
        if(ImGui::MenuItem(ModeName(mode!=0),nullptr,active,active || available[mode].valid) && !active)
        {
            requestedMode=mode!=0;requestSwitch=true;failure.clear();wasPaused=winmain::single_step;winmain::pause(false);
        }
        if(!active)ExplainMissing(mode);
    }
}

void ModeSelection::DrawConfirmation()
{
    if(requestSwitch){ImGui::OpenPopup("Change game mode?");requestSwitch=false;}
    winmain::ConstrainNextDialog(620,220);
    if(ImGui::BeginPopupModal("Change game mode?",nullptr,ImGuiWindowFlags_None))
    {
        ImGui::Text("Start a new %s game?",ModeName(requestedMode));
        ImGui::TextWrapped("The current game will end. Its unfinished score will not be recorded.");
        ImGui::TextWrapped("Saved high scores are kept separately for each mode.");
        if(!failure.empty())ImGui::TextWrapped("%s",failure.c_str());
        if(ImGui::Button("End current game and switch"))
        {
            // Do not mutate the active table's mode before its scores are saved at teardown.
            const bool activeMode=pb::FullTiltMode, activeDemo=pb::FullTiltDemoMode;
            const auto activePath=pb::BasePath, activeDat=pb::DatFileName;
            const bool selected=Select(requestedMode);
            pb::FullTiltMode=activeMode;pb::FullTiltDemoMode=activeDemo;
            pb::BasePath=activePath;pb::DatFileName=activeDat;
            if(selected){ImGui::CloseCurrentPopup();winmain::Restart();}
        }
        if(ImGui::Button("Keep current game")){ImGui::CloseCurrentPopup();if(!wasPaused)winmain::end_pause();}
        ImGui::EndPopup();
    }
}

const char* ModeSelection::WindowTitle()
{
    return pb::FullTiltMode?"Pinball - Full Tilt":"Pinball - Windows";
}
