#pragma once
// ============================================================================
// Renderer.h  ——  EasyX 图形渲染
// 对应原版 [M2]~[M7] 绘制：地图、塔、怪物、士兵、特效、UI
// ============================================================================

#include "Game.h"
#include <graphics.h>

// ---------- 颜色工具：0xRRGGBB → EasyX COLORREF ----------
inline COLORREF hx(unsigned int c)
{
    return RGB((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
}
// 亮度混合：t=0 取 a，t=1 取 b
inline unsigned int mixColor(unsigned int a, unsigned int b, double t)
{
    if (t < 0) t = 0; if (t > 1) t = 1;
    int ar=(a>>16)&255, ag=(a>>8)&255, ab=a&255;
    int br=(b>>16)&255, bg=(b>>8)&255, bb=b&255;
    return ((int)(ar+(br-ar)*t)<<16) | ((int)(ag+(bg-ag)*t)<<8) | (int)(ab+(bb-ab)*t);
}

// ---------- UI 命中区域（供 main.cpp 判断点击） ----------
struct Rect {
    int x, y, w, h;
    bool hit(int mx, int my) const {
        return mx >= x && mx <= x + w && my >= y && my <= y + h;
    }
};

// 顶部按钮
const Rect RECT_WAVE    = { 470, 14, 140, 38 };
const Rect RECT_PAUSE   = { 620, 14, 110, 38 };
const Rect RECT_SPEED   = { 740, 14, 110, 38 };
const Rect RECT_RESTART = { 860, 14, 110, 38 };

// 左侧 6 张建塔卡片
inline Rect towerCardRect(int i) { return { 8, 104 + i * 88, 160, 78 }; }
// 右侧 4 张技能卡片
inline Rect skillCardRect(int i) { return { RIGHT_X, 104 + i * 126, 168, 108 }; }

// 塔操作面板与按钮（地图右上角固定位置）
const Rect RECT_PANEL = { MAP_OX + CANVAS_W - 196, MAP_OY + 8, 188, 146 };
inline Rect upgradeBtnRect() { return { RECT_PANEL.x + 8, RECT_PANEL.y + 64, 172, 34 }; }
inline Rect sellBtnRect()    { return { RECT_PANEL.x + 8, RECT_PANEL.y + 104, 172, 34 }; }

// 结算界面按钮
const Rect RECT_OVER_BTN = { WIN_W / 2 - 95, WIN_H / 2 + 80, 190, 50 };

// ============================================================================
class Renderer
{
public:
    explicit Renderer(Game& game);
    ~Renderer();

    void render();                      // 绘制一帧（含 UI 与地图）

private:
    Game& g;
    IMAGE* m_map;                       // 离屏预渲染的静态地图
    int m_dx = 0, m_dy = 0;             // 震屏偏移

    void bakeMap();                     // 预渲染地图到离屏 IMAGE

    // 绘制子模块
    void drawTopBar();
    void drawBuildPanel();
    void drawSkillPanel();
    void drawMapLayer();
    void drawEntities();
    void drawBullets();
    void drawFx();
    void drawAim();
    void drawTowerPanel();
    void drawMessages();
    void drawOverlay();

    // 实体外观
    void drawTower(const Tower& t);
    void drawMonster(const Monster& m);
    void drawSoldier(const Soldier& s);
    void drawBullet(const Bullet& b);
    void drawFxItem(const FXItem& f);

    // 地图外观
    void drawPath(IMAGE& img);
    void drawBase(IMAGE& img);
    void drawGrassPatch(IMAGE& img, double x, double y, double rr, unsigned int col);
    void drawTuft(IMAGE& img, double x, double y);
    void drawFlower(IMAGE& img, double x, double y, int seed);
    void drawPebble(IMAGE& img, double x, double y);
    void drawStump(IMAGE& img, double x, double y);
    void drawBarrel(IMAGE& img, double x, double y);
    void drawHut(IMAGE& img, double x, double y, double scale);

    // 通用
    void text(int x, int y, const TCHAR* s, int size, unsigned int color, bool center = false);
    int  textW(const TCHAR* s, int size);
};

// 工具：简易确定性随机（保证地图装饰每次一致）
unsigned int mulberry32(unsigned int& seed);
