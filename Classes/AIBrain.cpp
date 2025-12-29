#include "AIBrain.h"

USING_NS_CC;

AIBrain::AIBrain(Difficulty difficulty) 
    : _difficulty(difficulty)
    , _state(State::TRANSITION)
    , _reactionTimer(0.0f)
    , _stealTimer(0.0f)
    , _isShooting(false)
    , _shotTimer(0.0f)
    , _hasJumpedForShot(false)
    , _defenseReactionTimer(0.0f)
    , _isReactingToShot(false)
    , _firstOffenseFrame(false)
{
    switch (_difficulty) {
        case Difficulty::EASY:
            _reactionInterval = 0.3f; // Faster (was 0.5)
            break;
        case Difficulty::NORMAL:
            _reactionInterval = 0.1f; // Faster (was 0.2)
            break;
        case Difficulty::HARD:
            _reactionInterval = 0.0f; // Instant (was 0.05)
            break;
    }
}

AIBrain::~AIBrain() {}

void AIBrain::update(const InputData& input) {
    // Reaction delay
    _reactionTimer -= input.dt;
    if (_stealTimer > 0) _stealTimer -= input.dt;
    
    // Bypass reaction timer if:
    // 1. First frame of offense (Instant reaction)
    // 2. Currently Shooting (Need real-time updates for shot timer)
    bool bypassReaction = _firstOffenseFrame || _isShooting;
    
    if (_reactionTimer > 0 && !bypassReaction) {
        return;
    }
    
    // Only reset timer if it actually expired (not just bypassed)
    // Or should we reset it anyway to keep cadence?
    // If we are shooting, we update every frame. Reaction timer becomes irrelevant until we stop shooting.
    if (_reactionTimer <= 0) {
        _reactionTimer = _reactionInterval; 
    }
    
    _firstOffenseFrame = false; // Consumed
    
    // Update State
    updateState(input);
    
    // Reset momentary outputs
    _output.jump = false;
    _output.steal = false;
    // _output.shoot handled in offense logic (hold)
    
    switch (_state) {
        case State::OFFENSE:
            processOffense(input);
            break;
        case State::DEFENSE:
            processDefense(input);
            break;
        case State::TRANSITION:
            processTransition(input);
            break;
    }
}

void AIBrain::updateState(const InputData& input) {
    State oldState = _state;
    if (input.hasBall) {
        _state = State::OFFENSE;
    } else if (input.opponentHasBall) {
        _state = State::DEFENSE;
    } else {
        _state = State::TRANSITION;
    }
    
    // Reset flags on state change
    if (_state != oldState) {
        if (_state == State::OFFENSE) {
            _isShooting = false;
            _shotTimer = 0.0f;
            _hasJumpedForShot = false;
            _reactionTimer = 0.0f;
            _firstOffenseFrame = true; // Force immediate reaction
            CCLOG("AIBrain: Entered OFFENSE");
        } else if (_state == State::DEFENSE) {
            _defenseReactionTimer = 0.0f;
            _isReactingToShot = false;
        }
    }
}

