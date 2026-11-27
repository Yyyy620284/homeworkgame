// ============================================================================
// Renderer.cpp  ——  EasyX 图形渲染实现
// ============================================================================

#include "Renderer.h"
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <vector>

unsigned int mulberry32(unsigned int& seed)
{
    seed += 0x6D2B79F5u;
    unsigned int t = seed;
    t = (t ^ (t >> 15)) * (t | 1);
    t ^= t + ((t ^ (t >> 7)) * (t | 61));
    return (t ^ (t >> 14)) & 0xffffffffu;
}

// ============================================================================
// 构造 / 析构
// ============================================================================
Renderer::Renderer(Game& game) : g(game)
{
    m_map = new IMAGE(CANVAS_W, CANVAS_H);
    bakeMap();
}
Renderer::~Renderer()
{
    delete m_map;
}

// ============================================================================
// 文本工具
// ============================================================================
void Renderer::text(int x, int y, const TCHAR* s, int size, unsigned int color, bool center)
{
    settextstyle(size, 0, _T("Microsoft YaHei"));
    settextcolor(hx(color));
    setbkmode(TRANSPARENT);
    if (center) {
        int w = textwidth(s);
        outtextxy(x - w / 2, y, s);
    } else {
        outtextxy(x, y, s);
    }
}
int Renderer::textW(const TCHAR* s, int size)
{
    settextstyle(size, 0, _T("Microsoft YaHei"));
    return textwidth(s);
}

// ============================================================================
// 地图预渲染：草地 / 路径 / 基地 / 装饰
// ============================================================================
void Renderer::drawGrassPatch(IMAGE& img, double x, double y, double rr, unsigned int col)
{
    SetWorkingImage(&img);
    setfillcolor(hx(col));
    fillellipse((int)(x - rr), (int)(y - rr * 0.65),
                (int)(x + rr), (int)(y + rr * 0.65));
}
void Renderer::drawTuft(IMAGE& img, double x, double y)
{
    SetWorkingImage(&img);
    setlinecolor(hx(0x356820));
    setlinewidth(2);
    for (int i = -1; i <= 1; ++i) {
        line((int)x, (int)y, (int)(x + i * 5 - 2), (int)(y - 9 - (i == 0 ? 3 : 0)));
    }
    setlinecolor(hx(0x4e8a2c));
    line((int)x + 1, (int)y, (int)(x + 6), (int)(y - 7));
}
void Renderer::drawFlower(IMAGE& img, double x, double y, int seed)
{
    SetWorkingImage(&img);
    setlinecolor(hx(0x3a7020));
    setlinewidth(2);
    line((int)x, (int)y, (int)x, (int)(y - 10));
    unsigned int petals[] = { 0xff8ab0, 0xffd040, 0xe0a0ff, 0xffffff };
    setfillcolor(hx(petals[seed % 4]));
    fillcircle((int)x, (int)(y - 12), 3);
    setfillcolor(hx(0xffe066));
    fillcircle((int)x, (int)(y - 12), 1);
}
void Renderer::drawPebble(IMAGE& img, double x, double y)
{
    SetWorkingImage(&img);
    setfillcolor(hx(0x9aa090));
    fillcircle((int)x, (int)y, 4);
    setfillcolor(hx(0xb8bcae));
    fillcircle((int)(x - 1), (int)(y - 1), 2);
}
void Renderer::drawStump(IMAGE& img, double x, double y)
{
    SetWorkingImage(&img);
    setfillcolor(hx(0x6a4a28));
    fillrect((int)(x - 6), (int)(y - 4), (int)(x + 6), (int)(y + 4));
    setfillcolor(hx(0x8a6238));
    fillellipse((int)(x - 6), (int)(y - 7), (int)(x + 6), (int)(y - 1));
}
void Renderer::drawBarrel(IMAGE& img, double x, double y)
{
    SetWorkingImage(&img);
    setfillcolor(hx(0x8a5a2a));
    fillrect((int)(x - 10), (int)(y - 14), (int)(x + 10), (int)(y + 14));
    setlinecolor(hx(0x3a2410));
    setlinewidth(2);
    rectangle((int)(x - 10), (int)(y - 14), (int)(x + 10), (int)(y + 14));
    line((int)(x - 10), (int)(y - 5), (int)(x + 10), (int)(y - 5));
    line((int)(x - 10), (int)(y + 5), (int)(x + 10), (int)(y + 5));
}
void Renderer::drawHut(IMAGE& img, double x, double y, double scale)
{
    SetWorkingImage(&img);
    int w = (int)(36 * scale), h = (int)(26 * scale);
    setfillcolor(hx(0xc89a58));
    fillrect((int)(x - w/2), (int)(y - h/2), (int)(x + w/2), (int)(y + h/2));
    setlinecolor(hx(0x4a2e14));
    setlinewidth(2);
    rectangle((int)(x - w/2), (int)(y - h/2), (int)(x + w/2), (int)(y + h/2));
    // 屋顶
    POINT roof[3];
    roof[0] = { (int)(x - w/2 - 5), (int)(y - h/2) };
    roof[1] = { (int)(x + w/2 + 5), (int)(y - h/2) };
    roof[2] = { (int)x, (int)(y - h/2 - 22) };
    setfillcolor(hx(0x8a3a20));
    fillpolygon(roof, 3);
    // 门
    setfillcolor(hx(0x3a2410));
    fillrect((int)(x - 6), (int)(y + 2), (int)(x + 6), (int)(y + h/2));
}

