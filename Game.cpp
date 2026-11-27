// ============================================================================
// Game.cpp  ——  游戏逻辑实现
// ============================================================================

#include "Game.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

// 随机数：0~1
static double rnd() { return (double)rand() / RAND_MAX; }

// 路径格标记表（true = 该格是泥土路，不可建塔）
static bool g_pathCell[ROWS][COLS];
static void initPathCells()
{
    for (int r = 0; r < ROWS; ++r)
        for (int c = 0; c < COLS; ++c) g_pathCell[r][c] = false;

    for (int i = 0; i < PATH_N - 1; ++i) {
        int c1 = PATH_CELLS[i][0], r1 = PATH_CELLS[i][1];
        int c2 = PATH_CELLS[i+1][0], r2 = PATH_CELLS[i+1][1];
        int dc = (c2 > c1) - (c2 < c1);
        int dr = (r2 > r1) - (r2 < r1);
        int c = c1, r = r1;
        while (true) {
            if (r >= 0 && r < ROWS && c >= 0 && c < COLS) g_pathCell[r][c] = true;
            if (c == c2 && r == r2) break;
            if (c != c2) c += dc; else if (r != r2) r += dr;
        }
    }
    g_pathCell[BASE_CELL[1]][BASE_CELL[0]] = true;
}

// ============================================================================
// 构造 / 重置
// ============================================================================
Game::Game()
{
    initPathCells();
    newGame();
}

void Game::newGame()
{
    coin = START_COIN;
    hp = START_HP;
    wave = 0;
    waveActive = false;
    over = false;
    paused = false;
    win = false;
    time = 0;
    speed = 1;
    selectedTowerKey = -1;
    selectedTowerId = -1;
    pendingSkill = -1;
    shakeT = 0; shakeMag = 0;
    toastMsg[0] = 0; toastT = 0;
    bannerMsg[0] = 0; bannerT = 0;
    nextId = 1;

    for (int i = 0; i < SK_COUNT; ++i) itemCd[i] = 0;

    towers.clear(); monsters.clear(); bullets.clear(); soldiers.clear();
    iceZones.clear(); flameZones.clear(); fxs.clear(); spawnQueue.clear();
}

// ---------- 静态几何辅助 ----------
Point Game::cellCenter(int c, int r)
{
    return { PAD + c * CELL + CELL / 2.0, PAD + r * CELL + CELL / 2.0 };
}
Point Game::pathPoint(int i)
{
    return cellCenter(PATH_CELLS[i][0], PATH_CELLS[i][1]);
}
Point Game::nearestPathPoint(double x, double y)
{
    Point best{x, y};
    double bd = 1e9;
    for (int i = 0; i < PATH_N - 1; ++i) {
        Point a = pathPoint(i), b = pathPoint(i + 1);
        double dx = b.x - a.x, dy = b.y - a.y;
        double L2 = dx*dx + dy*dy; if (L2 < 1) L2 = 1;
        double t = ((x - a.x)*dx + (y - a.y)*dy) / L2;
        if (t < 0) t = 0; if (t > 1) t = 1;
        double px = a.x + dx*t, py = a.y + dy*t;
        double d = std::hypot(px - x, py - y);
        if (d < bd) { bd = d; best = {px, py}; }
    }
    return best;
}

bool Game::buildable(int c, int r) const
{
    if (c < 0 || r < 0 || c >= COLS || r >= ROWS) return false;
    if (g_pathCell[r][c]) return false;
    // 基地周围一格保留
    if (std::abs(c - BASE_CELL[0]) <= 1 && std::abs(r - BASE_CELL[1]) <= 1) return false;
    // 该格已有塔则不可再建
    for (const Tower& t : towers) if (t.c == c && t.r == r) return false;
    return true;
}

Tower* Game::findTower(int id)
{
    for (Tower& t : towers) if (t.id == id) return &t;
    return nullptr;
}
Monster* Game::findMonster(int id)
{
    for (Monster& m : monsters) if (m.id == id) return &m;
    return nullptr;
}
Soldier* Game::findSoldier(int id)
{
    for (Soldier& s : soldiers) if (s.id == id) return &s;
    return nullptr;
}
Tower* Game::towerAt(double x, double y, double radius)
{
    for (Tower& t : towers)
        if (std::hypot(t.x - x, t.y - y) < radius) return &t;
    return nullptr;
}
const TowerLevel& Game::stats(const Tower& t) const
{
    return TOWER_DATA[t.key].levels[t.level - 1];
}

// ============================================================================
// 建造 / 升级 / 出售
// ============================================================================
bool Game::tryBuild(int c, int r)
{
    if (selectedTowerKey < 0) return false;
    if (!buildable(c, r)) { say(_T("这里不能建塔")); return false; }
    int cost = TOWER_DATA[selectedTowerKey].buildCost;
    if (coin < cost) { say(_T("金币不足")); return false; }

    coin -= cost;
    Tower t;
    t.id = nextId++;
    t.key = selectedTowerKey;
    t.c = c; t.r = r;
    Point p = cellCenter(c, r);
    t.x = p.x; t.y = p.y;
    t.level = 1;
    t.totalSpent = cost;
    t.breathPhase = rnd() * 6.2831853;
    towers.push_back(t);

    fxFlash(p.x, p.y - 20, 52, TOWER_DATA[t.key].color, 22);
    fxText(p.x, p.y - 52, _T("建造!"), 0xffe066, 15, 44);
    return true;
}

