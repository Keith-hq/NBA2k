#ifndef __HUD_H__
#define __HUD_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"

class HUD : public cocos2d::Layer {
public:
    static HUD* create();
    virtual bool init() override;

    void updateScore(int playerScore, int aiScore);
    void updateTime(float gameTime);
    void updateShotClock(float shotClock);
    void showFeedback(const std::string& text);

    // Responsive Layout
    void onResize(const cocos2d::Size& size);

private:
    cocos2d::ui::Text* _playerScoreText;
    cocos2d::ui::Text* _aiScoreText;
    cocos2d::ui::Text* _gameTimeText;
    cocos2d::ui::Text* _shotClockText;
    cocos2d::ui::Text* _feedbackText;
    
    void setupUI();
};

#endif // __HUD_H__
