#include "AudioManager.h"
#include "base/CCDirector.h"
#include "2d/CCCamera.h"
#include "2d/CCScene.h"

USING_NS_CC;
using namespace CocosDenshion;

AudioManager* AudioManager::_instance = nullptr;

AudioManager* AudioManager::getInstance() {
    if (!_instance) {
        _instance = new AudioManager();
        _instance->init();
    }
    return _instance;
}

void AudioManager::destroyInstance() {
    if (_instance) {
        delete _instance;
        _instance = nullptr;
    }
}

AudioManager::AudioManager() 
    : _masterVolume(1.0f)
    , _musicVolume(0.5f)
    , _sfxVolume(1.0f)
    , _listenerPos(Vec3::ZERO)
{
}

AudioManager::~AudioManager() {
    SimpleAudioEngine::end();
}

void AudioManager::init() {
    // Preload effects if needed
}

void AudioManager::playBackgroundMusic(const std::string& filename, bool loop) {
    SimpleAudioEngine::getInstance()->playBackgroundMusic(filename.c_str(), loop);
    SimpleAudioEngine::getInstance()->setBackgroundMusicVolume(_musicVolume * _masterVolume);
}

void AudioManager::stopBackgroundMusic() {
    SimpleAudioEngine::getInstance()->stopBackgroundMusic();
}

void AudioManager::pauseBackgroundMusic() {
    SimpleAudioEngine::getInstance()->pauseBackgroundMusic();
}

void AudioManager::resumeBackgroundMusic() {
    SimpleAudioEngine::getInstance()->resumeBackgroundMusic();
}

void AudioManager::setBackgroundMusicVolume(float volume) {
    _musicVolume = volume;
    SimpleAudioEngine::getInstance()->setBackgroundMusicVolume(_musicVolume * _masterVolume);
}

unsigned int AudioManager::playEffect(const std::string& filename, bool loop, float pitch, float pan, float gain) {
    // Check limit? SimpleAudioEngine usually handles max instances (32 default on windows)
    // We can just play.
    return SimpleAudioEngine::getInstance()->playEffect(filename.c_str(), loop, pitch, pan, gain * _sfxVolume * _masterVolume);
}

void AudioManager::stopEffect(unsigned int soundId) {
    SimpleAudioEngine::getInstance()->stopEffect(soundId);
}

void AudioManager::stopAllEffects() {
    SimpleAudioEngine::getInstance()->stopAllEffects();
}

void AudioManager::setEffectsVolume(float volume) {
    _sfxVolume = volume;
    SimpleAudioEngine::getInstance()->setEffectsVolume(_sfxVolume * _masterVolume);
}

void AudioManager::setMasterVolume(float volume) {
    _masterVolume = volume;
    // Update both
    setBackgroundMusicVolume(_musicVolume);
    setEffectsVolume(_sfxVolume);
}

void AudioManager::updateListenerPosition() {
    auto scene = Director::getInstance()->getRunningScene();
    if (!scene) return;
    
    // Find default camera or user camera
    Camera* cam = scene->getDefaultCamera();
    if (cam) {
        _listenerPos = cam->getPosition3D();
    }
}

unsigned int AudioManager::playSpatialEffect(const std::string& filename, const Vec3& position, float maxDistance) {
    updateListenerPosition();
    
    float dist = position.distance(_listenerPos);
    if (dist > maxDistance) return 0; // Too far
    
    float gain = 1.0f - (dist / maxDistance);
    if (gain < 0.0f) gain = 0.0f;
    
    // Simple Pan (Left/Right)
    // Project position to camera space to get pan?
    // Simplified: just attenuation for now.
    
    return playEffect(filename, false, 1.0f, 0.0f, gain);
}