bool Game::upgradeTower(int towerId)
{
    Tower* t = findTower(towerId);
    if (!t) return false;
    if (t->level >= 4) { say(_T("已满级")); return false; }
    const TowerLevel& next = TOWER_DATA[t->key].levels[t->level];
    if (coin < next.cost) { say(_T("金币不足")); return false; }

    coin -= next.cost;
    t->totalSpent += next.cost;
    t->level++;
    fxFlash(t->x, t->y - 30, 62, TOWER_DATA[t->key].color, 26);
    fxRing(t->x, t->y - 10, 46, 0xffe066, 24);
    fxText(t->x, t->y - 72, t->level == 4 ? _T("满级!") : _T("升级!"), 0xffd040, 17, 52);
    return true;
}

bool Game::sellTower(int towerId)
{
    auto it = std::find_if(towers.begin(), towers.end(),
        [towerId](const Tower& t) { return t.id == towerId; });
    if (it == towers.end()) return false;

    int refund = (int)(it->totalSpent * SELL_RATE);
    coin += refund;
    fxNumber(it->x, it->y - 30, refund, 0xffd040, 15);
    fxBurst(it->x, it->y - 10, 12, 0xc8b890, 3.5, 3.5, 32);
    towers.erase(it);
    // 清除该兵营召唤的士兵
    soldiers.erase(std::remove_if(soldiers.begin(), soldiers.end(),
        [towerId](const Soldier& s) { return s.barrack == towerId; }), soldiers.end());
    selectedTowerId = -1;
    return true;
}

// ============================================================================
// 波次系统
// ============================================================================
void Game::startWave()
{
    if (over || waveActive || wave >= WAVE_MAX) return;
    wave++;
    spawnQueue.clear();
    for (int g = 0; g < WAVE_GROUP_N[wave - 1]; ++g) {
        const SpawnGroup& sg = WAVE_DATA[wave - 1][g];
        for (int i = 0; i < sg.count; ++i)
            spawnQueue.push_back({ sg.type, i * sg.gap });
    }
    std::sort(spawnQueue.begin(), spawnQueue.end(),
        [](const SpawnItem& a, const SpawnItem& b) { return a.delay < b.delay; });

    spawnTimer = 0;
    waveActive = true;

    TCHAR buf[32];
    swprintf_s(buf, 32, _T("第 %d 波"), wave);
    banner(buf, 1.4);
}

void Game::updateWave(double dt)
{
    if (!waveActive) return;
    spawnTimer += dt;
    while (!spawnQueue.empty() && spawnQueue.front().delay <= spawnTimer) {
        spawnMonster(spawnQueue.front().type);
        spawnQueue.erase(spawnQueue.begin());
    }
    // 队列已空且场上怪物全部死亡 → 本波结束
    if (spawnQueue.empty()) {
        bool anyAlive = false;
        for (const Monster& m : monsters) if (!m.dead) { anyAlive = true; break; }
        if (!anyAlive) {
            waveActive = false;
            int bonus = WAVE_BONUS + wave * 10;
            coin += bonus;
            TCHAR msg[80];
            swprintf_s(msg, 80, _T("第 %d 波清除! 奖励 +%d"), wave, bonus);
            say(msg, 2.2);
            if (wave >= WAVE_MAX) { over = true; win = true; }
        }
    }
}

// ============================================================================
// 怪物：生成 / 伤害 / 移动
// ============================================================================
void Game::spawnMonster(MonsterKind type)
{
    const MonsterDef& d = MONSTER_DATA[type];
    double wScale = 1 + (wave - 1) * WAVE_SCALE_HP;
    double sScale = 1 + (wave - 1) * WAVE_SCALE_SPEED;
    Point p = pathPoint(0);

    Monster m;
    m.id = nextId++;
    m.type = type;
    m.x = p.x; m.y = p.y;
    m.seg = 0; m.segT = 0;
    m.hp = m.maxHp = d.hp * wScale;
    m.speed = d.speed * sScale;
    m.armor = d.armor;
    m.reward = d.reward;
    m.r = d.r;
    m.boss = d.boss;
    m.walkPhase = rnd() * 10;
    monsters.push_back(m);

    if (m.boss) {
        shakeT = 0.5; shakeMag = 9;
        fxText(m.x, m.y - 40, _T("BOSS 来袭!"), 0xff5a3a, 22, 70);
    }
}

