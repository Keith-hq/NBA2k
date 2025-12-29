#include "HumanController.h"
#include "Player.h" // To check state/position

USING_NS_CC;

HumanController::HumanController() 
    : _currentInput(Vec2::ZERO)
    , _cameraAngleY(0.0f)
    , _cameraAngleX(20.0f)
    , _cameraDistance(10.0f)
{
    _input = InputSystem::getInstance();
}

HumanController::~HumanController() {
}

Vec2 HumanController::getMoveInput() {
    Vec2 targetInput = Vec2::ZERO;
    
    if (_input->isKeyPressed(GameKey::UP)) targetInput.y = -1; // -Z
    if (_input->isKeyPressed(GameKey::DOWN)) targetInput.y = 1; // +Z
    if (_input->isKeyPressed(GameKey::LEFT)) targetInput.x = -1; // -X
    if (_input->isKeyPressed(GameKey::RIGHT)) targetInput.x = 1; // +X
    
    if (targetInput.length() > 0) {
        targetInput.normalize();
        
        // Rotate input by camera angle so UP is always "forward" relative to camera
        float rad = CC_DEGREES_TO_RADIANS(_cameraAngleY);
        float x = targetInput.x * cos(rad) - targetInput.y * sin(rad);
        float y = targetInput.x * sin(rad) + targetInput.y * cos(rad);
        targetInput.x = x;
        targetInput.y = y;
    }
    
    // Smooth interpolation
    float alpha = 0.2f; // Smoothing factor
    _currentInput = _currentInput * (1.0f - alpha) + targetInput * alpha;
    
    if (_currentInput.length() < 0.05f) return Vec2::ZERO;
    
    return _currentInput;
}

bool HumanController::isSprintPressed() {
    return _input->isKeyPressed(GameKey::SPRINT);
}

bool HumanController::isJumpPressed() {
    return _input->isKeyDown(GameKey::JUMP);
}

bool HumanController::isShootPressed() {
    return _input->isKeyPressed(GameKey::SHOOT);
}

bool HumanController::isPassPressed() {
    return _input->isKeyDown(GameKey::PASS);
}

bool HumanController::isStealPressed() {
    return _input->isKeyDown(GameKey::STEAL);
}

bool HumanController::isCrossoverPressed() {
    return _input->isKeyDown(GameKey::CROSSOVER);
}

bool HumanController::isDefendPressed() {
    // Auto-defend logic + Manual override
    if (_input->isKeyPressed(GameKey::DEFEND)) return true;
    
    // Auto-defend if close to ball handler (simplified: assume we are defending if no ball)
    if (_player && !_player->hasBall()) {
        // Find ball (hacky global search or scene reference, for now just manual)
        // In a real system, we'd check distance to nearest opponent with ball
    }
    
    return false;
}

void HumanController::updateCamera(Camera* camera, float dt) {
    if (!_player || !camera) return;
    
    // Rotate camera with mouse (optional, or just follow)
    // For now, simple follow
    
    Vec3 targetPos = _player->getPosition3D();
    
    // Calculate offset based on angles
    float radY = CC_DEGREES_TO_RADIANS(_cameraAngleY);
    float radX = CC_DEGREES_TO_RADIANS(_cameraAngleX);
    
    float hDist = _cameraDistance * cos(radX);
    float vDist = _cameraDistance * sin(radX);
    
    Vec3 offset(sin(radY) * hDist, vDist, cos(radY) * hDist);
    
    Vec3 cameraPos = targetPos + offset;
    
    // Smooth camera
    // Use Lerp for position smoothing to reduce jitter
    Vec3 currentPos = camera->getPosition3D();
    
    // Camera Jitter Fix (Floor Shaking)
    // Separate damping for Y axis to handle physics floor corrections
    
    float smoothFactor = 10.0f * dt; // Faster horizontal tracking
    if (smoothFactor > 1.0f) smoothFactor = 1.0f;
    
    float smoothYFactor = 2.0f * dt; // Slower vertical tracking to filter noise
    if (smoothYFactor > 1.0f) smoothYFactor = 1.0f;
    
    Vec3 newPos = currentPos;
    
    // Horizontal Lerp
    newPos.x = currentPos.x + (cameraPos.x - currentPos.x) * smoothFactor;
    newPos.z = currentPos.z + (cameraPos.z - currentPos.z) * smoothFactor;
    
    // Vertical Lerp with Deadzone
    // If target Y is close to current Y, don't move (Hysteresis)
    if (std::abs(cameraPos.y - currentPos.y) > 0.02f) {
        newPos.y = currentPos.y + (cameraPos.y - currentPos.y) * smoothYFactor;
    }
    
    camera->setPosition3D(newPos);
    
    // LookAt smoothing
    // Directly looking at targetPos causes jitter if targetPos jitters.
    // We should look at a smoothed point.
    
    static Vec3 smoothLookAtTarget = targetPos;
    // Initialize if far off
    if (smoothLookAtTarget.distance(targetPos) > 5.0f) smoothLookAtTarget = targetPos;
    
    float lookAtSmooth = 10.0f * dt;
    if (lookAtSmooth > 1.0f) lookAtSmooth = 1.0f;
    
    smoothLookAtTarget = smoothLookAtTarget + (targetPos - smoothLookAtTarget) * lookAtSmooth;
    
    // Lock Y lookAt if player is grounded to avoid looking up/down rapidly?
    // For now just smooth is enough.
    
    camera->lookAt(smoothLookAtTarget);
}
