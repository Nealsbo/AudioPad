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

enum MediaColumnID
{
    MediaColumnID_Name,
    MediaColumnID_Length,
    MediaColumnID_ShortCut
};

struct Media {
    Media() {}
    ~Media() {}

    Mix_Music* file = nullptr;
    std::string path = "";
    std::string name = "blank";
    std::string type = "mp3";
    uint32_t length = 1;
    int ID = 0;
};

enum MUS_STATE {
    IDLE = 0,
    PLAYING,
    STOPED,
    PAUSED
};

enum MUS_CMD {
    PLAY = 1,
    STOP,
    PAUSE
};

struct MediaPlayer {
    MediaPlayer() {}
    ~MediaPlayer() {}

    void SetMedia(Mix_Music* m) {
        currentMedia = m;
        hasMedia = true;
    }

    void Eject() {
        currentMedia = nullptr;
        hasMedia = true;
    }

    void Play() {
        printf("PLayer play\n");
        if (playerState == MUS_STATE::PAUSED) {
            playerState = MUS_STATE::PLAYING;
            printf("PLayer resume\n");
            Mix_ResumeMusic();
        } else {
            playerState = MUS_STATE::PLAYING;
            Mix_PlayMusic(currentMedia, 0);
        }
    }

    void PlayNewMedia(Mix_Music* m) {
        printf("PLayer play new\n");
        if (playerState == MUS_STATE::PLAYING) {
            Mix_HaltMusic();
        }
        playerState = MUS_STATE::PLAYING;
        currentMedia = m;
        hasMedia = true;
        Mix_PlayMusic(currentMedia, 0);
    }

    void Rewinding(int value) {
        if (playerState == MUS_STATE::PAUSED || playerState == MUS_STATE::PLAYING) {
            Mix_SetMusicPosition((double)value);
        }
    }

    void Stop() {
        printf("PLayer stop\n");
        if (playerState == MUS_STATE::PAUSED || playerState == MUS_STATE::PLAYING) {
            playerState = MUS_STATE::STOPED;
            Mix_HaltMusic();
        }
    }

    void Pause() {
        printf("PLayer pause\n");
        if (playerState == MUS_STATE::PLAYING) {
            playerState = MUS_STATE::PAUSED;
            Mix_PauseMusic();
        }
    }

    int GetCurrentMediaTime() {
        return (int)Mix_GetMusicPosition(currentMedia);
    }

    void Update() {
        if(playerState == MUS_STATE::PLAYING && !Mix_PlayingMusic())
            playerState = MUS_STATE::STOPED;
    }

    Mix_Music* currentMedia = nullptr;
    int playerState = IDLE;
    bool hasMedia = false;
};

const char* KEYS1[] = { "NONE", "SHIFT", "CTRL", "ALT" };
const char* KEYS2[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "A", "B", "C" };

void addNewMedia(std::vector<Media>& mediaList, const char* mediaPath) {
    Media audio;
    static int id = 1;
    Mix_Music *m = Mix_LoadMUS(mediaPath);

    if (!m) {
        printf("Mix_LoadMUS error: %s\n", Mix_GetError());
        return;
    }

    std::string pathstr = mediaPath;
    std::size_t subpos = pathstr.find_last_of("\\");

    audio.path = mediaPath;
    audio.name = pathstr.substr(subpos + 1);
    audio.length = Mix_MusicDuration(m);
    audio.file = m;
    audio.ID = id;
    id++;
    mediaList.push_back(audio);
}

