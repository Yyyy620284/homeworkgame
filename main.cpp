// ============================================================================
// main.cpp  ——  程序入口、主循环与鼠标交互
// 《王国保卫》C++ 版（EasyX）—— 完整复刻网页原作玩法
// ============================================================================

#include "Renderer.h"
#include <cmath>
#include <ctime>
#include <conio.h>

int main()
{
    // 初始化图形窗口
    initgraph(WIN_W, WIN_H);
    setbkcolor(hx(0x102010));
    cleardevice();
    BeginBatchDraw();                       // 双缓冲，避免闪烁

    srand((unsigned int)time(nullptr));     // 随机种子

    Game game;
    Renderer renderer(game);

    DWORD lastTick = GetTickCount();        // 上一帧时间戳

    while (true)
    {
        // ---------- 消息处理 ----------
        while (MouseHit()) {
            MOUSEMSG msg = GetMouseMsg();
            int mx = msg.x, my = msg.y;

            // 鼠标在地图内的坐标
            int mcx = mx - MAP_OX, mcy = my - MAP_OY;
            game.mouseX = mcx;
            game.mouseY = mcy;
            game.hoverC = (int)std::floor((mcx - PAD) / (double)CELL);
            game.hoverR = (int)std::floor((mcy - PAD) / (double)CELL);

            if (msg.uMsg == WM_MOUSEMOVE) {
                continue;                   // 仅更新悬停格
            }
            if (msg.uMsg == WM_RBUTTONDOWN) {
                // 右键：取消选择 / 关闭面板
                game.selectedTowerKey = -1;
                game.pendingSkill = -1;
                game.selectedTowerId = -1;
                continue;
            }
            if (msg.uMsg != WM_LBUTTONDOWN) continue;

            // ---------- 结算界面 ----------
            if (game.over) {
                if (RECT_OVER_BTN.hit(mx, my)) game.newGame();
                continue;
            }

            // ---------- 顶部按钮 ----------
            if (RECT_WAVE.hit(mx, my))    { game.startWave(); continue; }
            if (RECT_PAUSE.hit(mx, my))   { game.paused = !game.paused; continue; }
            if (RECT_SPEED.hit(mx, my))   { game.speed = game.speed == 1 ? 2 : (game.speed == 2 ? 3 : 1); continue; }
            if (RECT_RESTART.hit(mx, my)) { game.newGame(); continue; }

            // ---------- 左侧建造塔卡片 ----------
            bool picked = false;
            for (int i = 0; i < TK_COUNT; ++i) {
                if (towerCardRect(i).hit(mx, my)) {
                    game.selectedTowerKey = (game.selectedTowerKey == i) ? -1 : i;
                    game.selectedTowerId = -1;
                    game.pendingSkill = -1;
                    picked = true;
                    break;
                }
            }
            if (picked) continue;

            // ---------- 右侧技能卡片 ----------
            for (int i = 0; i < SK_COUNT; ++i) {
                if (skillCardRect(i).hit(mx, my)) {
                    if (game.canUseSkill(i)) {
                        if (game.pendingSkill == i) game.pendingSkill = -1;
                        else {
                            game.pendingSkill = i;
                            game.selectedTowerKey = -1;
                            game.selectedTowerId = -1;
                            game.say(_T("点击地图选择释放位置"));
                        }
                    } else {
                        game.say(game.itemCd[i] > 0 ? _T("技能冷却中") : _T("金币不足"));
                    }
                    picked = true;
                    break;
                }
            }
            if (picked) continue;

            // ---------- 塔操作面板按钮 ----------
            if (game.selectedTowerId >= 0) {
                if (upgradeBtnRect().hit(mx, my)) {
                    game.upgradeTower(game.selectedTowerId);
                    continue;
                }
                if (sellBtnRect().hit(mx, my)) {
                    game.sellTower(game.selectedTowerId);
                    continue;
                }
            }

            // ---------- 地图点击 ----------
            if (mcx < 0 || mcy < 0 || mcx > CANVAS_W || mcy > CANVAS_H) continue;

            // 技能瞄准模式：释放技能
            if (game.pendingSkill >= 0) {
                game.castSkill(game.pendingSkill, mcx, mcy);
                continue;
            }
            // 建造模式：尝试建塔
            if (game.selectedTowerKey >= 0) {
                game.tryBuild(game.hoverC, game.hoverR);
                continue;
            }
            // 点选塔打开操作面板
            Tower* t = game.towerAt(mcx, mcy, 32);
            game.selectedTowerId = t ? t->id : -1;
        }

        // 键盘：Esc 取消操作，Q 退出
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 27) {                     // Esc
                game.selectedTowerKey = -1;
                game.pendingSkill = -1;
                game.selectedTowerId = -1;
            } else if (ch == 'q' || ch == 'Q') {
                break;
            }
        }

        // ---------- 时间步进 ----------
        DWORD now = GetTickCount();
        double rawDt = (now - lastTick) / 1000.0;
        lastTick = now;
        if (rawDt > 0.05) rawDt = 0.05;     // 限制最大步长，防止跳帧
        double dt = (game.paused || game.over) ? 0 : rawDt * game.speed;

        if (dt > 0) game.update(dt);
        else {
            // 暂停时仅推进提示计时（不推进战斗）
            if (game.toastT > 0) game.toastT -= rawDt * 60;
            if (game.bannerT > 0) game.bannerT -= rawDt * 60;
        }

        renderer.render();
        FlushBatchDraw();
    }

    EndBatchDraw();
    closegraph();
    return 0;
}