void Renderer::drawPath(IMAGE& img)
{
    SetWorkingImage(&img);
    // 外描边（深棕）+ 泥土主体 + 高光
    const int widths[3] = { (int)(CELL * 0.96), (int)(CELL * 0.84), (int)(CELL * 0.60) };
    const unsigned int cols[3] = { 0x4a3018, 0x8a5a30, 0xb08850 };
    for (int layer = 0; layer < 3; ++layer) {
        setlinecolor(hx(cols[layer]));
        setlinewidth(widths[layer]);
        for (int i = 0; i < PATH_N - 1; ++i) {
            Point a = Game::pathPoint(i), b = Game::pathPoint(i + 1);
            line((int)a.x, (int)a.y, (int)b.x, (int)b.y);
        }
    }
    // 石子点缀（沿路径）
    unsigned int seed = 12345;
    for (int k = 0; k < 120; ++k) {
        double t = (mulberry32(seed) / (double)0xffffffff) * (PATH_N - 1);
        int seg = (int)t;
        if (seg >= PATH_N - 1) seg = PATH_N - 2;
        double f = t - seg;
        Point a = Game::pathPoint(seg), b = Game::pathPoint(seg + 1);
        double px = a.x + (b.x - a.x) * f;
        double py = a.y + (b.y - a.y) * f;
        px += (mulberry32(seed) / (double)0xffffffff - 0.5) * CELL * 0.5;
        py += (mulberry32(seed) / (double)0xffffffff - 0.5) * CELL * 0.5;
        setfillcolor(hx(k % 2 ? 0x9a7040 : 0x6a4a28));
        fillcircle((int)px, (int)py, 1 + (k % 2));
    }
}

void Renderer::drawBase(IMAGE& img)
{
    SetWorkingImage(&img);
    Point p = Game::cellCenter(BASE_CELL[0], BASE_CELL[1]);
    int x = (int)p.x, y = (int)p.y;
    // 阴影
    setfillcolor(hx(0x2a2010));
    fillellipse(x - 30, y + 8, x + 30, y + 20);
    // 城墙
    setfillcolor(hx(0xc0b8a0));
    fillrect(x - 24, y - 20, x + 24, y + 12);
    setlinecolor(hx(0x5a5038)); setlinewidth(2);
    rectangle(x - 24, y - 20, x + 24, y + 12);
    // 城垛
    for (int i = -2; i <= 2; ++i)
        fillrect(x + i * 10 - 4, y - 26, x + i * 10 + 3, y - 18);
    // 门洞
    setfillcolor(hx(0x3a2410));
    fillrect(x - 8, y - 4, x + 8, y + 12);
    // 旗帜
    setlinecolor(hx(0x4a3018)); setlinewidth(3);
    line(x, y - 26, x, y - 52);
    POINT flag[3] = { {x, y-52}, {x+22, y-46}, {x, y-38} };
    setfillcolor(hx(0xc83020)); fillpolygon(flag, 3);
}

void Renderer::bakeMap()
{
    IMAGE* old = GetWorkingImage();
    SetWorkingImage(m_map);

    // 草地渐变（横向色带模拟）
    unsigned int top = 0x7db848, bot = 0x559030;
    for (int y = 0; y < CANVAS_H; ++y) {
        unsigned int c = mixColor(top, bot, y / (double)CANVAS_H);
        setfillcolor(hx(c));
        line(0, y, CANVAS_W, y);
    }
    // 草地斑块
    unsigned int seed = 20240101;
    for (int i = 0; i < 90; ++i) {
        double x = mulberry32(seed) / (double)0xffffffff * CANVAS_W;
        double y = mulberry32(seed) / (double)0xffffffff * CANVAS_H;
        double rr = 18 + mulberry32(seed) / (double)0xffffffff * 46;
        unsigned int c = (mulberry32(seed) & 1) ? 0x8ec858 : 0x4e8828;
        drawGrassPatch(*m_map, x, y, rr, c);
    }

    drawPath(*m_map);
    drawBase(*m_map);

    // 装饰：避开路径格与基地周围
    seed = 777;
    auto cellFree = [&](int c, int r) {
        if (c < 0 || r < 0 || c >= COLS || r >= ROWS) return false;
        // 路径格判定：与 g_pathCell 一致，这里用简化判断——离路径折线太近则跳过
        Point pp = Game::cellCenter(c, r);
        for (int i = 0; i < PATH_N - 1; ++i) {
            Point a = Game::pathPoint(i), b = Game::pathPoint(i + 1);
            double dx = b.x - a.x, dy = b.y - a.y;
            double L2 = dx*dx + dy*dy; if (L2 < 1) L2 = 1;
            double t = ((pp.x-a.x)*dx + (pp.y-a.y)*dy) / L2;
            if (t < 0) t = 0; if (t > 1) t = 1;
            double d = std::hypot(a.x + dx*t - pp.x, a.y + dy*t - pp.y);
            if (d < CELL * 0.7) return false;
        }
        if (std::abs(c - BASE_CELL[0]) <= 1 && std::abs(r - BASE_CELL[1]) <= 1) return false;
        return true;
    };

    for (int i = 0; i < 150; ++i) {
        int c = (int)(mulberry32(seed) / (double)0xffffffff * COLS);
        int r = (int)(mulberry32(seed) / (double)0xffffffff * ROWS);
        if (!cellFree(c, r)) continue;
        double x = PAD + c * CELL + 8 + mulberry32(seed) / (double)0xffffffff * (CELL - 16);
        double y = PAD + r * CELL + 8 + mulberry32(seed) / (double)0xffffffff * (CELL - 16);
        double t = mulberry32(seed) / (double)0xffffffff;
        if (t < 0.42) drawTuft(*m_map, x, y);
        else if (t < 0.72) drawFlower(*m_map, x, y, (int)seed);
        else if (t < 0.88) drawPebble(*m_map, x, y);
        else drawStump(*m_map, x, y);
    }
    drawBarrel(*m_map, PAD + 30, PAD + 8 * CELL + 30);
    drawHut(*m_map, PAD + 11.5 * CELL, PAD + 0.5 * CELL, 1.0);
    drawHut(*m_map, PAD + 5.5 * CELL, PAD + 8.4 * CELL, 0.85);

    SetWorkingImage(old);
}