void Game::damageMonster(Monster* m, double dmg, bool ignoreArmor, double pierce, bool crit)
{
    if (!m || m->dead) return;
    double real;
    if (ignoreArmor) real = dmg;
    else if (pierce >= 0) real = dmg - m->armor * (1 - pierce);
    else real = dmg - m->armor;
    if (real < 1) real = 1;
    m->hp -= real;
    m->flash = 4;

    fxNumber(m->x + (rnd() - 0.5) * 14, m->y - m->r - 8, (int)(real + 0.5),
             crit ? 0xffd040 : 0xffffff, crit ? 17 : 13);
    if (crit) fxText(m->x, m->y - m->r - 26, _T("暴击!"), 0xff8a20, 15, 38);

    if (m->hp <= 0) killMonster(m);
}

void Game::killMonster(Monster* m)
{
    if (!m || m->dead) return;
    m->dead = true;
    m->deathT = 0.42;
    coin += m->reward;
    fxNumber(m->x, m->y - m->r - 12, m->reward, 0xffd040);
    fxBurst(m->x, m->y - m->r * 0.6, m->boss ? 34 : 16,
            m->boss ? 0xff8a20 : 0xc8402a, m->boss ? 7 : 4.5,
            m->boss ? 5 : 3.4, 32);
    fxRing(m->x, m->y - m->r * 0.5, m->boss ? 58 : 26, 0xffd040, 22);
    if (m->boss) { shakeT = 0.4; shakeMag = 7; fxMushroom(m->x, m->y - 20, 40); }
}

void Game::leakMonster(Monster* m)
{
    m->dead = true;
    m->deathT = 0;
    hp--;
    fxText(m->x, m->y - 20, _T("-1"), 0xff5a5a, 18, 46);
    if (hp <= 0) { over = true; win = false; }
}

void Game::moveMonster(Monster* m, double dt)
{
    if (m->frozen > 0) { m->frozen -= dt; return; }
    if (m->engaged != -1 && findSoldier(m->engaged)) return;   // 被士兵拦截

    double sp = m->speed;
    if (m->slowT > 0) { m->slowT -= dt; sp *= (1 - m->slowRatio); }
    double dist = sp * dt;

    while (dist > 0) {
        if (m->seg >= PATH_N - 1) { leakMonster(m); return; }
        Point a = pathPoint(m->seg), b = pathPoint(m->seg + 1);
        double segLen = std::hypot(b.x - a.x, b.y - a.y);
        if (m->segT + dist >= segLen) {
            dist -= (segLen - m->segT);
            m->segT = 0;
            m->seg++;
            if (m->seg >= PATH_N - 1) { leakMonster(m); return; }
        } else {
            m->segT += dist;
            dist = 0;
        }
    }
    Point a = pathPoint(m->seg), b = pathPoint(m->seg + 1);
    double segLen = std::hypot(b.x - a.x, b.y - a.y); if (segLen < 1) segLen = 1;
    double t = m->segT / segLen;
    m->x = a.x + (b.x - a.x) * t;
    m->y = a.y + (b.y - a.y) * t;
    m->walkPhase += dt * 9;
}

