#include <Windows.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_opengl2.h"
#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include <SDL_mixer.h>

#include "nfd.h"

#include <vector>
#include <string>
#include <unordered_map>

#include "Media.h"


// TODO:
// hotkey with mods
// Media dublicates

enum MediaColumnID {
    MediaColumnID_Name,
    MediaColumnID_Duration,
    MediaColumnID_Hotkey
};

const char* MODS1[] = { "", "SHIFT", "CTRL", "ALT", "SHIFT+CTRL", "SHIFT+ALT", "ALT+CTRL", "SHIFT+ALT+CTRL" };
const char* KEYS1[] = { "", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "Numpad 1", "Numpad 2", "Numpad 3", "Numpad 4", "Numpad 5", "Numpad 6", "Numpad 7", "Numpad 8", "Numpad 9", "Numpad 0",
                            "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z" };


class Application {
public:
    Application() {}
    ~Application() {}
    
    int Init();
    void Run();
    void Shutdown();

    void HandleInput();

    void DrawUI();

private:
    PlayList plist;
    MediaPlayer player;

    SDL_Window* window;
    SDL_GLContext gl_context;
    ImGuiIO* io = nullptr;
    bool p_open = true;
    bool isRunning = true;

    int masterVolume = 100;
    std::string currentMediaName = "";
};

int Application::Init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("SDL Init Error: %s\n", SDL_GetError());
        return -1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0) {
        printf("Mix OpenAudio error: %s\n", Mix_GetError());
        SDL_Quit();
        return -1;
    }

    NFD_Init();

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
    window = SDL_CreateWindow("AudioPad", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 540, window_flags);
    if (window == nullptr) {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }

    gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();

    ImGuiStyle& img_style = ImGui::GetStyle();
    img_style.FrameRounding = 4;
    //img_style.FramePadding.y = 5;

    plist.player = &player;
    return 0;
}

void Application::Run() {
    bool p_open = true;
    io = &ImGui::GetIO();

    char dir0[256] = "sounds directory...";

    while (isRunning) {
        HandleInput();

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(16);
            continue;
        }
        
        player.Update();

        DrawUI();

        glViewport(0, 0, (int)io->DisplaySize.x, (int)io->DisplaySize.y);
        glClearColor(0.7, 0.3, 1.0, 1.0); // Background Color
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
}

void Application::HandleInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            isRunning = false;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
            isRunning = false;
        if (event.key.keysym.sym == SDLK_SPACE && event.type == SDL_KEYDOWN) {
            player.FlipPlayPause();
        }
        if (event.type == SDL_KEYDOWN) {
            int keycode = event.key.keysym.sym;
            int mod = 0;//= SDL_GetModState();
            printf("key pressed: %i, mod: %i\n", keycode, mod);
            plist.PlayByHotkey(keycode, mod);
        }

        /*
        if ((event.key.keysym.sym >= SDLK_1 || event.key.keysym.sym <= SDLK_9) && event.type == SDL_KEYDOWN) {
            printf("key pressed: %i\n", event.key.keysym.sym - SDLK_0);
            plist.PlayByHotkey(event.key.keysym.sym - SDLK_0, 0);
            currentMediaName = plist.currentMediaName;
        }
        if ((event.key.keysym.sym >= SDLK_a || event.key.keysym.sym <= SDLK_z) && event.type == SDL_KEYDOWN) {
            printf("key pressed: %i\n", event.key.keysym.sym - SDLK_a);
            plist.PlayByHotkey(event.key.keysym.sym, 0);
            currentMediaName = plist.currentMediaName;
        }
        if ((event.key.keysym.sym >= SDLK_KP_1 || event.key.keysym.sym <= SDLK_KP_0) && event.type == SDL_KEYDOWN) {
            printf("key pressed: %i\n", event.key.keysym.sym - SDLK_0);
            plist.PlayByHotkey(event.key.keysym.sym - SDLK_0, 0);
            currentMediaName = plist.currentMediaName;
        }
        */
    }
}