// ============================================================================
// 顶部资源条
// ============================================================================
void Renderer::drawTopBar()
{
    // 木质面板
    setfillcolor(hx(0x7a4e22));
    fillrect(0, 0, WIN_W, TOP_H);
    setfillcolor(hx(0xa8763c));
    fillrect(0, 0, WIN_W, 8);
    setlinecolor(hx(0x3a2410)); setlinewidth(3);
    line(0, TOP_H, WIN_W, TOP_H);

    // 资源数字
    TCHAR buf[64];
    swprintf_s(buf, 64, _T("金币 %d"), g.coin);
    text(20, 20, buf, 20, 0xffe066);
    swprintf_s(buf, 64, _T("生命 %d"), g.hp);
    text(180, 20, buf, 20, 0xff8a7a);
    swprintf_s(buf, 64, _T("波次 %d / %d"), g.wave, WAVE_MAX);
    text(330, 20, buf, 20, 0xb8e0ff);

    // 按钮
    auto button = [&](const Rect& r, const TCHAR* label, unsigned int col, bool enabled = true) {
        setfillcolor(hx(enabled ? col : 0x555555));
        fillroundrect(r.x, r.y, r.x + r.w, r.y + r.h, 8);
        setlinecolor(hx(0x2a1808)); setlinewidth(2);
        roundrect(r.x, r.y, r.x + r.w, r.y + r.h, 8);
        text(r.x + r.w / 2, r.y + 8, label, 16, 0xffffff, true);
    };

    TCHAR waveLabel[32];
    if (g.waveActive || g.wave >= WAVE_MAX) {
        _tcscpy_s(waveLabel, 32, g.wave >= WAVE_MAX ? _T("全部完成") : _T("战斗中"));
    } else {
        swprintf_s(waveLabel, 32, _T("开始第%d波"), g.wave + 1);
    }
    button(RECT_WAVE, waveLabel, 0xd08a20, !g.waveActive && !g.over && g.wave < WAVE_MAX);
    button(RECT_PAUSE, g.paused ? _T("继续") : _T("暂停"), 0x3a78c8);
    TCHAR spd[16]; swprintf_s(spd, 16, _T("速度 x%d"), g.speed);
    button(RECT_SPEED, spd, 0x9858c8);
    button(RECT_RESTART, _T("重开"), 0xc87820);
}

// ============================================================================
// 左侧建造塔面板
// ============================================================================
void Renderer::drawBuildPanel()
{
    setfillcolor(hx(0x2a4a24));
    fillrect(0, TOP_H, LEFT_W, WIN_H - TOP_H);
    text(LEFT_W / 2, TOP_H + 12, _T("建 造 塔"), 18, 0xffe9a0, true);

    const TCHAR* icons[TK_COUNT] = { _T("弓"), _T("法"), _T("兵"), _T("炮"), _T("冰"), _T("狙") };
    for (int i = 0; i < TK_COUNT; ++i) {
        Rect r = towerCardRect(i);
        const TowerDef& d = TOWER_DATA[i];
        bool sel = (g.selectedTowerKey == i);
        bool afford = (g.coin >= d.buildCost);

        setfillcolor(hx(sel ? 0xf0c050 : (afford ? 0xc89a50 : 0x8a7a60)));
        fillroundrect(r.x, r.y, r.x + r.w, r.y + r.h, 8);
        setlinecolor(hx(sel ? 0xffe066 : 0x5a3a14)); setlinewidth(sel ? 3 : 2);
        roundrect(r.x, r.y, r.x + r.w, r.y + r.h, 8);

        text(r.x + 8, r.y + 6, icons[i], 18, 0x3a2410);
        text(r.x + 34, r.y + 6, d.name, 16, 0x2a1a08);
        TCHAR cost[32]; swprintf_s(cost, 32, _T("金币 %d"), d.buildCost);
        text(r.x + 8, r.y + 32, cost, 14, 0x8a4a08);
        // 描述可能较长，截断前若干字
        text(r.x + 8, r.y + 54, d.desc, 11, 0x5a4020);
    }
    // 底部提示
    text(8, WIN_H - 54, _T("选塔后点地图空地建造"), 12, 0xd8e8c0);
    text(8, WIN_H - 34, _T("右键取消选择"), 12, 0xd8e8c0);
}

// ============================================================================
// 右侧技能面板
// ============================================================================
void Renderer::drawSkillPanel()
{
    setfillcolor(hx(0x1a2e18));
    fillrect(RIGHT_X, TOP_H, WIN_W - RIGHT_X, WIN_H - TOP_H);
    text(RIGHT_X + (WIN_W - RIGHT_X) / 2, TOP_H + 12, _T("英 雄 技 能"), 18, 0xffe9a0, true);

    const TCHAR* icons[SK_COUNT] = { _T("陨"), _T("雷"), _T("兵"), _T("焰") };
    for (int i = 0; i < SK_COUNT; ++i) {
        Rect r = skillCardRect(i);
        const SkillDef& d = SKILL_DATA[i];
        double cd = g.itemCd[i];
        bool cooling = cd > 0;
        bool aiming = (g.pendingSkill == i);
        bool afford = (g.coin >= d.cost);

        setfillcolor(hx(cooling ? 0x3a3040 : (aiming ? 0x6a5a2a : 0x3a2a60)));
        fillroundrect(r.x, r.y, r.x + r.w, r.y + r.h, 8);
        setlinecolor(hx(aiming ? 0xffe066 : 0x1a1030)); setlinewidth(aiming ? 3 : 2);
        roundrect(r.x, r.y, r.x + r.w, r.y + r.h, 8);

        text(r.x + 8, r.y + 8, icons[i], 18, cooling ? 0x8878a0 : 0xd8c8ff);
        text(r.x + 34, r.y + 8, d.name, 15, cooling ? 0x8878a0 : 0xd8c8ff);

        TCHAR buf[64];
        swprintf_s(buf, 64, _T("金币 %d  CD %d"), d.cost, d.cd);
        text(r.x + 8, r.y + 36, buf, 12, cooling ? 0x8878a0 : 0xffe066);
        // 描述
        text(r.x + 8, r.y + 58, d.desc, 10, 0xb0a0d0);

        if (cooling) {
            int sec = (int)std::ceil(cd);
            swprintf_s(buf, 64, _T("%d"), sec);
            text(r.x + r.w / 2, r.y + r.h / 2 - 10, buf, 26, 0xffffff, true);
            // 冷却进度条
            setfillcolor(hx(0x8a6ae8));
            fillrect(r.x, r.y + r.h - 5, r.x + (int)(r.w * (cd / d.cd)), r.y + r.h);
        }
    }
    text(RIGHT_X + 8, WIN_H - 34, _T("点技能后点地图释放"), 12, 0xd8e8c0);
}

