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

enum MediaColumnID
{
    MediaColumnID_Name,
    MediaColumnID_Duration,
    MediaColumnID_Hotkey
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

std::string openMediaFilesDialog(std::vector<Media>&);

struct HotKeyData {
    bool operator==(const HotKeyData& h) const {
        return mod == h.mod && keycode == h.keycode;
    }

    int mod = 0;
    int keycode = 0;
};

template<>
struct std::hash<HotKeyData> {
    size_t operator()(const HotKeyData& h) const {
        return (hash<SDL_Keycode>()(h.keycode) ^ (hash<Uint16>()(h.mod) << 1));
    }
};

struct MediaPlayer {
    MediaPlayer() {}
    ~MediaPlayer() {}

    void SetMedia(Mix_Music* m) {
        playingMedia = m;
        hasMedia = true;
    }

    void Eject() {
        playingMedia = nullptr;
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
            Mix_PlayMusic(playingMedia, 0);
        }
    }

    void PlayNewMedia(Mix_Music* m) {
        printf("PLayer play new\n");
        if (playerState == MUS_STATE::PLAYING) {
            Mix_HaltMusic();
        }
        playerState = MUS_STATE::PLAYING;
        playingMedia = m;
        hasMedia = true;
        Mix_PlayMusic(playingMedia, 0);
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

    void FlipPlayPause() {
        if (playerState == MUS_STATE::PLAYING) {
            Pause();
        } else if (playerState == MUS_STATE::PAUSED || playerState == MUS_STATE::STOPED) {
            Play();
        }
    }

    void RewindTo(int value) {
        if (playerState == MUS_STATE::PAUSED || playerState == MUS_STATE::PLAYING) {
            Mix_SetMusicPosition((double)value);
        }
    }

    int GetPlayingMediaTime() {
        return (int)Mix_GetMusicPosition(playingMedia);
    }

    void SetMediaHotKey(const HotKeyData& h) {
        hotKey.mod = h.mod;
        hotKey.keycode = h.keycode;
    }

    void Update() {
        if(playerState == MUS_STATE::PLAYING && !Mix_PlayingMusic())
            playerState = MUS_STATE::STOPED;
    }

    Mix_Music* playingMedia = nullptr;
    int playerState = IDLE;
    bool hasMedia = false;
    HotKeyData hotKey;
};

struct PlayList {
    PlayList(MediaPlayer* pl = nullptr) {
        player = pl;
    }

    ~PlayList() {
        player = nullptr;
    }

    void LoadPlayList() {
        std::string resDirName = openMediaFilesDialog(playList);

        if (!resDirName.empty()) {
            sprintf_s(playListDirC, "%s\0", resDirName.c_str());
            playListDir = playListDirC;
            activeMedia = &playList[0];
            player->SetMedia(activeMedia->file);
            currentMediaName = activeMedia->name;
        }
    }

    void ClearPlayList() {
        activeMedia = nullptr;
        currentMediaName = "none";
        playListDir = "";
        hotkeyAssigns.clear();
        playList.clear();
    }

    bool AssignHotkey(int id, const HotKeyData& hotkey) {
        CheckOnPlayer();
        if (hotkeyAssigns.find(hotkey) != hotkeyAssigns.end()) {
            hotkeyAssigns.emplace(hotkey, id);
            return true;
        }
        return false;
    }

    void RemoveHotkey(int key, int mod) {
        HotKeyData hk{ mod, key };
        if (hotkeyAssigns.find(hk) != hotkeyAssigns.end()) {
            hotkeyAssigns.erase(hotkeyAssigns.find(hk));
        }
    }

    void PlayByHotkey(int key, int mod) {
        CheckOnPlayer();
        HotKeyData hk{ mod, key };
        if (hotkeyAssigns.find(hk) != hotkeyAssigns.end()) {
            Media* playMedia = &playList[hotkeyAssigns[hk]];
            player->PlayNewMedia(playMedia->file);
        }
    }

    void AssignPlayer(MediaPlayer* pl) {
        player = pl;
    }

    void CheckOnPlayer() {
        assert(player);
    }

    char playListDirC[128] = { 0 };
    std::string playListDir = "";

    std::vector<Media> playList;
    std::unordered_map<HotKeyData, int> hotkeyAssigns;

    MediaPlayer* player = nullptr;
    std::string currentMediaName = "none";
    Media* activeMedia = nullptr;
};

const char* MODS1[] = { "", "SHIFT", "CTRL", "ALT", "SHIFT+CTRL", "SHIFT+ALT", "ALT+CTRL", "SHIFT+ALT+CTRL" };
const char* KEYS1[] = { "", "1", "2", "3", "4", "5", "6", "7", "8", "9" };

void strToSdlk_Conv(int &key, const char* line) {
    int keycode = (int)line[0];
}

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
    }
}

void Application::DrawUI() {
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    {
        static bool use_work_area = true;
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
        ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);

        ImGui::Begin("AudioPadPlayer", &p_open, flags);
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

        ImGui::SeparatorText("Hot Key Combo");

        // TODO Clean up
        static int item_current_2 = -1;
        static int item_current_3 = -1;
        static int item_current_4 = -1;
        static int item_current_5 = -1;
        ImGui::PushItemWidth(60);
        ImGui::Combo("Mod", &item_current_2, MODS1, IM_ARRAYSIZE(MODS1)); ImGui::SameLine();
        ImGui::Combo("Key", &item_current_3, KEYS1, IM_ARRAYSIZE(KEYS1));
        ImGui::PopItemWidth();

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
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                ImGui::TableNextColumn();
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(item->name.c_str());
                ImGui::TableNextColumn();
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d:%02d", item->length / 60u, item->length % 60u);
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

        ImGui::End();
    }

    ImGui::Render();
}

void Application::Shutdown() {
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

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