void Application::DrawUI() {
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    static bool use_work_area = true;
    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
    ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);

    ImGui::Begin("AudioPadPlayer", &p_open, flags);

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("AudioPadTabBar", tab_bar_flags))
    
    //
    // Construction of ui under label "PlayList"
    //
    if (ImGui::BeginTabItem("PlayList")) {
        ImGui::Text("Select Sounds Folder:");
        ImGui::InputText("##", plist.playListDirC, IM_ARRAYSIZE(plist.playListDirC), ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        if (ImGui::Button("...")) {
            plist.LoadPlayList();
        }

        //
        // Media Player interaction elements
        //
        ImGui::SeparatorText("Player");

        ImGui::PushItemWidth(60);
        if (ImGui::Button("Play")) {
            if (plist.activeMedia == nullptr) {
                printf("Nothing to play\n");
            }
            else {
                if (player.playingMedia != plist.activeMedia->file) {
                    player.PlayNewMedia(plist.activeMedia->file);
                }
                else {
                    player.Play();
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Pause")) {
            player.Pause();
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
            player.Stop();
        }
        ImGui::SameLine();
        ImGui::PopItemWidth();

        char timestr[64] = { 0 };
        int playingMediaTime = 0;
        int minlength = 0;
        int medialength = 0;

        if (plist.activeMedia) {
            playingMediaTime = player.GetPlayingMediaTime();
            medialength = plist.activeMedia->length;
        }

        sprintf_s(timestr, "%i:%02i/%i:%02i\0", playingMediaTime / 60, playingMediaTime % 60, medialength / 60, medialength % 60);
        ImGui::Text(timestr); ImGui::SameLine();

        ImGui::PushItemWidth(-180);
        if (ImGui::SliderInt("##2", &playingMediaTime, minlength, medialength, "")) {
            player.RewindTo(playingMediaTime);
        }

        ImGui::PopItemWidth(); ImGui::SameLine();
        ImGui::PushItemWidth(120);
        if (ImGui::SliderInt("Volume", &masterVolume, 0, 100)) {
            Mix_VolumeMusic((masterVolume * MIX_MAX_VOLUME) / 100);
        }
        ImGui::PopItemWidth();

        ImGui::Text("Selected media: "); ImGui::SameLine();
        ImGui::Text(currentMediaName.c_str());
        ImGui::Text("");
        static int item_current_2 = -1;
        static int item_current_3 = -1;
        static int item_current_4 = -1;
        static int item_current_5 = -1;
        /*
        ImGui::SeparatorText("Hot Key Combo");

        // TODO Clean up
        ImGui::PushItemWidth(60);
        ImGui::Combo("Mod", &item_current_2, MODS1, IM_ARRAYSIZE(MODS1)); ImGui::SameLine();
        ImGui::Combo("Key", &item_current_3, KEYS1, IM_ARRAYSIZE(KEYS1));
        ImGui::PopItemWidth();
        */
        //
        // Playlist section
        //
        ImGui::SeparatorText("Audio List");

        static ImGuiTableFlags tableFlags =
            ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
            | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody
            | ImGuiTableFlags_ScrollY;

        ImGui::BeginTable("Audio List", 4, tableFlags, ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 15), 0.0f);

        //ImGuiTableColumnFlags_DefaultSort
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40.0f, MediaColumnID_Name);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 640.0f, MediaColumnID_Name);
        ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthFixed, 80.0f, MediaColumnID_Duration);
        ImGui::TableSetupColumn("HotKey", ImGuiTableColumnFlags_PreferSortDescending | ImGuiTableColumnFlags_WidthStretch, 0.0f, MediaColumnID_Hotkey);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        static ImVector<int> selection;
        static int selected_item = 0;

        ImGuiListClipper clipper;
        clipper.Begin(plist.playList.size());
        while (clipper.Step()) {
            for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++) {
                char label[32];
                Media* item = &plist.playList[row_n];
                sprintf_s(label, "%04d", item->ID);
                ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick;
                const bool item_is_selected = selection.contains(item->ID);

                ImGui::PushID(item->ID);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                if (ImGui::Selectable(label, selected_item == row_n, selectable_flags)) {
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        selected_item = row_n;
                        player.PlayNewMedia(item->file);
                        currentMediaName = item->name;
                        plist.activeMedia = item;
                    }
                }

                //TODO pop up on select hot key
                //ImGui::OpenPopupOnItemClick("select_key_popup", ImGuiPopupFlags_MouseButtonRight);

                if (ImGui::BeginPopupContextItem()) {
                    ImGui::SeparatorText("Select HotKey");
                    //ImGui::Text("Mod"); ImGui::SameLine(); ImGui::Combo("##key5", &item_current_5, MODS1, IM_ARRAYSIZE(MODS1));
                    ImGui::Text("Key"); ImGui::SameLine(); ImGui::Combo("##key4", &item_current_4, KEYS1, IM_ARRAYSIZE(KEYS1));
                    if (ImGui::Button("Apply HotKey")) {
                        // Assign hotkey shortcut
                        printf("Hotkey assign\n");
                        int keycode = 0;
                        int mod = 0;

                        if (item_current_4 > 0 && item_current_4 <= 10) {
                            keycode = item_current_4 + SDLK_0 - 1;
                        }  else if (item_current_4 > 10 && item_current_4 <= 20) {
                            keycode = SDLK_KP_1 + (item_current_4 - 11);
                        } else if (item_current_4 > 20 && item_current_4 < 47) {
                            keycode = SDLK_a + (item_current_4 - 21);
                        }

                        HotKeyData hk = { mod, keycode };
                        plist.AssignHotkey(item->ID, hk);
                        item->hotkey.keycode = hk.keycode;
                        item->isHotkey = true;
                        ImGui::CloseCurrentPopup();
                        item_current_4 = -1;
                    }
                    ImGui::EndPopup();
                }

                ImGui::TableNextColumn();
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(item->name.c_str());
                ImGui::TableNextColumn();
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d:%02d", item->length / 60u, item->length % 60u);
                ImGui::TableNextColumn();
                ImGui::TableSetColumnIndex(3);
                if (item->isHotkey) {
                    if (!item->hotkey.mod) {
                        ImGui::Text("%s", item->hotkey.GetKeyName().c_str());
                    }
                }
                else {

                }
                ImGui::PopID();
            }
        }

        ImGui::EndTable();

        if (ImGui::Button("Clear Media List")) {
            player.Stop();
            player.Eject();
            plist.ClearPlayList();
        }

        ImGui::SeparatorText("End Line");

        if (ImGui::Button("Exit")) {
            isRunning = false;
        }
        ImGui::SameLine();
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate); ImGui::SameLine();

        ImGui::EndTabItem();
    }


    //
    // Construction of ui under label "SoundPad"
    //
    if (ImGui::BeginTabItem("SoundPad")) {
        const char* btn_names[] = { "Button 1", "Button 2", "Button 3", "Button 4", "Button 5", "Button 6", "Button 7", "Button 8", "Button 9" };
        ImVec2 btn_size = ImVec2(180, 120);

        // TODO: Add tooltips for assigned sounds
        for (int i = 0; i < 9; i++)
        {
            ImGui::PushID(i);
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(i / 9.0f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(i / 9.0f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(i / 9.0f, 0.8f, 0.8f));
            if (ImGui::Button(btn_names[i], btn_size)) {
                plist.PlayByHotkey(i + SDLK_1, 0);
                //plist.PlayByHotkey(i + SDLK_KP_1, 0);
                printf("%s Pressed\n", btn_names[i]);
            }
            ImGui::PopStyleColor(3);
            ImGui::PopID();
            if ((i + 1) % 3 > 0)
                ImGui::SameLine();
        }

        ImGui::SeparatorText("End Line");

        ImGui::EndTabItem();
        if (ImGui::Button("Exit")) {
            isRunning = false;
        }
    }

    ImGui::EndTabBar();

    ImGui::End();

    ImGui::Render();
}

void Application::Shutdown() {
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    Mix_CloseAudio();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    NFD_Quit();
}

int main(int argc, char* argv[]) {
    Application app;

    if (app.Init()) {
        printf("App init failed!\n");
    }

    app.Run();
    
    app.Shutdown();

    return 0;
}