// ============================================================================
// 防御塔外观：6 种塔 × 4 级
// ============================================================================
void Renderer::drawTower(const Tower& t)
{
    int x = (int)(t.x + MAP_OX + m_dx);
    int y = (int)(t.y + MAP_OY + m_dy);
    int lv = t.level;
    double breath = 1 + 0.03 * std::sin(g.time * 3 + t.breathPhase);

    // 基座阴影
    setfillcolor(hx(0x2a2010));
    fillellipse(x - 24, y + 12, x + 24, y + 22);

    if (t.key == TK_ARCHER) {
        // 树桩 + 树冠（等级越高树越大）
        int trunk = 16 + lv * 3;
        setfillcolor(hx(0x6a4420));
        fillrect(x - 8, y - 6, x + 8, y + 14);
        setlinecolor(hx(0x3a2410)); setlinewidth(2);
        rectangle(x - 8, y - 6, x + 8, y + 14);
        int crown = (int)((18 + lv * 5) * breath);
        setfillcolor(hx(0x3a7a28));
        fillcircle(x, y - trunk, crown);
        setfillcolor(hx(0x4e9434));
        fillcircle(x - crown / 3, y - trunk - crown / 4, crown / 2);
        setfillcolor(hx(0x66ad40));
        fillcircle(x + crown / 4, y - trunk - crown / 3, crown / 3);
        // 弓箭手（小红帽）
        setfillcolor(hx(0xe8c8a0));
        fillcircle(x, y - trunk + 2, 5);
        setfillcolor(hx(lv >= 3 ? 0xc83020 : 0x2a6a20));
        fillcircle(x, y - trunk - 2, 5);
        // 满级皇冠
        if (lv == 4) {
            setfillcolor(hx(0xffd040));
            POINT crownPts[5] = { {x-10,y-trunk-crown-4},{x-6,y-trunk-crown-12},{x,y-trunk-crown-6},
                                  {x+6,y-trunk-crown-12},{x+10,y-trunk-crown-4} };
            fillpolygon(crownPts, 5);
        }
    }
    else if (t.key == TK_MAGE) {
        // 石柱 + 紫水晶
        setfillcolor(hx(0x887a94));
        fillrect(x - 12, y - 20, x + 12, y + 14);
        setlinecolor(hx(0x3a3048)); setlinewidth(2);
        rectangle(x - 12, y - 20, x + 12, y + 14);
        setfillcolor(hx(0x5a4a70));
        fillrect(x - 14, y - 24, x + 14, y - 18);
        int cr = 8 + lv * 2;
        setfillcolor(hx(0xa060e8));
        POINT gem[4] = { {x, y-20-cr},{x+cr, y-20},{x, y-20+cr/2},{x-cr, y-20} };
        fillpolygon(gem, 4);
        setfillcolor(hx(0xe0b0ff));
        POINT gem2[3] = { {x, y-20-cr},{x+cr/2, y-20},{x, y-20} };
        fillpolygon(gem2, 3);
        // 浮动光点
        for (int i = 0; i < lv; ++i) {
            double a = g.time * 2 + i * 2.1;
            setfillcolor(hx(0xd8a0ff));
            fillcircle((int)(x + std::cos(a) * 20), (int)(y - 20 + std::sin(a) * 12), 2);
        }
    }
    else if (t.key == TK_BARRACKS) {
        // 帐篷 + 旗帜
        setfillcolor(hx(0xc8a040));
        POINT tent[3] = { {x-20, y+14}, {x+20, y+14}, {x, y-16-lv*2} };
        fillpolygon(tent, 3);
        setlinecolor(hx(0x5a3a10)); setlinewidth(2);
        POINT tentL[3] = { {x-20,y+14}, {x,y-16-lv*2}, {x,y+14} };
        POINT tentR[3] = { {x,y-16-lv*2}, {x+20,y+14}, {x,y+14} };
        line(tentL[0].x,tentL[0].y,tentL[1].x,tentL[1].y);
        line(tentR[1].x,tentR[1].y,tentR[2].x,tentR[2].y);
        // 旗杆
        setlinecolor(hx(0x4a3018)); setlinewidth(3);
        line(x, y-16-lv*2, x, y-38-lv*2);
        setfillcolor(hx(lv>=3 ? 0xe0c040 : 0xc83020));
        POINT flag[3] = { {x,y-38-lv*2},{x+18,y-33-lv*2},{x,y-26-lv*2} };
        fillpolygon(flag, 3);
        // 剑
        setlinecolor(hx(0xd8d8e0)); setlinewidth(3);
        line(x+14, y+2, x+22, y-12);
    }
    else if (t.key == TK_ARTILLERY) {
        // 火山石堆 + 炮管
        setfillcolor(hx(0x5a4030));
        fillellipse(x - 18, y - 4, x + 18, y + 14);
        setfillcolor(hx(0x7a5030));
        fillellipse(x - 14, y - 10, x + 14, y + 6);
        setfillcolor(hx(0x9a6030));
        fillellipse(x - 8, y - 14, x + 8, y + 0);
        // 岩浆裂缝
        setlinecolor(hx(lv >= 3 ? 0xff6020 : 0xc84010)); setlinewidth(2);
        line(x-8, y-6, x+2, y-2); line(x+2, y-2, x+8, y-8);
        // 炮管朝向
        double a = t.aim;
        int bx = (int)(x + std::cos(a) * 22), by = (int)(y - 10 + std::sin(a) * 22);
        setlinecolor(hx(0x3a3028)); setlinewidth(10);
        line(x, y-10, bx, by);
        setlinecolor(hx(0x6a5a48)); setlinewidth(6);
        line(x, y-10, bx, by);
    }
    else if (t.key == TK_FROST) {
        // 冰晶石台
        setfillcolor(hx(0x6a8898));
        fillellipse(x - 18, y, x + 18, y + 12);
        setfillcolor(hx(0x8ab8c8));
        fillellipse(x - 14, y - 6, x + 14, y + 4);
        int n = 3 + lv;
        for (int i = 0; i < n; ++i) {
            double px = x + (i - (n-1)/2.0) * 7;
            int h = 12 + (i % 3) * 6 + lv * 2;
            setfillcolor(hx(i % 2 ? 0x9fd8f0 : 0xdff4ff));
            POINT p[3] = { {(int)px-4, y-6}, {(int)px+4, y-6}, {(int)px, y-6-h} };
            fillpolygon(p, 3);
        }
        // 寒气
        setfillcolor(hx(0xe8faff));
        fillcircle(x, y - 20 - lv * 2, 3 + lv);
    }
    else if (t.key == TK_SNIPER) {
        // 高瞭望塔
        int h = 26 + lv * 4;
        setfillcolor(hx(0x6a4a28));
        fillrect(x - 6, y - h, x - 2, y + 12);
        fillrect(x + 2, y - h, x + 6, y + 12);
        // 交叉支撑
        setlinecolor(hx(0x4a3018)); setlinewidth(2);
        line(x-6, y+8, x+6, y-h+8); line(x+6, y+8, x-6, y-h+8);
        // 平台
        setfillcolor(hx(0x8a6238));
        fillrect(x - 14, y - h - 6, x + 14, y - h + 2);
        // 瞭望台
        setfillcolor(hx(0x9a7050));
        fillrect(x - 10, y - h - 16, x + 10, y - h - 6);
        setfillcolor(hx(0xc84020));
        fillrect(x - 10, y - h - 18, x + 10, y - h - 15);
        // 红色瞄准镜
        double a = t.aim;
        int sx = (int)(x + std::cos(a) * 18), sy = (int)(y - h - 10 + std::sin(a) * 18);
        setlinecolor(hx(0xff4020)); setlinewidth(3);
        line(x, y-h-10, sx, sy);
        setfillcolor(hx(0xff6040));
        fillcircle(sx, sy, 3);
    }

    // 建造闪光（新塔在建造后短暂发光，用 bornT 控制）
    if (t.bornT > 0) {
        setfillcolor(hx(0xffe066));
        fillcircle(x, y - 30, (int)(10 + t.bornT * 20));
    }
}

