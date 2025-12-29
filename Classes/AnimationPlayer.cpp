#include "AnimationPlayer.h"
#include "Player.h"

USING_NS_CC;

// Helper to play animation
static void runAnimation(Player* owner, const std::string& filename, bool loop = true) {
    if (!owner || !owner->getModel()) return;
    
    auto model = owner->getModel();
    
    // 1. Try loading specific file
    auto animation = Animation3D::create(filename);
    
    // 2. Fallback: Try loading from character.c3b with animation name
    if (!animation) {
        std::string animName = filename;
        // Extract filename without extension: "models/.../run.c3b" -> "run"
        size_t lastSlash = animName.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            animName = animName.substr(lastSlash + 1);
        }
        size_t lastDot = animName.find_last_of(".");
        if (lastDot != std::string::npos) {
            animName = animName.substr(0, lastDot);
        }
        
        std::string skeletonPath = "models/player/skeleton/character.c3b";
        
        // Try with name
        animation = Animation3D::create(skeletonPath, animName);
        
        // 3. Fallback: Try default from character.c3b (only if we really need something)
        // But this might play "idle" when we want "run", which is weird. 
        // Better to fail than play wrong animation?
        // Let's try to load the default one just to see if ANY animation exists.
        if (!animation) {
             // Try "Take 001" (Common default)
             animation = Animation3D::create(skeletonPath, "Take 001");
        }
        
        if (animation) {
             CCLOG("Loaded animation '%s' from character.c3b (fallback for %s)", animName.c_str(), filename.c_str());
        }
    }

    if (animation) {
        auto animate = Animate3D::create(animation);
        model->stopAllActions();
        if (loop) {
            model->runAction(RepeatForever::create(animate));
        } else {
            model->runAction(animate);
        }
        // CCLOG("Playing animation: %s", filename.c_str());
    } else {
        CCLOG("Failed to load animation: %s (and fallback failed)", filename.c_str());
    }
}

AnimationPlayer* AnimationPlayer::create(Player* owner) {
    AnimationPlayer* ret = new (std::nothrow) AnimationPlayer();
    if (ret && ret->init(owner)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool AnimationPlayer::init(Player* owner) {
    _owner = owner;
    _currentState = AnimState::NONE;
    _stateTime = 0.0f;
    return true;
}

void AnimationPlayer::update(float dt) {
    _stateTime += dt;
}

void AnimationPlayer::playState(AnimState state) {
    // Force replay for SHOOT to ensure it restarts?
    // No, if we call this every frame in updateVisuals, we shouldn't restart it.
    // Only restart if transitioning FROM another state, which is handled by _currentState == state check.
    
    if (_currentState == state && state != AnimState::CELEBRATE) return;
    
    _currentState = state;
    _stateTime = 0.0f;
    
    switch (state) {
        case AnimState::IDLE: playIdle(); break;
        case AnimState::RUN: playRun(); break;
        case AnimState::DRIBBLE: playDribble(); break;
        case AnimState::SHOOT: playShoot(); break;
        case AnimState::JUMP: playJump(); break;
        case AnimState::DEFEND: playDefend(); break;
        case AnimState::CELEBRATE: playCelebrate(); break;
    }
}

void AnimationPlayer::playDefend() {
    runAnimation(_owner, "models/player/animations/defend.c3b", true);
}

void AnimationPlayer::stopAllParts() {
    if (_owner && _owner->getModel()) {
        _owner->getModel()->stopAllActions();
    }
}

void AnimationPlayer::playIdle() {
    // If we have ball, hold. If not, maybe defend?
    // For now, use hold as default idle.
    runAnimation(_owner, "models/player/animations/hold.c3b", true);
}

void AnimationPlayer::playRun() {
    runAnimation(_owner, "models/player/animations/run.c3b", true);
}

void AnimationPlayer::playDribble() {
    runAnimation(_owner, "models/player/animations/dribble.c3b", true);
}

void AnimationPlayer::playShoot() {
    runAnimation(_owner, "models/player/animations/shoot.c3b", false);
}

void AnimationPlayer::playJump() {
    // Match AI: AI uses defend animation when jumping (blocking)
    // Using defend.c3b makes the jump look like a block/rebound attempt
    runAnimation(_owner, "models/player/animations/defend.c3b", true); 
}

void AnimationPlayer::playCelebrate() {
    runAnimation(_owner, "models/player/animations/defend.c3b", true);
}

void AnimationPlayer::setRunSpeed(float speed) {
    // Could adjust speed of action if we stored it
}

void AnimationPlayer::setLookAt(const cocos2d::Vec3& target) {
    // IK not supported yet
}
