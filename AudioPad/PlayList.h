#pragma once

#include "Media.h"
class PlayList {
public:
    PlayList(MediaPlayer* pl = nullptr);
    ~PlayList();

    void LoadPlayList(std::vector<std::string>& pathes);
    void ClearPlayList();

    void AddMedia(std::string file_name);
    void RemoveMedia(int id);

    void PlayMedia(int id);
    void PlayNextMedia();

    bool AssignHotkey(int id, const HotKeyData& hotkey);
    void RemoveHotkey(int key, int mod);
    void PlayByHotkey(int key, int mod);

    void AssignPlayer(MediaPlayer* pl);

    void CheckOnPlayer();

    char playListDirC[256] = { 0 };
    std::string playListDir = "";
    std::vector<Media> playList;

    std::unordered_map<HotKeyData, int> hotkeyAssigns;

    Media* activeMedia = nullptr;
    int activeMediaId = 0;
    std::string activeMediaName = "none";

    MediaPlayer* player = nullptr;
};