// ============================================================================
// 怪物外观：6 种 + 走路动画
// ============================================================================
void Renderer::drawMonster(const Monster& m)
{
    int x = (int)(m.x + MAP_OX + m_dx);
    int y = (int)(m.y + MAP_OY + m_dy);
    int r = m.r;
    double bob = std::sin(m.walkPhase) * 2.5;
    int by = (int)(y + bob);
    const MonsterDef& d = MONSTER_DATA[m.type];

    // 阴影
    setfillcolor(hx(0x2a2010));
    fillellipse(x - r, y + r - 2, x + r, y + r + 5);

    unsigned int body = d.color;
    if (m.flash > 0) body = 0xffffff;
    if (m.frozen > 0) body = mixColor(body, 0xaee8ff, 0.6);

    // 身体
    setfillcolor(hx(body));
    fillcircle(x, by - r, r);
    setlinecolor(hx(0x2a2010)); setlinewidth(1);
    circle(x, by - r, r);

    if (m.type == MK_ORC) {
        // 兽人：獠牙 + 怒眉
        setfillcolor(hx(0xffffff));
        POINT f1[3] = { {x-6,by-r+4},{x-2,by-r+4},{x-4,by-r+10} };
        POINT f2[3] = { {x+2,by-r+4},{x+6,by-r+4},{x+4,by-r+10} };
        fillpolygon(f1,3); fillpolygon(f2,3);
        setfillcolor(hx(0x201810));
        fillcircle(x-5, by-r, 2); fillcircle(x+5, by-r, 2);
    } else if (m.type == MK_GOBLIN) {
        // 哥布林：尖耳朵
        setfillcolor(hx(body));
        POINT e1[3] = { {x-r,by-r},{x-r-8,by-r-6},{x-r+2,by-r+4} };
        POINT e2[3] = { {x+r,by-r},{x+r+8,by-r-6},{x+r-2,by-r+4} };
        fillpolygon(e1,3); fillpolygon(e2,3);
        setfillcolor(hx(0x201810));
        fillcircle(x-4, by-r-1, 2); fillcircle(x+4, by-r-1, 2);
    } else if (m.type == MK_SKELETON) {
        // 骷髅：眼窝 + 骨牙
        setfillcolor(hx(0x201810));
        fillcircle(x-4, by-r-2, 3); fillcircle(x+4, by-r-2, 3);
        setlinecolor(hx(0x201810)); setlinewidth(1);
        line(x-6, by-r+5, x+6, by-r+5);
        for (int i = -2; i <= 2; ++i) line(x+i*2, by-r+5, x+i*2, by-r+8);
    } else if (m.type == MK_SHADOW) {
        // 暗影：兜帽 + 发光眼
        setfillcolor(hx(0x301850));
        fillcircle(x, by-r-4, r-2);
        setfillcolor(hx(0xff40a0));
        fillcircle(x-4, by-r, 2); fillcircle(x+4, by-r, 2);
    } else if (m.type == MK_ARMORED) {
        // 重甲：头盔 + 护甲纹
        setfillcolor(hx(0x607080));
        fillrect(x - r + 2, by - r - r + 2, x + r - 2, by - r + 2);
        setlinecolor(hx(0x40505c)); setlinewidth(2);
        line(x-r+3, by-r, x+r-3, by-r);
        setfillcolor(hx(0x201810));
        fillrect(x-6, by-r+1, x+6, by-r+4);
    } else if (m.type == MK_BOSS) {
        // 巨魔首领：角 + 大眼
        setfillcolor(hx(0xe0c0a0));
        POINT h1[3] = { {x-r+2,by-r-4},{x-r-10,by-r-20},{x-r+8,by-r-8} };
        POINT h2[3] = { {x+r-2,by-r-4},{x+r+10,by-r-20},{x+r-8,by-r-8} };
        fillpolygon(h1,3); fillpolygon(h2,3);
        setfillcolor(hx(0xff4020));
        fillcircle(x-r/2, by-r, 4); fillcircle(x+r/2, by-r, 4);
        setfillcolor(hx(0xffffff));
        POINT t1[3] = { {x-9,by-r+8},{x-3,by-r+8},{x-6,by-r+16} };
        POINT t2[3] = { {x+3,by-r+8},{x+9,by-r+8},{x+6,by-r+16} };
        fillpolygon(t1,3); fillpolygon(t2,3);
    }

    // 减速冰霜覆盖
    if (m.slowT > 0) {
        setfillcolor(hx(0xaee8ff));
        setbkmode(TRANSPARENT);
        fillcircle(x, by - r, r + 1);
    }
    // 血条（受伤时显示）
    if (m.hp < m.maxHp) {
        int bw = r * 2 + 4;
        int bx = x - bw / 2, byy = by - r * 2 - 8;
        setfillcolor(hx(0x2a1010));
        fillrect(bx - 1, byy - 1, bx + bw + 1, byy + 5);
        setfillcolor(hx(0x40c030));
        fillrect(bx, byy, bx + (int)(bw * m.hp / m.maxHp), byy + 3);
        setfillcolor(hx(0xc03020));
        fillrect(bx, byy, bx + (int)(bw * m.hp / m.maxHp), byy + 3);
        setfillcolor(hx(0x40c030));
        fillrect(bx, byy, bx + (int)(bw * m.hp / m.maxHp), byy + 2);
    }
}