// ============================================================================
// 塔攻击
// ============================================================================
void Game::updateTowerAttack(Tower& t, double dt)
{
    if (t.key == TK_BARRACKS) return;
    const TowerLevel& st = stats(t);
    t.cool -= dt;

    // 目标：射程内推进最远的怪物
    Monster* best = nullptr;
    double bp = -1;
    for (Monster& m : monsters) {
        if (m.dead) continue;
        if (std::hypot(m.x - t.x, m.y - t.y) > st.range) continue;
        double prog = m.seg + m.segT / 1000.0;
        if (prog > bp) { bp = prog; best = &m; }
    }
    if (!best) return;

    // 平滑转向
    double want = std::atan2(best->y - t.y, best->x - t.x);
    double da = want - t.aim;
    while (da > 3.14159265) da -= 6.2831853;
    while (da < -3.14159265) da += 6.2831853;
    t.aim += da * (dt * 10 > 1 ? 1 : dt * 10);

    if (t.cool > 0) return;
    t.cool = st.rate;
    int bid = best->id;
    double bx = best->x, by = best->y, br = best->r;

    if (t.key == TK_ARCHER) {
        int shots = (int)(st.multi > 0 ? st.multi : 1);
        if (t.level <= 2) shots = 1;
        for (int i = 0; i < shots; ++i) {
            Bullet b;
            b.kind = 0;
            double oy = t.level >= 4 ? -100 : t.level == 3 ? -58 : t.level == 2 ? -52 : -32;
            b.x = t.x; b.y = t.y + oy;
            b.target = bid; b.speed = st.projSpeed; b.st = st; b.towerId = t.id;
            b.px = b.x; b.py = b.y;
            b.angle = std::atan2(by - b.y, bx - b.x);
            b.delay = i * 0.09;
            bullets.push_back(b);
        }
    }
    else if (t.key == TK_MAGE) {
        // 法师塔：瞬时电弧，无需飞行弹道
        fxArc(t.x, t.y - 58, bx, by - br * 0.5, 0xc060ff, 15);
        fxArc(t.x, t.y - 58, bx, by - br * 0.5, 0x8a30e0, 12);
        fxFlash(bx, by - br * 0.5, 26, 0xc070ff, 12);
        fxRing(bx, by - br * 0.5, st.aoe * 0.9, 0xc080ff, 16);
        fxBurst(bx, by - br * 0.5, 8, 0xe0b0ff, 3.6, 2.8, 20);
        damageMonster(best, st.dmg, true, -1, false);
        for (Monster& o : monsters) {
            if (o.id == bid || o.dead) continue;
            if (std::hypot(o.x - bx, o.y - by) <= st.aoe)
                damageMonster(&o, st.dmg * 0.55, true, -1, false);
        }
        if (t.level >= 4) { best->burnDps = st.dmg * 0.12; best->burnT = 1.6; }
    }
    else if (t.key == TK_ARTILLERY) {
        Bullet b;
        b.kind = 2;
        b.x = bx + (rnd() - 0.5) * 10;
        b.y = std::max(40.0, by - 260);
        b.target = bid; b.speed = st.projSpeed * 4.2; b.st = st; b.towerId = t.id;
        b.spin = 0; b.vy = 1;
        bullets.push_back(b);
        fxFlash(bx, by - br * 0.4, st.aoe * 0.5, 0xff5020, 16);
    }
    else if (t.key == TK_FROST) {
        Bullet b;
        b.kind = 3;
        b.x = t.x + std::cos(t.aim) * 8;
        b.y = t.y - 34 + std::sin(t.aim) * 6;
        b.target = bid; b.speed = st.projSpeed; b.st = st; b.towerId = t.id;
        b.px = b.x; b.py = b.y; b.angle = t.aim;
        bullets.push_back(b);
        fxBurst(b.x, b.y, 6, 0xdff4ff, 3, 3, 16);
    }
    else if (t.key == TK_SNIPER) {
        Bullet b;
        b.kind = 4;
        b.x = t.x + std::cos(t.aim) * 12;
        b.y = t.y - 84 + std::sin(t.aim) * 8;
        b.target = bid; b.speed = st.projSpeed; b.st = st; b.towerId = t.id;
        b.px = b.x; b.py = b.y; b.angle = t.aim; b.life = 1;
        bullets.push_back(b);
        fxFlash(b.x, b.y, 20, 0xff4020, 8);
        shakeT = std::max(shakeT, 0.06); shakeMag = std::max(shakeMag, 2.0);
    }
}

void Game::spawnIceSpikes(double x, double y, double r, const TowerLevel& st)
{
    IceZone z;
    z.x = x; z.y = y; z.r = r;
    z.life = z.maxLife = 3.2;
    z.slow = st.slow > 0 ? st.slow : 0.5;
    z.slowT = 0.6;
    z.dps = st.dmg * 0.18;
    z.bornT = 0.28;
    int n = 6 + (int)(rnd() * 4);
    for (int i = 0; i < n; ++i) {
        double a = rnd() * 6.2831853, d = rnd() * r * 0.78;
        Spike s;
        s.dx = std::cos(a) * d;
        s.dy = std::sin(a) * d * 0.5;
        s.h = 10 + rnd() * 14;
        s.w = 3.2 + rnd() * 2.6;
        s.tilt = (rnd() - 0.5) * 0.5;
        z.spikes.push_back(s);
    }
    iceZones.push_back(z);
    fxRing(x, y, r, 0xbfe8ff, 16);
}

void Game::updateBullets(double dt)
{
    for (int i = (int)bullets.size() - 1; i >= 0; --i) {
        Bullet& b = bullets[i];
        if (b.delay > 0) { b.delay -= dt; continue; }
        Monster* tg = findMonster(b.target);
        if (!tg || tg->dead) { bullets.erase(bullets.begin() + i); continue; }

        double step = b.speed * dt * 60;
        double prevX = b.x, prevY = b.y;

        if (b.kind == 2) {                       // 空投炮弹：垂直落下
            b.y += step;
            b.spin += dt * 10;
            double hitY = tg->y - tg->r * 0.4;
            if (b.y >= hitY) {
                b.x += (tg->x - b.x) * 0.55;
                b.x = std::min(b.x, tg->x + 12);
                b.x = std::max(b.x, tg->x - 12);
                onBulletHit(b, tg);
                bullets.erase(bullets.begin() + i);
            }
            continue;
        }

        double dx = tg->x - b.x, dy = (tg->y - tg->r * 0.6) - b.y;
        double d = std::hypot(dx, dy); if (d < 1) d = 1;
        if (d <= step) {
            onBulletHit(b, tg);
            bullets.erase(bullets.begin() + i);
            continue;
        }
        b.x += dx / d * step;
        b.y += dy / d * step;
        b.px = prevX; b.py = prevY;
        if (b.kind == 0 || b.kind == 3 || b.kind == 4) b.angle = std::atan2(dy, dx);
        if (b.kind == 3 || b.kind == 2) b.spin += dt * 6;
    }
}

