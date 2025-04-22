#include "Media.h"

std::unordered_map<int, std::string> keyNames = {
    { SDLK_0, "0" },
    { SDLK_1, "1" },
    { SDLK_2, "2" },
    { SDLK_3, "3" },
    { SDLK_4, "4" },
    { SDLK_5, "5" },
    { SDLK_6, "6" },
    { SDLK_7, "7" },
    { SDLK_8, "8" },
    { SDLK_9, "9" },
    { SDLK_KP_1, "Numpad 1" },
    { SDLK_KP_2, "Numpad 2" },
    { SDLK_KP_3, "Numpad 3" },
    { SDLK_KP_4, "Numpad 4" },
    { SDLK_KP_5, "Numpad 5" },
    { SDLK_KP_6, "Numpad 6" },
    { SDLK_KP_7, "Numpad 7" },
    { SDLK_KP_8, "Numpad 8" },
    { SDLK_KP_9, "Numpad 9" },
    { SDLK_KP_0, "Numpad 0" },
    { SDLK_a, "A" },
    { SDLK_b, "B" },
    { SDLK_c, "C" },
    { SDLK_d, "D" },
    { SDLK_e, "E" },
    { SDLK_f, "F" },
    { SDLK_g, "G" },
    { SDLK_h, "H" },
    { SDLK_i, "I" },
    { SDLK_j, "J" },
    { SDLK_k, "K" },
    { SDLK_l, "L" },
    { SDLK_m, "M" },
    { SDLK_n, "N" },
    { SDLK_o, "O" },
    { SDLK_p, "P" },
    { SDLK_q, "Q" },
    { SDLK_r, "R" },
    { SDLK_s, "S" },
    { SDLK_t, "T" },
    { SDLK_u, "U" },
    { SDLK_v, "V" },
    { SDLK_w, "W" },
    { SDLK_x, "X" },
    { SDLK_y, "Y" },
    { SDLK_z, "Z" }
};

//const char* MODS1[] = { "", "SHIFT", "CTRL", "ALT", "SHIFT+CTRL", "SHIFT+ALT", "ALT+CTRL", "SHIFT+ALT+CTRL" };

std::unordered_map<int, std::string> modNames = {
    { KMOD_SHIFT, "SHIFT" },
    { KMOD_CTRL, "CTRL" },
    { KMOD_ALT,  "ALT" },
    { KMOD_SHIFT | KMOD_ALT, "SHIFT+CTRL" },
    { KMOD_SHIFT | KMOD_CTRL, "SHIFT+ALT" },
    { KMOD_ALT | KMOD_CTRL, "ALT+CTRL" },
    { KMOD_SHIFT | KMOD_ALT | KMOD_CTRL, "SHIFT+ALT+CTRL"}
};


std::string HotKeyData::GetKeyName() {
    return keyNames[keycode];
}

////////////////
//
// Media Player
//
////////////////

MediaPlayer::MediaPlayer() {}
MediaPlayer::~MediaPlayer() {}

void MediaPlayer::SetMedia(Mix_Music* m) {
    playingMedia = m;
    hasMedia = true;
}

void MediaPlayer::Eject() {
    playingMedia = nullptr;
    hasMedia = true;
}

void MediaPlayer::Play() {
    printf("PLayer play\n");
    if (playerState == MUS_STATE::PAUSED) {
        playerState = MUS_STATE::PLAYING;
        printf("PLayer resume\n");
        Mix_ResumeMusic();
    }
    else {
        playerState = MUS_STATE::PLAYING;
        Mix_PlayMusic(playingMedia, 0);
    }
}

void MediaPlayer::PlayNewMedia(Mix_Music* m) {
    printf("PLayer play new\n");
    if (playerState == MUS_STATE::PLAYING) {
        Mix_HaltMusic();
    }
    playerState = MUS_STATE::PLAYING;
    playingMedia = m;
    hasMedia = true;
    Mix_PlayMusic(playingMedia, 0);
}

void MediaPlayer::Stop() {
    printf("PLayer stop\n");
    if (playerState == MUS_STATE::PAUSED || playerState == MUS_STATE::PLAYING) {
        playerState = MUS_STATE::STOPED;
        Mix_HaltMusic();
    }
}

void MediaPlayer::Pause() {
    printf("PLayer pause\n");
    if (playerState == MUS_STATE::PLAYING) {
        playerState = MUS_STATE::PAUSED;
        Mix_PauseMusic();
    }
}