// ============================================================================
// 士兵外观
// ============================================================================
void Renderer::drawSoldier(const Soldier& s)
{
    int x = (int)(s.x + MAP_OX + m_dx);
    int y = (int)(s.y + MAP_OY + m_dy);
    bool mil = s.temp;
    // 阴影
    setfillcolor(hx(0x2a2010));
    fillellipse(x - 9, y + 4, x + 9, y + 9);
    // 身体
    setfillcolor(hx(mil ? 0xe0c040 : 0x3060a0));
    fillrect(x - 7, y - 12, x + 7, y + 4);
    setlinecolor(hx(0x1a2a40)); setlinewidth(1);
    rectangle(x - 7, y - 12, x + 7, y + 4);
    // 头
    setfillcolor(hx(0xe8c8a0));
    fillcircle(x, y - 16, 6);
    // 头盔
    setfillcolor(hx(mil ? 0xc09020 : 0x708090));
    fillcircle(x, y - 18, 6);
    fillrect(x - 7, y - 18, x + 7, y - 15);
    // 剑（挥砍动画）
    double ang = -0.8 + s.swing * 1.6;
    int sx = (int)(x + s.face * (10 + std::cos(ang) * 12));
    int sy = (int)(y - 8 + std::sin(ang) * 14);
    setlinecolor(hx(0xd8d8e0)); setlinewidth(3);
    line(x + s.face * 6, y - 4, sx, sy);
    setlinecolor(hx(0x6a4a28)); setlinewidth(2);
    line(x + s.face * 3, y - 6, x + s.face * 8, y - 2);

    // 血条
    if (s.hp < s.maxHp) {
        int bw = 20;
        setfillcolor(hx(0x2a1010));
        fillrect(x - 10, y - 28, x + 11, y - 23);
        setfillcolor(hx(0x40c030));
        fillrect(x - 10, y - 28, x - 10 + (int)(bw * s.hp / s.maxHp), y - 25);
    }
}

// ============================================================================
// 子弹与特效
// ============================================================================
void Renderer::drawBullet(const Bullet& b)
{
    if (b.delay > 0) return;
    int x = (int)(b.x + MAP_OX + m_dx);
    int y = (int)(b.y + MAP_OY + m_dy);
    if (b.kind == 0) {          // 箭
        int tx = (int)(b.px + MAP_OX + m_dx), ty = (int)(b.py + MAP_OY + m_dy);
        setlinecolor(hx(0xe8d8a0)); setlinewidth(2);
        line(tx, ty, x, y);
        setfillcolor(hx(0xffffff));
        fillcircle(x, y, 2);
    } else if (b.kind == 2) {   // 炮弹
        setfillcolor(hx(0x3a2a20));
        fillcircle(x, y, 7);
        setfillcolor(hx(0xff6020));
        fillcircle(x - 2, y - 2, 3);
        setlinecolor(hx(0xff8020)); setlinewidth(2);
        line(x, y - 8, x, y - 16);
    } else if (b.kind == 3) {   // 冰弹
        setfillcolor(hx(0xdff4ff));
        POINT p[4] = { {x,y-6},{x+6,y},{x,y+6},{x-6,y} };
        fillpolygon(p, 4);
    } else if (b.kind == 4) {   // 狙击子弹（红色拖尾）
        int tx = (int)(b.px + MAP_OX + m_dx), ty = (int)(b.py + MAP_OY + m_dy);
        setlinecolor(hx(0xff4020)); setlinewidth(3);
        line(tx, ty, x, y);
        setfillcolor(hx(0xffe080));
        fillcircle(x, y, 3);
    }
}

void Renderer::drawFxItem(const FXItem& f)
{
    double k = f.maxLife > 0 ? f.life / f.maxLife : 0;
    int x = (int)(f.x + MAP_OX + m_dx);
    int y = (int)(f.y + MAP_OY + m_dy);

    if (f.kind == 0) {          // 飘字
        int yy = (int)(f.y - (1 - k) * 30 + MAP_OY + m_dy);
        text(x, yy, f.textBuf, (int)f.size, f.color, true);
    } else if (f.kind == 1) {   // 粒子
        int sz = std::max(1, (int)(f.size * k));
        setfillcolor(hx(f.color));
        fillcircle(x, y, sz);
    } else if (f.kind == 2) {   // 光环扩散
        setlinecolor(hx(f.color));
        setlinewidth(std::max(1, (int)(4 * k)));
        int rr = (int)(f.r * (1 - k * 0.6));
        circle(x, y, rr);
    } else if (f.kind == 3) {   // 闪光
        setfillcolor(hx(f.color));
        fillcircle(x, y, (int)(f.r * k));
    } else if (f.kind == 4) {   // 电弧（锯齿线）
        int tx = (int)(f.tx + MAP_OX + m_dx);
        int ty = (int)(f.ty + MAP_OY + m_dy);
        setlinecolor(hx(f.color));
        setlinewidth(std::max(1, (int)(3 * k + 1)));
        int segs = 6;
        int px = x, py = y;
        unsigned int sd = (unsigned int)(f.life * 1000);
        for (int i = 1; i <= segs; ++i) {
            double t = i / (double)segs;
            int nx = (int)(x + (tx - x) * t);
            int ny = (int)(y + (ty - y) * t);
            if (i < segs) {
                nx += (int)((mulberry32(sd) / (double)0xffffffff - 0.5) * 16);
                ny += (int)((mulberry32(sd) / (double)0xffffffff - 0.5) * 16);
            }
            line(px, py, nx, ny);
            px = nx; py = ny;
        }
    } else if (f.kind == 5) {   // 闪电柱
        setlinecolor(hx(f.color));
        setlinewidth(std::max(1, (int)(4 * k)));
        line(x, y, (int)(f.tx + MAP_OX + m_dx), (int)(f.ty + MAP_OY + m_dy));
    } else if (f.kind == 6) {   // 蘑菇云
        setfillcolor(hx(0x6a5a50));
        fillcircle(x, y - (int)(f.r * 0.3), (int)(f.r * 0.5 * k + 4));
        setfillcolor(hx(0x9a8a7a));
        fillcircle(x, y - (int)(f.r * 0.6), (int)(f.r * 0.7 * k + 6));
        setfillcolor(hx(0xff8020));
        fillcircle(x, y - (int)(f.r * 0.2), (int)(f.r * 0.3 * k + 2));
    }
}

