#include "EffectsManager.h"

USING_NS_CC;

EffectsManager* EffectsManager::_instance = nullptr;

EffectsManager* EffectsManager::getInstance() {
    if (!_instance) {
        _instance = new EffectsManager();
    }
    return _instance;
}

void EffectsManager::destroyInstance() {
    if (_instance) {
        delete _instance;
        _instance = nullptr;
    }
}

EffectsManager::EffectsManager() 
    : _scene(nullptr)
    , _shakeTimer(0.0f)
    , _shakeIntensity(0.0f)
    , _originalCameraPos(Vec3::ZERO)
{
}

EffectsManager::~EffectsManager() {
}

void EffectsManager::init(Scene* scene) {
    _scene = scene;
}

void EffectsManager::playGoalEffect(const Vec3& position) {
    if (!_scene) return;

    // Sparks / Fireworks
    auto emitter = ParticleFireworks::create();
    emitter->retain();
    
    // Configure
    emitter->setDuration(1.0f);
    emitter->setLife(0.5f);
    emitter->setLifeVar(0.2f);
    emitter->setSpeed(100);
    emitter->setSpeedVar(30);
    emitter->setStartColor(Color4F::ORANGE);
    emitter->setEndColor(Color4F::RED);
    emitter->setStartSize(10.0f);
    emitter->setEndSize(0.0f);
    emitter->setEmissionRate(100);
    emitter->setAutoRemoveOnFinish(true);
    
    // 3D Billboard?
    // Cocos2d-x ParticleSystemQuad is 2D. To use in 3D, we can use Billboard or Pupil.
    // Or just project 3D pos to 2D and add to UI layer?
    // Let's try to add it as a BillboardParticleSystem if available, or just a 2D particle on top of everything at projected pos.
    
    // Better: Use Pupil/Particle3D if available, but standard ParticleSystemQuad is easier.
    // Let's use 2D projection for simplicity as requested.
    
    Camera* camera = nullptr;
    const auto& cams = _scene->getCameras();
    for (auto* c : cams) {
        if (c->getCameraFlag() == CameraFlag::USER1) { camera = c; break; }
    }
    if (!camera) camera = _scene->getDefaultCamera();
    if (camera) {
        Vec2 screenPos = camera->projectGL(position);
        emitter->setPosition(screenPos);
        _scene->addChild(emitter, 1000); // Top Z
    }
    
    emitter->release();
}

void EffectsManager::playRimHitEffect(const Vec3& position) {
    if (!_scene) return;
    
    // Dust / Small Debris
    auto emitter = ParticleExplosion::create();
    emitter->retain();
    
    emitter->setDuration(0.1f);
    emitter->setLife(0.3f);
    emitter->setSpeed(50);
    emitter->setStartColor(Color4F::GRAY);
    emitter->setEndColor(Color4F(0.5, 0.5, 0.5, 0));
    emitter->setStartSize(5.0f);
    emitter->setAutoRemoveOnFinish(true);
    
    Camera* camera = nullptr;
    const auto& cams = _scene->getCameras();
    for (auto* c : cams) {
        if (c->getCameraFlag() == CameraFlag::USER1) { camera = c; break; }
    }
    if (!camera) camera = _scene->getDefaultCamera();
    if (camera) {
        Vec2 screenPos = camera->projectGL(position);
        emitter->setPosition(screenPos);
        _scene->addChild(emitter, 1000);
    }
    
    emitter->release();
}

void EffectsManager::playSweatEffect(const Vec3& position) {
    // Optional, maybe skip for now to keep performance high
}

void EffectsManager::shakeScreen(float intensity, float duration) {
    _shakeIntensity = intensity;
    _shakeTimer = duration;
    
    if (_scene) {
        auto camera = _scene->getDefaultCamera();
        if (camera && _originalCameraPos.isZero()) {
            _originalCameraPos = camera->getPosition3D();
        }
    }
}

void EffectsManager::update(float dt) {
    if (_shakeTimer > 0) {
        _shakeTimer -= dt;
        
        if (_scene) {
            auto camera = _scene->getDefaultCamera();
            if (camera) {
                if (_originalCameraPos.isZero()) _originalCameraPos = camera->getPosition3D();
                
                float offsetX = (CCRANDOM_0_1() * 2 - 1) * _shakeIntensity;
                float offsetY = (CCRANDOM_0_1() * 2 - 1) * _shakeIntensity;
                float offsetZ = (CCRANDOM_0_1() * 2 - 1) * _shakeIntensity;
                
                camera->setPosition3D(_originalCameraPos + Vec3(offsetX, offsetY, offsetZ));
            }
        }
        
        if (_shakeTimer <= 0) {
            // Reset
            if (_scene) {
                auto camera = _scene->getDefaultCamera();
                if (camera && !_originalCameraPos.isZero()) {
                    camera->setPosition3D(_originalCameraPos);
                    _originalCameraPos = Vec3::ZERO;
                }
            }
        }
    }
}
