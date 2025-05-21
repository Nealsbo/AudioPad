#include "PlayList.h"

#include <algorithm>


PlayList::PlayList(MediaPlayer* pl) {
    player = pl;
}

PlayList::~PlayList() {
    ClearPlayList();
    player = nullptr;
}

void PlayList::LoadPlayList(std::vector<std::string>& pathes) {
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
#ifndef WIN32
    snprintf(playListDirC, sizeof(playListDirC), "%s\0", resDirName.c_str());
#else
    sprintf_s(playListDirC, "%s\0", resDirName.c_str());
#endif
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
            if (activeMedia->ID == id) {
                player->Eject();
            }
            Mix_FreeMusic(m->file);

            playList.erase(m);
            break;
        }
    }
}

Media* PlayList::FindMedia(int id) {
    auto it = std::find_if(playList.begin(), playList.end(), 
        [&id](const Media& m) {
            return m.ID == id;
    });

    if (it == playList.end()) {
        return nullptr;
    }

    return &playList[it - playList.begin()];
}

void PlayList::SetNextMedia(int id) {

}

void PlayList::PlayPrevMedia() {
    printf("Play previous media\n");
}

void PlayList::PlayNextMedia() {
    printf("Play previous media\n");
}

bool PlayList::AssignHotkey(int id, const HotKeyData& hotkey) {
    if (hotkeyAssigns[hotkey] != 0) {
        int oldid = hotkeyAssigns[hotkey];
        hotkeyAssigns[hotkey] = id;
        playList[oldid - 1].isHotkey = false;
    }

    hotkeyAssigns[hotkey] = id;
    printf("Assigned key: %i, with mod: %i to id: %i\n", hotkey.keycode, hotkey.mod, id);
    return true;
}

int PlayList::GetIDByHotkey(const HotKeyData& hotkey) {
    if (hotkeyAssigns.find(hotkey) != hotkeyAssigns.end()) {
        return hotkeyAssigns[hotkey];
    } else {
        return -1;
    }
}

void PlayList::RemoveHotkey(int key, int mod) {
    HotKeyData hk{ mod, key };
    if (hotkeyAssigns.find(hk) != hotkeyAssigns.end()) {
        hotkeyAssigns.erase(hotkeyAssigns.find(hk));
    }
}

void PlayList::PlayByHotkey(int key, int mod) {
    int hkmod = mod;
    //expand key modes to l and r keys
    if (mod & KMOD_SHIFT) hkmod |= KMOD_SHIFT;
    if (mod & KMOD_ALT) hkmod |= KMOD_ALT;
    if (mod & KMOD_CTRL) hkmod |= KMOD_CTRL;

    printf("Check hotkey: key %i, mod %i\n", key, hkmod);

    HotKeyData hk{ hkmod, key };
    auto it = hotkeyAssigns.find(hk);
    if (it != hotkeyAssigns.end()) {
        printf("Hotkey found\n");
        Media* playMedia = FindMedia(it->second);
        player->PlayNewMedia(playMedia->file);
        activeMediaName = playMedia->name;
        activeMedia = playMedia;
    }
}

void PlayList::AssignPlayer(MediaPlayer* pl) {
    player = pl;
}

void PlayList::SortByID(bool isAsc = true) {
    if (isAsc)
        std::sort(playList.begin(), playList.end(), [](const Media& a, const Media& b) { return a.ID < b.ID; });
    else
        std::sort(playList.begin(), playList.end(), [](const Media& a, const Media& b) { return a.ID > b.ID; });
}

// TODO: case insensitive
void PlayList::SortByName(bool isAsc = true) {
    if (isAsc)
        std::sort(playList.begin(), playList.end(), [](const Media& a, const Media& b) { return a.name.compare(b.name) >= 0; });
    else
        std::sort(playList.begin(), playList.end(), [](const Media& a, const Media& b) { return a.name.compare(b.name) < 0; });
}

void PlayList::SortByDuration(bool isAsc = true) {
    if (isAsc)
        std::sort(playList.begin(), playList.end(), [](const Media& a, const Media& b) { return a.length < b.length; });
    else
        std::sort(playList.begin(), playList.end(), [](const Media& a, const Media& b) { return a.length > b.length; });
}

void PlayList::SetAutoMode(bool mode) {
    printf("Change auto mode to %s\n", mode ? "true" : "false");
    isAutoPlay = mode;
}

void PlayList::SetLoopMode(bool mode) {
    printf("Change loop mode to %s\n", mode ? "true" : "false");
    isLoopPlay = mode;
}

void PlayList::SetShuffleMode(bool mode) {
    printf("Change shuffle mode to %s\n", mode ? "true" : "false");
    isShufflePlay = mode;
}