// ============================================================================
// 地面层：火海 / 冰锥丛 / 建造预览
// ============================================================================
void Renderer::drawMapLayer()
{
    // 火海
    for (const FlameZone& z : g.flameZones) {
        int x = (int)(z.x + MAP_OX + m_dx), y = (int)(z.y + MAP_OY + m_dy);
        int k = (int)(z.time / z.maxTime * 255);
        for (int i = 0; i < 9; ++i) {
            double a = g.time * 3 + i * 0.7;
            double rr = z.r * (0.3 + 0.5 * (i % 3) / 2.0);
            int fx = (int)(x + std::cos(a) * z.r * 0.6);
            int fy = (int)(y + std::sin(a) * z.r * 0.3);
            setfillcolor(hx(i % 2 ? 0xff6020 : 0xffa020));
            fillcircle(fx, fy - (int)(6 + 4 * std::sin(g.time * 6 + i)), (int)(rr * 0.35));
        }
    }
    // 冰锥丛
    for (const IceZone& z : g.iceZones) {
        int x = (int)(z.x + MAP_OX + m_dx), y = (int)(z.y + MAP_OY + m_dy);
        double grow = z.bornT > 0 ? (1 - z.bornT / 0.28) : 1;
        setfillcolor(hx(0xa8e0ff));
        fillellipse(x - (int)(z.r * grow), y - (int)(z.r * 0.55 * grow),
                    x + (int)(z.r * grow), y + (int)(z.r * 0.55 * grow));
        setfillcolor(hx(0xdff4ff));
        fillellipse(x - (int)(z.r * 0.8 * grow), y - (int)(z.r * 0.4 * grow),
                    x + (int)(z.r * 0.8 * grow), y + (int)(z.r * 0.4 * grow));
        for (const Spike& s : z.spikes) {
            int sx = x + (int)(s.dx * grow), sy = y + (int)(s.dy * grow);
            int sh = (int)(s.h * grow);
            setfillcolor(hx(0xe8faff));
            POINT p[3] = { {sx-(int)(s.w),sy},{sx+(int)(s.w),sy},{sx+(int)(s.tilt*10),sy-sh} };
            fillpolygon(p, 3);
        }
    }
    // 建造预览
    if (g.selectedTowerKey >= 0 && g.hoverC >= 0 && g.hoverR >= 0) {
        Point p = Game::cellCenter(g.hoverC, g.hoverR);
        int x = (int)(p.x + MAP_OX + m_dx), y = (int)(p.y + MAP_OY + m_dy);
        bool ok = g.buildable(g.hoverC, g.hoverR) && g.coin >= TOWER_DATA[g.selectedTowerKey].buildCost;
        setfillcolor(hx(ok ? 0x40c040 : 0xc04040));
        setlinecolor(hx(0xffffff)); setlinewidth(2);
        fillrect(x - CELL/2 + 2, y - CELL/2 + 2, x + CELL/2 - 2, y + CELL/2 - 2);
        // 半透明效果用亮色描边代替
        setfillcolor(hx(TOWER_DATA[g.selectedTowerKey].color));
        fillcircle(x, y, 16);
    }
    // 技能瞄准范围
    if (g.pendingSkill >= 0) {
        int x = (int)(g.mouseX + MAP_OX + m_dx), y = (int)(g.mouseY + MAP_OY + m_dy);
        setlinecolor(hx(SKILL_DATA[g.pendingSkill].color)); setlinewidth(2);
        int r = g.pendingSkill == SK_FLAME ? 92 : 66;
        circle(x, y, r);
        line(x - 12, y, x + 12, y);
        line(x, y - 12, x, y + 12);
    }
}