void Game::onBulletHit(Bullet& b, Monster* m)
{
    const TowerLevel& st = b.st;
    double mx = m->x, my = m->y, mr = m->r;
    int mid = m->id;

    if (b.kind == 0) {                           // 箭
        if (st.crit > 0 && rnd() < st.crit) {
            damageMonster(m, st.dmg * 2.2, false, -1, true);
            fxBurst(mx, my - mr, 10, 0xffe080, 4, 3, 20);
            fxRing(mx, my - mr * 0.6, 20, 0xffd040, 16);
        } else {
            damageMonster(m, st.dmg, false, -1, false);
            fxBurst(mx, my - mr * 0.6, 5, 0xe8d8a0, 2.6, 2.6, 14);
        }
        Tower* tw = findTower(b.towerId);
        if (tw && tw->level >= 4) {
            for (Monster& o : monsters) {
                if (o.id == mid || o.dead) continue;
                if (std::hypot(o.x - mx, o.y - my) < 40)
                    damageMonster(&o, st.dmg * 0.4, false, -1, false);
            }
        }
    }
    else if (b.kind == 2) {                      // 炮弹爆炸
        shakeT = std::max(shakeT, 0.16); shakeMag = std::max(shakeMag, 4.0);
        fxFlash(mx, my - mr * 0.4, st.aoe * 0.8, 0xffb040, 20);
        fxRing(mx, my - mr * 0.4, st.aoe * 0.9, 0xffb040, 20);
        fxBurst(mx, my - mr * 0.4, 12, 0xff8020, 4.5, 3, 24);
        for (Monster& o : monsters) {
            if (o.dead) continue;
            double d = std::hypot(o.x - mx, o.y - my);
            if (d <= st.aoe) {
                bool core = d <= st.aoe * 0.42;
                double dmg = st.dmg * (core ? 1.5 : 0.72);
                Tower* tw = findTower(b.towerId);
                if (tw && tw->level >= 4 && core) dmg = st.dmg * 2;
                damageMonster(&o, dmg, false, st.pierce, false);
                if (st.burn > 0) { o.burnDps = st.burn; o.burnT = 2.0; }
            }
        }
        fxMushroom(mx, my - mr * 0.4, st.aoe * 0.62);
    }
    else if (b.kind == 3) {                      // 冰弹
        damageMonster(m, st.dmg, false, -1, false);
        double R = st.aoe > 0 ? st.aoe : 40;
        fxRing(mx, my - mr * 0.5, R, 0xbfe8ff, 22);
        fxBurst(mx, my - mr * 0.5, 10, 0x8fe0ff, 4, 3, 22);
        fxFlash(mx, my - mr * 0.5, R * 1.1, 0xa8e0ff, 14);
        for (Monster& o : monsters) {
            if (o.dead) continue;
            if (std::hypot(o.x - mx, o.y - my) > R) continue;
            if (o.id != mid) damageMonster(&o, st.dmg * 0.5, false, -1, false);
            o.slowT = std::max(o.slowT, st.slowT > 0 ? st.slowT : 2);
            o.slowRatio = st.slow > 0 ? st.slow : 0.4;
            if (st.freeze > 0 && rnd() < st.freeze) o.frozen = std::max(o.frozen, 1.2);
        }
        spawnIceSpikes(mx, my, std::max(30.0, R * 0.8), st);
    }
    else if (b.kind == 4) {                      // 狙击
        bool crit = st.crit > 0 && rnd() < st.crit;
        double dmg = st.dmg * (crit ? 2 : 1);
        damageMonster(m, dmg, false, st.armorPen, crit);
        fxBurst(mx, my - mr * 0.5, crit ? 16 : 9, 0xffd040, crit ? 6 : 4, 3.2, 24);
        fxRing(mx, my - mr * 0.5, crit ? 34 : 24, 0xffd040, 18);
        fxFlash(mx, my - mr * 0.5, crit ? 42 : 28, 0xffe080, 12);
        if (crit) { fxText(mx, my - mr - 16, _T("暴击!"), 0xffd040, 16, 40);
                    shakeT = std::max(shakeT, 0.10); shakeMag = std::max(shakeMag, 3.0); }
    }
}

// ============================================================================
// 士兵系统
// ============================================================================
void Game::ensureSoldiers(Tower& barrack)
{
    if (barrack.key != TK_BARRACKS) return;
    const TowerLevel& st = stats(barrack);

    int mine = 0;
    for (Soldier& s : soldiers) if (s.barrack == barrack.id && !s.dead) ++mine;
    if (mine >= st.count) return;

    Point rally = nearestPathPoint(barrack.x, barrack.y);
    for (int i = mine; i < st.count; ++i) {
        double ang = -3.14159265 / 2 + (i - (st.count - 1) / 2.0) * 0.6;
        Soldier s;
        s.id = nextId++;
        s.barrack = barrack.id;
        s.slot = i;
        s.x = rally.x + std::cos(ang) * 14;
        s.y = rally.y + std::sin(ang) * 8;
        s.rallyX = s.x; s.rallyY = s.y;
        s.hp = s.maxHp = st.soldierHp;
        s.dps = st.soldierDps;
        s.respawnT = 0;
        soldiers.push_back(s);
    }
}