void AIBrain::processOffense(const InputData& input) {
    // 0. Handle Clear Ball Rule
    if (input.needsClear) {
        // Move away from hoop (towards center court)
        // Hoop is at -Z. Center is at 0,0,0.
        // We just move towards (0,0,0) or (0,0,10) to be safe.
        cocos2d::Vec3 centerTarget(0, 0, 10.0f);
        _output.moveDir = getDirectionTo(input.selfPos, centerTarget);
        _output.sprint = true;
        _output.shoot = false;
        return;
    }

    float distToHoop = getDistance2D(input.selfPos, input.hoopPos);
    
    // 1. Handle Shooting State
    if (_isShooting) {
        _shotTimer += input.dt;
        _output.moveDir = cocos2d::Vec2::ZERO;
        _output.sprint = false;
        
        // Failsafe: Prevent getting stuck indefinitely
        if (_shotTimer > 3.0f) {
            _isShooting = false;
            _output.shoot = false;
            CCLOG("AIBrain: Shot Failsafe Triggered");
            return;
        }
        
        // Target time based on User Request (1.0s optimal)
        // AI skill determines deviation
        float targetTime = 1.0f;
        
        // Shoot logic...
        // For debugging, let's log timer occasionally
        // CCLOG("AIBrain: Shot Timer: %.2f / %.2f", _shotTimer, targetTime);
        
        if (_shotTimer >= targetTime) {
            _output.shoot = false; // Release
            _isShooting = false;
            CCLOG("AIBrain: Released Shot at %.2f", _shotTimer);
        } else {
            _output.shoot = true; // Hold
        }
        return;
    }

    // Offense Logic: Move to hoop, shoot if open
    
    // Default movement: Drive to hoop
    cocos2d::Vec2 dirToHoop = getDirectionTo(input.selfPos, input.hoopPos);
    
    // Ensure we are moving by default (will be overridden by logic below if needed)
    _output.moveDir = dirToHoop;
    _output.sprint = true;

    float distToOpponent = getDistance2D(input.selfPos, input.opponentPos);
    
    // Check if we should shoot
    // User Request: Shoot when distance to player is greater than a certain range.
    // We define "Safe Range" as 1.5 meters (Reduced from 2.0f for more aggression).
    float safeRange = 1.5f;
    
    // Condition: In range (< 7.5f), and Open (opponent > safeRange), facing hoop
    // We relaxed the hoop distance slightly to allow mid-range/3pt shots if open.
    bool canShoot = (distToHoop < 7.5f) && (distToOpponent > safeRange);
    
    // Hard AI is more aggressive / can shoot with less space
    if (_difficulty == Difficulty::HARD) {
        if (distToHoop < 8.0f && distToOpponent > 1.0f) canShoot = true;
    } else if (_difficulty == Difficulty::EASY) {
        // Easy AI needs more space
         if (distToOpponent < 2.5f) canShoot = false;
    }
    
    // If we are "Wide Open" (very far from defender), shoot even if further out
    if (distToOpponent > 4.0f && distToHoop < 9.0f) {
        canShoot = true;
    }
    
    // Force shoot if very close to hoop (Layup/Dunk range)
    if (distToHoop < 1.5f && distToOpponent > 0.5f) {
        canShoot = true;
    }

    // Shot Clock Panic
    // If shot clock is running out (< 4 seconds), force action
    if (input.shotClock < 4.0f) {
        // If reasonably close, just shoot
        if (distToHoop < 10.0f) {
            canShoot = true;
        } else {
             // Too far, must drive
             // handled by move logic, but ensure we don't just stand
        }
        CCLOG("AIBrain: Shot Clock Panic! Time: %.2f", input.shotClock);
    }
    
    if (canShoot) {
        // Start Shooting
        _isShooting = true;
        _shotTimer = 0.0f;
        _output.moveDir = cocos2d::Vec2::ZERO;
        _output.sprint = false;
        _output.shoot = true; 
    } else {
        _output.shoot = false;
        
        // Move Logic
        Vec2 moveDir = cocos2d::Vec2::ZERO;

        // More aggressive driving:
        // If not shooting, always try to move towards hoop unless completely blocked
        // Reduced "Defended" threshold to allow driving into contact
        
        if (distToOpponent < 1.0f) { // Only divert if VERY close
            // Opponent is close (Defended)
            // Strategy: Crossover / Drive Away
            
            // Vector from Opponent to Self (Retreat direction)
            Vec2 fromOpponent = getDirectionTo(input.opponentPos, input.selfPos);
            
            // We want to move towards hoop but away from opponent
            // Calculate a "Drive Lane"
            // Cross product to find perpendicular vector (Side step)
            Vec2 sideStep(dirToHoop.y, -dirToHoop.x); // Right
            
            // Check which side is more open?
            // Simple: Move away from opponent's projected side
            
            // If opponent is directly between us and hoop, we need to go around
            // Mix: 40% Towards Hoop, 60% Away from Opponent/Side
            
            // Dynamic Side Step:
            // Check cross product of (Self->Hoop) x (Self->Opponent)
            // If result is Positive (Opponent is Right), we go Left.
            // If result is Negative (Opponent is Left), we go Right.
            
            Vec2 toHoop = dirToHoop;
            Vec2 toOpp = getDirectionTo(input.selfPos, input.opponentPos);
            float cross = toHoop.x * toOpp.y - toHoop.y * toOpp.x;
            
            Vec2 sideDir;
            // Improved Crossover Logic:
            // Add randomness to side selection to simulate "shaking" the defender
            // 20% chance to go against the logical grain (Crossover)
            bool doCrossover = (CCRANDOM_0_1() < 0.2f);
            
            if ((cross > 0 && !doCrossover) || (cross <= 0 && doCrossover)) {
                // Go Left (Rotate +90)
                sideDir = Vec2(-toHoop.y, toHoop.x);
            } else {
                // Go Right (Rotate -90)
                sideDir = Vec2(toHoop.y, -toHoop.x);
            }

            // Apply Crossover Action
            // If we are changing direction or blocked, use Crossover
            if (doCrossover || distToOpponent < 0.8f) {
                _output.crossover = true;
            }

            // More aggressive drive: Bias towards side movement to shake off
            // Was 0.5 forward, 0.8 side. 
            // New: 0.6 forward, 0.8 side -> Keep pushing forward
            moveDir = (dirToHoop.getNormalized() * 0.6f + sideDir.getNormalized() * 0.8f).getNormalized();
            _output.sprint = true; // Sprint to blow by
            
            // Step Back Logic:
            // If very close (< 1.0m) and facing hoop, maybe step back to create space?
            if (distToOpponent < 0.8f && CCRANDOM_0_1() < 0.05f) { // 5% chance per frame when close
                 moveDir = -dirToHoop; // Step back
                 _output.sprint = true; // Fast step back
            }
            
        } else {
            // Open path to hoop?
            // Just go straight
            moveDir = dirToHoop;
            _output.sprint = true; // Always sprint on offense to pressure
            
            // If very close to hoop, stop sprinting to stabilize for shot?
            if (distToHoop < 2.0f) _output.sprint = false;
        }

        // Safety: Ensure we always move if not shooting
        if (moveDir.length() < 0.1f) {
            // If somehow zero, move to hoop
             moveDir = dirToHoop;
             if (moveDir.length() < 0.1f) {
                 // Already at hoop? Move back to clear space?
                 // Or just move random
                 moveDir = Vec2(1, 0); 
             }
        }
        _output.moveDir = moveDir;
    }
}

