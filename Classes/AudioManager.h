#ifndef __AUDIO_MANAGER_H__
#define __AUDIO_MANAGER_H__

#include "cocos2d.h"
#include "SimpleAudioEngine.h"
#include <string>

class AudioManager {
public:
    static AudioManager* getInstance();
    static void destroyInstance();

    // Initialization
    void init();

    // Music
    void playBackgroundMusic(const std::string& filename, bool loop = true);
    void stopBackgroundMusic();
    void pauseBackgroundMusic();
    void resumeBackgroundMusic();
    void setBackgroundMusicVolume(float volume);

    // Sound Effects
    unsigned int playEffect(const std::string& filename, bool loop = false, float pitch = 1.0f, float pan = 0.0f, float gain = 1.0f);
    void stopEffect(unsigned int soundId);
    void stopAllEffects();
    void setEffectsVolume(float volume);

    // Spatial Audio (Simplified)
    // Plays effect with volume attenuation based on distance from listener
    unsigned int playSpatialEffect(const std::string& filename, const cocos2d::Vec3& position, float maxDistance = 50.0f);

    // Settings
    void setMasterVolume(float volume);
    float getMasterVolume() const { return _masterVolume; }

private:
    AudioManager();
    ~AudioManager();
    
    static AudioManager* _instance;
    
    float _masterVolume;
    float _musicVolume;
    float _sfxVolume;

    cocos2d::Vec3 _listenerPos; // Usually camera position
    
    void updateListenerPosition();
};

#endif // __AUDIO_MANAGER_H__
