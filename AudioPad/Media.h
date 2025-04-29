#pragma once

#include <SDL.h>
#include <SDL_mixer.h>

#include <string>
#include <vector>
#include <unordered_map>

#include "nfd.h"



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

struct HotKeyData {
    bool operator==(const HotKeyData& h) const {
        return mod == h.mod && keycode == h.keycode;
    }

    std::string GetKeyName();

    int mod = 0;
    int keycode = 0;
    bool isNum = false;
};

template<>
struct std::hash<HotKeyData> {
    size_t operator()(const HotKeyData& h) const {
        return (hash<SDL_Keycode>()(h.keycode) ^ (hash<Uint16>()(h.mod) << 1));
    }
};

struct Media {
    Mix_Music* file = nullptr;

    std::string path = "";
    std::string name = "blank";
    std::string type = "mp3";

    uint32_t length = 1;
    int ID = 0;

    bool isHotkey = false;
    HotKeyData hotkey;
};

std::vector <std::string> openMediaFilesDialog();

struct MediaPlayer {
    MediaPlayer();
    ~MediaPlayer();

    void SetMedia(Mix_Music* m);
    void Eject();
    void Play();
    void PlayNewMedia(Mix_Music* m);
    void Stop();
    void Pause();
    void FlipPlayPause();
    void RewindTo(int value);
    int GetPlayingMediaTime();
    void SetMediaHotKey(const HotKeyData& h);
    void Update();

    Mix_Music* playingMedia = nullptr;
    int playerState = IDLE;
    bool hasMedia = false;
    HotKeyData hotKey;
};