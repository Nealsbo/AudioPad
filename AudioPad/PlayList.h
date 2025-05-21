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

    Media* FindMedia(int id);
    void SetNextMedia(int id);
    void PlayPrevMedia();
    void PlayNextMedia();

    bool AssignHotkey(int id, const HotKeyData& hotkey);
    int  GetIDByHotkey(const HotKeyData& hotkey);
    void RemoveHotkey(int key, int mod);
    void PlayByHotkey(int key, int mod);

    void AssignPlayer(MediaPlayer* pl);

    void SortByID(bool isAsc);
    void SortByName(bool isAsc);
    void SortByDuration(bool isAsc);

    void SetAutoMode(bool mode);
    void SetLoopMode(bool mode);
    void SetShuffleMode(bool mode);

    void CheckOnPlayer();

    char playListDirC[256] = { 0 };
    std::string playListDir = "";
    std::vector<Media> playList;

    std::unordered_map<HotKeyData, int> hotkeyAssigns;

    Media* activeMedia = nullptr;
    int activeMediaId = 0;
    std::string activeMediaName = "none";
    int activeMediaIndex = 0;

    Media* nextMedia = nullptr;
    int nextMediaId = 0;
    std::string nextMediaName = "none";
    int nextMediaIndex = 0;

    bool isAutoPlay = false;
    bool isShufflePlay = false;
    bool isLoopPlay = false;

    MediaPlayer* player = nullptr;
};

