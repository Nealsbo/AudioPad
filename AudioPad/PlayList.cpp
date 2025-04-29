#include "PlayList.h"

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
            if (activeMedia->ID == id) {
                player->Eject();
            }

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

    return &playList[it - playList.begin()];
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
    printf("Assigned key: %i, to id: %i\n", hotkey.keycode, id);
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
        Media* playMedia = FindMedia(it->second);
        player->PlayNewMedia(playMedia->file);
        activeMediaName = playMedia->name;
        activeMedia = playMedia;
    }
}

void PlayList::AssignPlayer(MediaPlayer* pl) {
    player = pl;
}