void Game::updateSoldiers(double dt)
{
    for (Tower& t : towers) if (t.key == TK_BARRACKS) ensureSoldiers(t);

    for (Soldier& s : soldiers) {
        if (s.dead) {
            s.respawnT -= dt;
            if (s.respawnT <= 0) {
                if (s.barrack == -1) continue;
                Tower* b = findTower(s.barrack);
                if (!b) continue;
                const TowerLevel& st = stats(*b);
                s.dead = false;
                s.hp = s.maxHp = st.soldierHp;
                s.dps = st.soldierDps;
                s.x = s.rallyX; s.y = s.rallyY;
                s.target = -1;
                fxFlash(s.x, s.y - 14, 26, 0xffe066, 16);
            }
            continue;
        }

        s.hp = std::min(s.maxHp, s.hp + s.maxHp * 0.025 * dt);   // 回血

        Tower* b = findTower(s.barrack);
        int lv = b ? b->level : 1;
        double pullR = lv >= 4 ? 74 : 56;

        Monster* target = findMonster(s.target);
        if (!target || target->dead ||
            std::hypot(target->x - s.x, target->y - s.y) > pullR + 16) {
            s.target = -1;
            double bd = 1e9;
            for (Monster& m : monsters) {
                if (m.dead || m.engaged != -1) continue;
                double d = std::hypot(m.x - s.rallyX, m.y - s.rallyY);
                if (d < pullR && d < bd) { bd = d; s.target = m.id; target = &m; }
            }
        }

        if (target && !target->dead) {
            double d = std::hypot(target->x - s.x, target->y - s.y);
            if (d < 1) d = 1;
            s.face = target->x > s.x ? 1 : -1;
            if (d > 18) {
                s.x += (target->x - s.x) / d * 62 * dt;
                s.y += (target->y - s.y) / d * 62 * dt;
            } else {
                target->engaged = s.id;               // 拦截，怪物停住
                s.atkCd -= dt;
                s.swing = std::max(0.0, s.swing - dt * 4);
                if (s.atkCd <= 0) {
                    s.atkCd = 0.85;
                    s.swing = 1;
                    damageMonster(target, s.dps * 0.85, false, -1, false);
                    fxBurst((s.x + target->x) / 2, (s.y + target->y) / 2 - 15,
                            1, 0xffffff, 0, 8, 8);
                    target->retaliCd -= dt;
                    if (target->retaliCd <= 0) {
                        target->retaliCd = 0.9;
                        s.hp -= std::max(3.0, s.maxHp * 0.045) * (target->boss ? 2 : 1);
                        if (s.hp <= 0) {
                            s.dead = true;
                            s.respawnT = 6;
                            target->engaged = -1;
                            fxBurst(s.x, s.y - 12, 10, 0xc8b890, 3, 3.5, 32);
                        }
                    }
                }
            }
        } else {
            double d = std::hypot(s.rallyX - s.x, s.rallyY - s.y);
            if (d > 1) {
                s.x += (s.rallyX - s.x) / d * 45 * dt;
                s.y += (s.rallyY - s.y) / d * 45 * dt;
                s.face = 1;
            }
        }

        // 解除失效的拦截
        for (Monster& m : monsters) {
            if (m.engaged == s.id &&
                (m.dead || std::hypot(m.x - s.x, m.y - s.y) > pullR + 24))
                m.engaged = -1;
        }
    }
}

// ============================================================================
// 英雄技能
// ============================================================================
bool Game::canUseSkill(int key) const
{
    if (key < 0 || key >= SK_COUNT) return false;
    if (over) return false;
    if (itemCd[key] > 0) return false;
    return coin >= SKILL_DATA[key].cost;
}