std::string openMediaFilesDialog(std::vector<Media>& mediaList) {
    const nfdpathset_t* filePathes;

    std::string dirName;

    nfdu8filteritem_t filters[2] = { { "Media", "mp3,wav,flac" } };
    nfdopendialogu8args_t args = { 0 };
    args.filterList = filters;
    args.filterCount = 1;

    nfdresult_t result = NFD_OpenDialogMultipleU8_With(&filePathes, &args);
    if (result == NFD_OKAY) {
        puts("Success!");

        nfdpathsetsize_t numPaths;
        NFD_PathSet_GetCount(filePathes, &numPaths);

        nfdpathsetsize_t i;
        if (numPaths) {
            nfdchar_t* path;
            NFD_PathSet_GetPath(filePathes, 0, &path);
            std::string pathstr = path;
            std::size_t subpos = pathstr.find_last_of("\\");
            dirName = pathstr.substr(0, subpos);

            printf("Path %i: %s\n", 0, path);
        }
        for (i = 0; i < numPaths; ++i) {
            nfdchar_t* path;
            NFD_PathSet_GetPath(filePathes, i, &path);
            printf("Path %i: %s\n", (int)i, path);
            addNewMedia(mediaList, path);

            NFD_PathSet_FreePath(path);
        }
    } else if (result == NFD_CANCEL) {
        puts("User pressed cancel.");
    } else {
        printf("Error: %s\n", NFD_GetError());
    }

    for (auto& m : mediaList) {
        printf("name: %s\n", m.name.c_str());
    }
    return dirName;
}

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER ) != 0) {
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
    SDL_Window* window = SDL_CreateWindow("AudioPad", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 540, window_flags);
    if (window == nullptr) {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();



    bool show_demo_window = true;
    bool show_another_window = false;
    bool p_open = true;

    int masterVolume = 100;
    static int currentMediaTime = 0;

    char dir0[256] = "sounds directory...";
    char selMedia[32] = "Selected media: ";
    std::string selMediaName = "";

    MediaPlayer player;
    std::vector<Media> mediaList;

    selMediaName = mediaList.empty() ? "none" : mediaList[0].name;
    Media* activeMedia = mediaList.empty() ? nullptr : &mediaList[0];

    bool done = false;
    bool isPlay = false;

    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(16);
            continue;
        }

        Mix_VolumeMusic((masterVolume * MIX_MAX_VOLUME) / 100);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        {
            static bool use_work_area = true;
            static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
            ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);

            static float f = 0.0f;
            static int counter = 0;

            player.Update();

            ImGui::Begin("AudioPadPlayer", &p_open, flags);

            ImGui::Text("Select Sounds Folder:");
            ImGui::InputText("##", dir0, IM_ARRAYSIZE(dir0), ImGuiInputTextFlags_ReadOnly);
            ImGui::SameLine();
            if (ImGui::Button("...")) {
                std::string resDirName = openMediaFilesDialog(mediaList);

                if (!resDirName.empty()) {
                    sprintf_s(dir0, "%s\0", resDirName.c_str());
                    activeMedia = &mediaList[0];
                    player.SetMedia(activeMedia->file);
                    selMediaName = activeMedia->name;
                }

            }

            ImGui::SeparatorText("Player");

            ImGui::PushItemWidth(60);
            if (ImGui::Button("Play")) {
                if (activeMedia == nullptr) {
                    printf("Nothing to play\n");
                } else {
                    if (player.currentMedia != activeMedia->file) {
                        player.PlayNewMedia(activeMedia->file);
                    } else {
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
            currentMediaTime = 0;
            int minlength = 0;
            int medialength = 0;

            if (!mediaList.empty()) {
                currentMediaTime = player.GetCurrentMediaTime();
                medialength = activeMedia->length;
            }

            sprintf_s(timestr, "%i:%02i/%i:%02i\0", currentMediaTime / 60, currentMediaTime % 60, medialength / 60, medialength % 60);
            ImGui::Text(timestr); ImGui::SameLine();

            ImGui::PushItemWidth(-180);
            if (ImGui::SliderInt("##2", &currentMediaTime, minlength, medialength, "")) {
                player.Rewinding(currentMediaTime);
            }
            //ImGui::ProgressBar((float)(currentMediaTime) / medialength, ImVec2(0.0f, 0.0f));
            ImGui::PopItemWidth(); ImGui::SameLine();
            ImGui::PushItemWidth(120);
            ImGui::SliderInt("Volume", &masterVolume, 0, 100);
            ImGui::PopItemWidth();

            ImGui::Text(selMedia); ImGui::SameLine();
            ImGui::Text(selMediaName.c_str());

            ImGui::SeparatorText("Short Cut Combo");

            ImGui::PushItemWidth(60);
            static int item_current_2 = 0;
            ImGui::Combo("Key 1", &item_current_2, KEYS1, IM_ARRAYSIZE(KEYS1)); ImGui::SameLine();
            static int item_current_3 = -1;
            ImGui::Combo("Key 2", &item_current_3, KEYS2, IM_ARRAYSIZE(KEYS2));
            ImGui::PopItemWidth();
            static int item_current_4 = -1;


            ImGui::SeparatorText("Audio List");

            static ImGuiTableFlags tableFlags =
                ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
                | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody
                | ImGuiTableFlags_ScrollY;

            ImGui::BeginTable("Audio List", 4, tableFlags, ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 15), 0.0f);

            //ImGuiTableColumnFlags_DefaultSort
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40.0f, MediaColumnID_Name);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 640.0f, MediaColumnID_Name);
            ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthFixed, 80.0f, MediaColumnID_Length);
            ImGui::TableSetupColumn("ShortCut", ImGuiTableColumnFlags_PreferSortDescending | ImGuiTableColumnFlags_WidthStretch, 0.0f, MediaColumnID_ShortCut);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            if (ImGui::BeginPopup("select_key_popup"))
            {
                ImGui::SeparatorText("Select HotKey");
                ImGui::Combo("##", &item_current_4, KEYS2, IM_ARRAYSIZE(KEYS2));
                ImGui::EndPopup();
            }

            static ImVector<int> selection;
            static int selected_item = 0;

            ImGuiListClipper clipper;
            clipper.Begin(mediaList.size());
            while (clipper.Step()) {
                for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++) {
                    char label[32];
                    Media* item = &mediaList[row_n];
                    sprintf_s(label, "%04d", item->ID);
                    ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick;
                    const bool item_is_selected = selection.contains(item->ID);

                    ImGui::PushID(item->ID);
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);

                    if (ImGui::Selectable(label, selected_item == row_n, selectable_flags)) {
                        if (ImGui::IsMouseDoubleClicked(0)) {
                            selected_item = row_n;
                            player.PlayNewMedia(mediaList[row_n].file);
                            selMediaName = mediaList[row_n].name;
                            activeMedia = &mediaList[row_n];
                        }
                    }

                    //TODO pop up on select hot key
                    //ImGui::OpenPopupOnItemClick("select_key_popup", ImGuiPopupFlags_MouseButtonRight);
                    /*
                    if (ImGui::BeginPopupContextItem()) // <-- use last item id as popup id
                    {
                        ImGui::SeparatorText("Select HotKey");
                        ImGui::Combo("##key3", &item_current_4, KEYS2, IM_ARRAYSIZE(KEYS2));
                        if (ImGui::Button("Close"))
                            ImGui::CloseCurrentPopup();
                        ImGui::EndPopup();
                    }
                    */

                    ImGui::TableNextColumn();
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(item->name.c_str());
                    ImGui::TableNextColumn();
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%d:%02d", item->length / 60u, item->length % 60u);
                    //ImGui::Text("%04d", item->length);
                    ImGui::PopID();
                }
            }

            ImGui::EndTable();

            if (ImGui::Button("Clear Media List")) {
                player.Stop();
                player.Eject();
                selMediaName = "none";
                activeMedia = nullptr;
                mediaList.clear();
            }

            ImGui::SeparatorText("End Line");

            if (ImGui::Button("Exit")) done = true; ImGui::SameLine();
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate); ImGui::SameLine();

            ImGui::End();
        }

        ImGui::Render();

        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.3, 0.3, 0.3, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    NFD_Quit();

    return 0;
}