// ============================================================================
// 塔操作面板
// ============================================================================
void Renderer::drawTowerPanel()
{
    if (g.selectedTowerId < 0) return;
    Tower* t = g.findTower(g.selectedTowerId);
    if (!t) { g.selectedTowerId = -1; return; }

    const Rect& R = RECT_PANEL;
    setfillcolor(hx(0xf0d898));
    fillroundrect(R.x, R.y, R.x + R.w, R.y + R.h, 10);
    setlinecolor(hx(0x5a3a14)); setlinewidth(3);
    roundrect(R.x, R.y, R.x + R.w, R.y + R.h, 10);

    const TowerDef& d = TOWER_DATA[t->key];
    const TowerLevel& st = g.stats(*t);
    TCHAR buf[64];
    swprintf_s(buf, 64, _T("%s Lv%d"), d.name, t->level);
    text(R.x + 10, R.y + 8, buf, 16, 0x3a2410);
    text(R.x + 10, R.y + 32, st.name, 14, 0x5a4020);
    if (t->key == TK_BARRACKS) {
        swprintf_s(buf, 64, _T("士兵 %d血/%d攻 x%d"), st.soldierHp, st.soldierDps, st.count);
    } else {
        swprintf_s(buf, 64, _T("伤害%d 射程%d 攻速%.2f"), (int)st.dmg, (int)st.range, st.rate);
    }
    text(R.x + 10, R.y + 52, buf, 12, 0x5a4020);

    Rect up = upgradeBtnRect();
    if (t->level < 4) {
        const TowerLevel& nx = d.levels[t->level];
        swprintf_s(buf, 64, _T("升级 金币%d"), nx.cost);
        setfillcolor(hx(g.coin >= nx.cost ? 0x4a8a28 : 0x7a8a70));
        fillroundrect(up.x, up.y, up.x + up.w, up.y + up.h, 6);
        text(up.x + up.w/2, up.y + 8, buf, 14, 0xffffff, true);
    } else {
        setfillcolor(hx(0xc8a040));
        fillroundrect(up.x, up.y, up.x + up.w, up.y + up.h, 6);
        text(up.x + up.w/2, up.y + 8, _T("已满级"), 14, 0x3a2410, true);
    }
    Rect sl = sellBtnRect();
    int refund = (int)(t->totalSpent * SELL_RATE);
    swprintf_s(buf, 64, _T("出售 +%d"), refund);
    setfillcolor(hx(0xc87820));
    fillroundrect(sl.x, sl.y, sl.x + sl.w, sl.y + sl.h, 6);
    text(sl.x + sl.w/2, sl.y + 8, buf, 14, 0xffffff, true);
}

// ============================================================================
// 消息：飘字 / 波次公告
// ============================================================================
void Renderer::drawMessages()
{
    if (g.toastT > 0) {
        int alpha = (int)(std::min(1.0, g.toastT / 20) * 255);
        setfillcolor(hx(0x8a3a08));
        int w = textW(g.toastMsg, 20) + 30;
        fillroundrect(MAP_OX + CANVAS_W/2 - w/2, MAP_OY + 10,
                      MAP_OX + CANVAS_W/2 + w/2, MAP_OY + 44, 8);
        text(MAP_OX + CANVAS_W/2, MAP_OY + 16, g.toastMsg, 20, 0xffffff, true);
    }
    if (g.bannerT > 0) {
        setfillcolor(hx(0x8a3a08));
        text(MAP_OX + CANVAS_W/2 + 2, MAP_OY + CANVAS_H/2 - 24 + 2,
             g.bannerMsg, 44, 0x2a1008, true);
        text(MAP_OX + CANVAS_W/2, MAP_OY + CANVAS_H/2 - 24,
             g.bannerMsg, 44, 0xffe066, true);
    }
}

// ============================================================================
// 结算界面
// ============================================================================
void Renderer::drawOverlay()
{
    if (!g.over) return;
    setfillcolor(hx(0x140a00));
    fillrect(MAP_OX, MAP_OY, MAP_OX + CANVAS_W, MAP_OY + CANVAS_H);
    text(MAP_OX + CANVAS_W/2, MAP_OY + CANVAS_H/2 - 90,
         g.win ? _T("王国守住了!") : _T("城堡陷落"),
         52, g.win ? 0xffe066 : 0xff5a4a, true);
    TCHAR sub[80];
    if (g.win)
        swprintf_s(sub, 80, _T("全部 %d 波敌人被歼灭，荣耀属于你!"), WAVE_MAX);
    else
        swprintf_s(sub, 80, _T("坚持到了第 %d 波，再来一次吧!"), g.wave);
    text(MAP_OX + CANVAS_W/2, MAP_OY + CANVAS_H/2 - 10, sub, 18, 0xd8c8a0, true);

    const Rect& R = RECT_OVER_BTN;
    setfillcolor(hx(0x4a8a28));
    fillroundrect(R.x, R.y, R.x + R.w, R.y + R.h, 10);
    setlinecolor(hx(0x234a10)); setlinewidth(3);
    roundrect(R.x, R.y, R.x + R.w, R.y + R.h, 10);
    text(R.x + R.w/2, R.y + 14, _T("再来一局"), 22, 0xffffff, true);
}

// ============================================================================
// 主渲染入口
// ============================================================================
void Renderer::render()
{
    cleardevice();

    // 震屏偏移
    m_dx = m_dy = 0;
    if (g.shakeT > 0) {
        double k = g.shakeT / 0.5;
        double mag = g.shakeMag * k;
        m_dx = (int)((rand() / (double)RAND_MAX - 0.5) * mag);
        m_dy = (int)((rand() / (double)RAND_MAX - 0.5) * mag);
    }

    // 地图外框
    setfillcolor(hx(0x1a2e18));
    fillrect(LEFT_W, TOP_H, RIGHT_X, WIN_H);
    setfillcolor(hx(0x3a2410));
    fillrect(MAP_OX - 5, MAP_OY - 5, MAP_OX + CANVAS_W + 5, MAP_OY + CANVAS_H + 5);

    // 静态地图（离屏缓存）
    putimage(MAP_OX + m_dx, MAP_OY + m_dy, m_map);

    // 地面层
    drawMapLayer();

    // 实体按 y 排序（上到下覆盖）
    struct Drawable { double y; int type; const void* ptr; };
    std::vector<Drawable> list;
    for (const Tower& t : g.towers)   list.push_back({ t.y, 0, &t });
    for (const Monster& m : g.monsters) if (!m.dead) list.push_back({ m.y, 1, &m });
    for (const Soldier& s : g.soldiers) if (!s.dead) list.push_back({ s.y, 2, &s });
    std::sort(list.begin(), list.end(),
              [](const Drawable& a, const Drawable& b) { return a.y < b.y; });
    for (const Drawable& d : list) {
        if (d.type == 0) drawTower(*(const Tower*)d.ptr);
        else if (d.type == 1) drawMonster(*(const Monster*)d.ptr);
        else drawSoldier(*(const Soldier*)d.ptr);
    }

    // 子弹 / 特效
    for (const Bullet& b : g.bullets) drawBullet(b);
    for (const FXItem& f : g.fxs) drawFxItem(f);

    // UI 层
    drawTopBar();
    drawBuildPanel();
    drawSkillPanel();
    drawTowerPanel();
    drawMessages();
    drawOverlay();
}