void MediaPlayer::FlipPlayPause() {
    if (playerState == MUS_STATE::PLAYING) {
        Pause();
    }
    else if (playerState == MUS_STATE::PAUSED || playerState == MUS_STATE::STOPED) {
        Play();
    }
}

void MediaPlayer::RewindTo(int value) {
    if (playerState == MUS_STATE::PAUSED || playerState == MUS_STATE::PLAYING) {
        Mix_SetMusicPosition((double)value);
    }
}

int MediaPlayer::GetPlayingMediaTime() {
    return (int)Mix_GetMusicPosition(playingMedia);
}

void MediaPlayer::SetMediaHotKey(const HotKeyData& h) {
    hotKey.mod = h.mod;
    hotKey.keycode = h.keycode;
}

void MediaPlayer::Update() {
    if (playerState == MUS_STATE::PLAYING && !Mix_PlayingMusic())
        playerState = MUS_STATE::STOPED;
}



//////////////
//
// Playlist
//
//////////////

PlayList::PlayList(MediaPlayer* pl) {
    player = pl;
}

PlayList::~PlayList() {
    ClearPlayList();
    player = nullptr;
}

void PlayList::LoadPlayList(std::vector<std::string> &pathes) {
    std::string resDirName;
    if (pathes.size() == 0) {
        return;
    }

    for (auto& p : pathes) {
        AddMedia(p);
    }

    std::string pathstr = pathes[0];
    std::size_t subpos = pathstr.find_last_of("\\");
    resDirName = pathstr.substr(0, subpos);

    sprintf_s(playListDirC, "%s\0", resDirName.c_str());
    playListDir = playListDirC;
    activeMedia = &playList[0];
    player->SetMedia(activeMedia->file);
    activeMediaName = activeMedia->name;
}

void PlayList::ClearPlayList() {
    for (auto& m : playList) {
        Mix_FreeMusic(m.file);
    }
    activeMedia = nullptr;
    activeMediaName = "none";
    playListDir = "";
    hotkeyAssigns.clear();
    playList.clear();
}

void PlayList::AddMedia(std::string mediaPath) {
    Media audio;
    static int id = 1;
    Mix_Music* m = Mix_LoadMUS(mediaPath.c_str());

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
    playList.push_back(audio);
}

void PlayList::RemoveMedia(int id) {
    for (auto m = playList.begin(); m != playList.end(); m++) {
        if (m->ID == id) {
            playList.erase(m);
            break;
        }
    }
}

void PlayList::PlayMedia(int id) {

}

void PlayList::PlayNextMedia() {

}

bool PlayList::AssignHotkey(int id, const HotKeyData& hotkey) {
    if (hotkeyAssigns[hotkey] != 0) {
        int oldid = hotkeyAssigns[hotkey];
        hotkeyAssigns[hotkey] = id;
        playList[oldid - 1].isHotkey = false;
    }

    hotkeyAssigns[hotkey] = id;
    //printf("Assigned key: %i, to id: %i\n", hotkey.keycode, id);
    return true;
}

void PlayList::RemoveHotkey(int key, int mod) {
    HotKeyData hk{ mod, key };
    if (hotkeyAssigns.find(hk) != hotkeyAssigns.end()) {
        hotkeyAssigns.erase(hotkeyAssigns.find(hk));
    }
}

void PlayList::PlayByHotkey(int key, int mod) {
    HotKeyData hk{ mod, key };
    auto it = hotkeyAssigns.find(hk);
    if (it != hotkeyAssigns.end()) {
        printf("Hotkey found\n");
        Media* playMedia = &playList[it->second - 1];
        player->PlayNewMedia(playMedia->file);
        activeMediaName = playMedia->name;
        activeMedia = playMedia;
    }
}

void PlayList::AssignPlayer(MediaPlayer* pl) {
    player = pl;
}


/*
void addNewMedia(std::vector<Media>& mediaList, const char* mediaPath) {
    Media audio;
    static int id = 1;
    Mix_Music* m = Mix_LoadMUS(mediaPath);

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
*/

std::vector <std::string> openMediaFilesDialog() {
    const nfdpathset_t* filePathes;
    std::vector<std::string> pathes;

    std::string dirName;

    nfdu8filteritem_t filters[2] = { { "Media", "mp3,wav,flac,ogg" } };
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
            pathes.push_back(path);

            NFD_PathSet_FreePath(path);
        }
    }
    else if (result == NFD_CANCEL) {
        puts("User pressed cancel.");
    }
    else {
        printf("Error: %s\n", NFD_GetError());
    }

    for (auto& m : pathes)
        printf("name: %s\n", m.c_str());
    return pathes;
}