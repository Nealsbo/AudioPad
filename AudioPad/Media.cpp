#include "Media.h"



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
    player = nullptr;
}

void PlayList::LoadPlayList() {
    std::string resDirName = openMediaFilesDialog(playList);

    if (!resDirName.empty()) {
        sprintf_s(playListDirC, "%s\0", resDirName.c_str());
        playListDir = playListDirC;
        activeMedia = &playList[0];
        player->SetMedia(activeMedia->file);
        currentMediaName = activeMedia->name;
    }
}

void PlayList::ClearPlayList() {
    activeMedia = nullptr;
    currentMediaName = "none";
    playListDir = "";
    hotkeyAssigns.clear();
    playList.clear();
}

bool PlayList::AssignHotkey(int id, const HotKeyData& hotkey) {
    hotkeyAssigns[hotkey] = id;
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
        currentMediaName = playMedia->name;
        activeMedia = playMedia;
    }
}

void PlayList::AssignPlayer(MediaPlayer* pl) {
    player = pl;
}


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
    }
    else if (result == NFD_CANCEL) {
        puts("User pressed cancel.");
    }
    else {
        printf("Error: %s\n", NFD_GetError());
    }

    for (auto& m : mediaList) {
        printf("name: %s\n", m.name.c_str());
    }
    return dirName;
}