bool Game::castSkill(int key, double x, double y)
{
    if (!canUseSkill(key)) {
        if (itemCd[key] > 0) say(_T("技能冷却中"));
        else say(_T("金币不足"));
        return false;
    }
    coin -= SKILL_DATA[key].cost;
    itemCd[key] = SKILL_DATA[key].cd;
    pendingSkill = -1;

    if (key == SK_METEOR) {
        shakeT = 0.5; shakeMag = 10;
        for (int i = 0; i < 6; ++i) {
            FXItem f;
            f.kind = 7;                        // 延迟陨石
            f.x = x + (rnd() - 0.5) * 130;
            f.y = y + (rnd() - 0.5) * 110;
            f.life = f.maxLife = i * 0.11 + 0.05;
            fxs.push_back(f);
        }
    }
    else if (key == SK_THUNDER) {
        shakeT = 0.25; shakeMag = 6;
        Monster* cur = nullptr;
        double bd = 1e9;
        for (Monster& m : monsters) {
            if (m.dead) continue;
            double d = std::hypot(m.x - x, m.y - y);
            if (d < 100 && d < bd) { bd = d; cur = &m; }
        }
        if (!cur) {
            fxArc(x, y - 160, x, y, 0xa0e0ff, 22);
            fxRing(x, y, 40, 0xa0e0ff, 20);
        } else {
            std::vector<int> hit;
            double dmg = 150;
            int cid = cur->id;
            for (int hop = 0; hop < 6 && cid != -1; ++hop) {
                Monster* c = findMonster(cid);
                if (!c || c->dead) break;
                hit.push_back(cid);
                fxArc(c->x + (rnd() - 0.5) * 20, c->y - 180, c->x, c->y - c->r, 0xc8f0ff, 16);
                fxArc(c->x, c->y - c->r, c->x, c->y, 0xffffff, 12);
                damageMonster(c, dmg, true, -1, false);
                fxBurst(c->x, c->y - c->r, 8, 0xc8f0ff, 4, 3, 18);
                dmg *= 0.76;
                int nxt = -1; double nd = 110;
                for (Monster& o : monsters) {
                    if (o.dead || std::find(hit.begin(), hit.end(), o.id) != hit.end()) continue;
                    double d = std::hypot(o.x - c->x, o.y - c->y);
                    if (d < nd) { nd = d; nxt = o.id; }
                }
                cid = nxt;
            }
            fxRing(x, y, 60, 0xc8f0ff, 24);
        }
    }
    else if (key == SK_MILITIA) {
        Point rally = nearestPathPoint(x, y);
        for (int i = 0; i < 3; ++i) {
            double ang = i * 3.14159265 * 2 / 3;
            Soldier s;
            s.id = nextId++;
            s.barrack = -1; s.slot = 100 + i;
            s.x = rally.x + std::cos(ang) * 20;
            s.y = rally.y + std::sin(ang) * 14;
            s.rallyX = s.x; s.rallyY = s.y;
            s.hp = s.maxHp = 220; s.dps = 30;
            s.temp = true; s.lifeT = 18;
            soldiers.push_back(s);
            fxFlash(s.x, s.y - 16, 34, 0xffe066, 20);
        }
        fxRing(rally.x, rally.y, 54, 0xffe066, 24);
        say(_T("援兵已降临!"));
    }
    else if (key == SK_FLAME) {
        FlameZone z;
        z.x = x; z.y = y; z.r = 92; z.time = z.maxTime = 4.5;
        flameZones.push_back(z);
        fxFlash(x, y, 92, 0xff8020, 34);
        fxRing(x, y, 92, 0xff8020, 26);
        for (Monster& m : monsters) {
            if (m.dead) continue;
            if (std::hypot(m.x - x, m.y - y) < 92) {
                damageMonster(&m, 70, true, -1, false);
                m.burnDps = 26; m.burnT = 4;
            }
        }
    }
    return true;
}

// ============================================================================
// 特效辅助
// ============================================================================
void Game::fxText(double x, double y, const TCHAR* text, unsigned int color, double size, double life)
{
    FXItem f;
    f.kind = 0; f.x = x; f.y = y;
    f.life = f.maxLife = life;
    f.size = size; f.color = color;
    _tcsncpy_s(f.textBuf, 48, text, _TRUNCATE);
    fxs.push_back(f);
}
void Game::fxNumber(double x, double y, int num, unsigned int color, double size)
{
    FXItem f;
    f.kind = 0; f.x = x; f.y = y;
    f.life = f.maxLife = 34;
    f.size = size; f.color = color;
    swprintf_s(f.textBuf, 48, _T("%d"), num);
    fxs.push_back(f);
}
void Game::fxBurst(double x, double y, int n, unsigned int color, double spd, double size, double life)
{
    for (int i = 0; i < n; ++i) {
        FXItem f;
        f.kind = 1; f.x = x; f.y = y;
        double a = rnd() * 6.2831853, v = (0.4 + rnd() * 0.9) * spd;
        f.vx = std::cos(a) * v; f.vy = std::sin(a) * v - 1;
        f.g = 0.06;
        f.life = f.maxLife = life * (0.7 + rnd() * 0.5);
        f.size = size;
        f.color = color;
        fxs.push_back(f);
    }
}
void Game::fxRing(double x, double y, double r, unsigned int color, double life)
{
    FXItem f;
    f.kind = 2; f.x = x; f.y = y; f.r = r; f.color = color;
    f.life = f.maxLife = life;
    fxs.push_back(f);
}
void Game::fxFlash(double x, double y, double r, unsigned int color, double life)
{
    FXItem f;
    f.kind = 3; f.x = x; f.y = y; f.r = r; f.color = color;
    f.life = f.maxLife = life;
    fxs.push_back(f);
}
void Game::fxArc(double x1, double y1, double x2, double y2, unsigned int color, double life)
{
    FXItem f;
    f.kind = 4; f.x = x1; f.y = y1; f.tx = x2; f.ty = y2;
    f.color = color; f.life = f.maxLife = life;
    fxs.push_back(f);
}
void Game::fxMushroom(double x, double y, double r)
{
    FXItem f;
    f.kind = 6; f.x = x; f.y = y; f.r = r;
    f.color = 0xff8020; f.life = f.maxLife = 38;
    fxs.push_back(f);
}
void Game::say(const TCHAR* msg, double seconds)
{
    _tcsncpy_s(toastMsg, 128, msg, _TRUNCATE);
    toastT = seconds * 60;
}
void Game::banner(const TCHAR* msg, double seconds)
{
    _tcsncpy_s(bannerMsg, 64, msg, _TRUNCATE);
    bannerT = seconds * 60;
}