void AIBrain::processDefense(const InputData& input) {
    _output.shoot = false; // Ensure not shooting
    _output.sprint = false;
    
    // 1. Calculate Positioning
    // Goal: Stay between opponent and hoop, but mirror opponent's movement
    
    Vec2 oppToHoop = getDirectionTo(input.opponentPos, input.hoopPos);
    
    // Dynamic Guard Distance
    // If opponent is far from hoop, sag off (give 2.0m space)
    // If opponent is close to hoop, tighten up (1.0m space)
    float distOppToHoop = getDistance2D(input.opponentPos, input.hoopPos);
    float guardDist = 1.5f;
    
    if (distOppToHoop > 8.0f) guardDist = 2.0f; // Sag off
    else if (distOppToHoop < 4.0f) guardDist = 1.0f; // Tight
    
    // Velocity Adjustment: Shift ideal position based on opponent velocity
    Vec2 oppVel2D(input.opponentVel.x, input.opponentVel.z);
    
    // Base Ideal Position
    Vec3 baseIdealPos = input.opponentPos + Vec3(oppToHoop.x, 0, oppToHoop.y) * guardDist;
    
    // Add velocity prediction offset (Anticipation)
    // Cut off drive: Place target slightly in front of opponent's movement direction
    Vec3 targetPos = baseIdealPos;
    if (oppVel2D.length() > 0.5f) {
        Vec2 moveDir = oppVel2D.getNormalized();
        // Shift target in the direction of opponent's movement to cut them off
        targetPos += Vec3(moveDir.x, 0, moveDir.y) * 0.5f; 
    }
    
    // Move towards target
    Vec2 dirToTarget = getDirectionTo(input.selfPos, targetPos);
    float distToTarget = getDistance2D(input.selfPos, targetPos);
    
    if (distToTarget > 0.2f) {
        _output.moveDir = dirToTarget;
        // Sprint if falling behind or opponent is sprinting (moving fast)
        if (distToTarget > 2.0f || oppVel2D.length() > 4.0f) {
            _output.sprint = true;
        }
    } else {
        _output.moveDir = Vec2::ZERO; // Hold ground
    }
    
    // 2. Stance and Distance Logic
    float distToOpponent = getDistance2D(input.selfPos, input.opponentPos);
    if (distToOpponent < 3.0f) {
        _output.defend = true;
    } else {
        _output.defend = false;
    }
    
    // 3. Block / Contest
    if (input.opponentIsShooting) {
        // Close out if far
        if (distToOpponent > 1.0f) {
            _output.moveDir = getDirectionTo(input.selfPos, input.opponentPos);
            _output.sprint = true;
        }
        
        // Block Logic with Reaction Delay
        if (!_hasJumpedForShot && !_isReactingToShot) {
             // Start Reaction Timer
             _isReactingToShot = true;
             
             // Reaction time based on difficulty (plus some randomness)
             // Simulates human reaction time (0.2s - 0.4s usually)
             float baseDelay = 0.2f;
             if (_difficulty == Difficulty::EASY) baseDelay = 0.4f;
             else if (_difficulty == Difficulty::HARD) baseDelay = 0.15f; // Even hard AI has limit
             
             _defenseReactionTimer = baseDelay + CCRANDOM_0_1() * 0.15f;
        }
        
        if (_isReactingToShot) {
            _defenseReactionTimer -= input.dt;
            if (_defenseReactionTimer <= 0) {
                // Reaction complete, make decision
                _isReactingToShot = false;
                
                // Check if still worth jumping
                // Only jump if close enough (1.2m) - tighter range for jump blocks
                if (distToOpponent < 1.2f) {
                     // Check probability (Bite on pump fake / Aggressiveness)
                     float jumpChance = 0.8f;
                     if (_difficulty == Difficulty::EASY) jumpChance = 0.3f;
                     
                     // Distance modifier: if very close, more likely to jump
                     if (distToOpponent < 0.8f) jumpChance += 0.2f;
                     
                     if (CCRANDOM_0_1() < jumpChance) {
                         _output.jump = true;
                         _hasJumpedForShot = true;
                     } else {
                         // Decided not to jump (just contest on ground)
                         _hasJumpedForShot = true; // Mark as "handled" so we don't try again
                     }
                } else {
                    // Too far to block, don't jump, just run closer
                     _hasJumpedForShot = true; // Don't try to jump again for this shot
                }
            }
        }
    } else {
        // Reset flags when opponent is not shooting
        _hasJumpedForShot = false;
        _isReactingToShot = false;
        _defenseReactionTimer = 0.0f;
    }
    
    // 4. Steal Logic
    if (distToOpponent < 1.5f && !input.opponentIsShooting && _stealTimer <= 0) {
        // Attempt steal if opponent is vulnerable or randomly
        float roll = CCRANDOM_0_1();
        float threshold = 0.005f; // Low chance per frame
        if (_difficulty == Difficulty::HARD) threshold = 0.02f;
        
        if (roll < threshold) { 
            _output.steal = true;
            _stealTimer = 3.0f; // Cooldown
        }
    }
}

void AIBrain::processTransition(const InputData& input) {
    _output.shoot = false;
    _output.defend = false;
    
    // Chase Ball
    Vec2 dirToBall = getDirectionTo(input.selfPos, input.ballPos);
    _output.moveDir = dirToBall;
    _output.sprint = true; // Always sprint to loose ball
    
    // If ball is high up (rebound), try to jump
    if (input.ballPos.y > 2.0f && getDistance2D(input.selfPos, input.ballPos) < 1.0f) {
        _output.jump = true;
    }
}

Vec2 AIBrain::getDirectionTo(const Vec3& from, const Vec3& to) {
    Vec2 diff(to.x - from.x, to.z - from.z);
    if (diff.length() > 0.01f) {
        return diff.getNormalized();
    }
    return Vec2::ZERO;
}

float AIBrain::getDistance(const Vec3& a, const Vec3& b) {
    return a.distance(b);
}

float AIBrain::getDistance2D(const Vec3& a, const Vec3& b) {
    return Vec2(a.x - b.x, a.z - b.z).length();
}