void Game::updateFx(double dt)
{
    for (int i = (int)fxs.size() - 1; i >= 0; --i) {
        FXItem& f = fxs[i];
        f.life -= dt * 60;
        if (f.kind == 7) {                       // 延迟陨石：到时爆炸
            if (f.life <= 0) {
                fxFlash(f.x, f.y, 56, 0xffb040, 26);
                fxRing(f.x, f.y, 64, 0xffb040, 26);
                fxBurst(f.x, f.y, 18, 0xffe080, 6, 4.5, 30);
                fxMushroom(f.x, f.y, 40);
                shakeT = std::max(shakeT, 0.2); shakeMag = std::max(shakeMag, 5.0);
                for (Monster& m : monsters) {
                    if (!m.dead && std::hypot(m.x - f.x, m.y - f.y) < 66)
                        damageMonster(&m, 190, true, -1, false);
                }
                fxs.erase(fxs.begin() + i);
            }
            continue;
        }
        if (f.kind == 1) {
            f.x += f.vx; f.y += f.vy; f.vy += f.g;
        }
        if (f.life <= 0) fxs.erase(fxs.begin() + i);
    }
}

// ============================================================================
// 主更新：一帧逻辑
// ============================================================================
void Game::update(double dt)
{
    if (paused || over) return;
    time += dt;

    // 技能冷却 / 提示计时 / 震屏
    for (int i = 0; i < SK_COUNT; ++i)
        if (itemCd[i] > 0) itemCd[i] = std::max(0.0, itemCd[i] - dt);
    if (toastT > 0) toastT -= dt * 60;
    if (bannerT > 0) bannerT -= dt * 60;
    if (shakeT > 0) shakeT -= dt;

    // 1) 波次出生与结束判定
    updateWave(dt);

    // 2) 怪物：灼烧、减速、移动
    for (Monster& m : monsters) {
        if (m.dead) continue;
        if (m.burnT > 0) {
            m.burnT -= dt;
            damageMonster(&m, m.burnDps * dt, true, -1, false);
            if (m.burnT <= 0) m.burnDps = 0;
        }
        if (m.frozen > 0) m.frozen -= dt;
        if (m.flash > 0) m.flash -= dt * 60;
        moveMonster(&m, dt);
    }
    // 移除死亡怪物
    monsters.erase(std::remove_if(monsters.begin(), monsters.end(),
        [](const Monster& m) { return m.dead; }), monsters.end());

    // 3) 塔攻击（同时递减建造闪光计时）
    for (Tower& t : towers) {
        if (t.bornT > 0) t.bornT -= dt;
        updateTowerAttack(t, dt);
    }

    // 4) 士兵
    updateSoldiers(dt);
    // 临时援兵倒计时
    for (Soldier& s : soldiers) {
        if (s.temp && !s.dead) {
            s.lifeT -= dt;
            if (s.lifeT <= 0) { s.dead = true; s.respawnT = 1e9; }
        }
    }
    soldiers.erase(std::remove_if(soldiers.begin(), soldiers.end(),
        [](const Soldier& s) { return s.temp && s.dead; }), soldiers.end());

    // 5) 子弹
    updateBullets(dt);

    // 6) 火海区域：持续灼烧
    for (int i = (int)flameZones.size() - 1; i >= 0; --i) {
        FlameZone& z = flameZones[i];
        z.time -= dt;
        for (Monster& m : monsters) {
            if (!m.dead && std::hypot(m.x - z.x, m.y - z.y) < z.r) {
                m.burnDps = 26; m.burnT = std::max(m.burnT, 1.2);
            }
        }
        if (z.time <= 0) flameZones.erase(flameZones.begin() + i);
    }

    // 7) 冰锥丛：范围减速 + 轻微冰伤（椭圆判定）
    for (int i = (int)iceZones.size() - 1; i >= 0; --i) {
        IceZone& z = iceZones[i];
        z.life -= dt;
        if (z.bornT > 0) z.bornT -= dt;
        for (Monster& m : monsters) {
            if (m.dead) continue;
            double ddx = (m.x - z.x) / z.r;
            double ddy = (m.y - z.y) / (z.r * 0.55);
            if (ddx * ddx + ddy * ddy > 1) continue;
            m.slowT = std::max(m.slowT, z.slowT);
            m.slowRatio = std::max(m.slowRatio, z.slow);
            damageMonster(&m, z.dps * dt, true, -1, false);
        }
        if (z.life <= 0) iceZones.erase(iceZones.begin() + i);
    }

    // 8) 特效
    updateFx(dt);
}
