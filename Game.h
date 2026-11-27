#pragma once
// ============================================================================
// Game.h  ——  游戏状态与实体系统
// 对应原版 [M9]~[M13]：状态、建造、战斗结算、士兵、技能、波次
// ============================================================================

#include "Config.h"
#include <vector>
#include <tchar.h>

// ---------- 实体：防御塔 ----------
struct Tower {
    int    id = 0;
    int    key = 0;                     // TowerKind
    int    c = 0, r = 0;                // 格坐标
    double x = 0, y = 0;                // 地图像素坐标
    int    level = 1;                   // 1..4
    double cool = 0;                    // 攻击冷却
    double aim = -0.5;                  // 炮口朝向
    double bornT = 1.0;                 // 建造闪光动画
    double breathPhase = 0;             // 呼吸动画相位
    int    totalSpent = 0;              // 累计花费（用于出售返还）
};

// ---------- 实体：怪物 ----------
struct Monster {
    int    id = 0;
    int    type = 0;                    // MonsterKind
    double x = 0, y = 0;
    int    seg = 0;                     // 当前所在路径段
    double segT = 0;                    // 段内已走距离
    double hp = 0, maxHp = 0, speed = 0;
    int    armor = 0, reward = 0, r = 10;
    bool   boss = false;
    double slowT = 0, slowRatio = 0;    // 减速剩余时间与比例
    double burnDps = 0, burnT = 0;      // 灼烧
    double frozen = 0;                  // 冻结剩余时间
    double flash = 0;                   // 受击闪白
    bool   dead = false;
    double deathT = 0;                  // 死亡动画计时
    double walkPhase = 0;               // 走路动画相位
    int    engaged = -1;                // 被哪个士兵拦截（soldier id）
    double retaliCd = 0;                // 反击冷却
};

// ---------- 实体：子弹 ----------
struct Bullet {
    int    kind = 0;                    // 0箭 1魔法(瞬时) 2炮弹 3冰弹 4狙击 5空投炸弹
    double x = 0, y = 0;
    int    target = -1;                 // 目标怪物 id
    double speed = 0;
    TowerLevel st;                      // 发射瞬间的等级数值快照
    int    towerId = -1;
    double spin = 0, vy = 1, trailT = 0, angle = 0;
    double px = 0, py = 0;              // 上一帧位置（画拖尾）
    double life = 1;
    double delay = 0;                   // 延迟发射（三箭齐射）
};

// ---------- 实体：士兵 ----------
struct Soldier {
    int    id = 0;
    int    barrack = -1;                // 所属兵营 tower id；-1 表示技能援兵
    int    slot = 0;
    double x = 0, y = 0, rallyX = 0, rallyY = 0;
    double hp = 0, maxHp = 0, dps = 0;
    int    target = -1;                 // 目标怪物 id
    double atkCd = 0, swing = 0;
    bool   dead = false;
    double respawnT = 0;                // 复活倒计时
    int    face = 1;                    // 朝向
    bool   temp = false;                // 技能召唤的临时援兵
    double lifeT = 0;                   // 临时援兵剩余时间
};

// ---------- 地面区域：冰锥丛 / 火海 ----------
struct Spike { double dx, dy, h, w, tilt; };
struct IceZone {
    double x = 0, y = 0, r = 0;
    double life = 0, maxLife = 0;
    double slow = 0, slowT = 0, dps = 0;
    double bornT = 0;
    std::vector<Spike> spikes;
};
struct FlameZone { double x = 0, y = 0, r = 0, time = 0, maxTime = 0; };

// ---------- 特效条目 ----------
// kind: 0飘字 1粒子 2光环 3闪光 4电弧 5闪电柱 6蘑菇云
struct FXItem {
    int    kind = 0;
    double x = 0, y = 0;
    double vx = 0, vy = 0, g = 0;
    double life = 0, maxLife = 0;
    double size = 0, r = 0, h = 0;
    double tx = 0, ty = 0;              // 电弧终点
    TCHAR  textBuf[48] = {0};           // 飘字内容（内嵌，避免 vector 扩容导致指针失效）
    unsigned int color = 0xffffff;
};

// ---------- 出生队列 ----------
struct SpawnItem { MonsterKind type; double delay; };

// ============================================================================
// Game 类
// ============================================================================
class Game {
public:
    // ---------- 状态 ----------
    int coin = START_COIN;
    int hp = START_HP;
    int wave = 0;
    bool waveActive = false;
    double spawnTimer = 0;              // 当前波已推进时间
    bool over = false;
    bool paused = false;
    bool win = false;

    double time = 0;                    // 累计时间（动画用）
    int speed = 1;                      // 1/2/3 倍速

    int selectedTowerKey = -1;          // 当前选中的塔类型（建造模式）
    int selectedTowerId = -1;           // 当前打开操作面板的塔
    int pendingSkill = -1;              // 待释放的技能（瞄准模式）

    double mouseX = 0, mouseY = 0;      // 鼠标在地图坐标系的位置
    int hoverC = -1, hoverR = -1;

    double shakeT = 0, shakeMag = 0;    // 震屏

    TCHAR toastMsg[128] = {0};
    double toastT = 0;
    TCHAR bannerMsg[64] = {0};
    double bannerT = 0;

    double itemCd[SK_COUNT] = {0,0,0,0};

    std::vector<Tower>    towers;
    std::vector<Monster>  monsters;
    std::vector<Bullet>   bullets;
    std::vector<Soldier>  soldiers;
    std::vector<IceZone>  iceZones;
    std::vector<FlameZone> flameZones;
    std::vector<FXItem>   fxs;
    std::vector<SpawnItem> spawnQueue;

    // ---------- 方法 ----------
    Game();

    void newGame();                     // 重置全部状态
    void update(double dt);             // 推进一帧逻辑

    // 地图 / 建造
    bool buildable(int c, int r) const;                 // 该格是否可建塔
    static Point cellCenter(int c, int r);              // 格中心 → 像素坐标
    static Point pathPoint(int i);                      // 路径点 → 像素坐标
    bool tryBuild(int c, int r);                        // 尝试建造
    bool upgradeTower(int towerId);                     // 升级
    bool sellTower(int towerId);                        // 出售

    // 查询
    Tower*    findTower(int id);
    Monster*  findMonster(int id);
    Soldier*  findSoldier(int id);
    Tower*    towerAt(double x, double y, double radius = 32);
    const TowerLevel& stats(const Tower& t) const;

    // 战斗 / 波次 / 技能
    void startWave();
    bool castSkill(int key, double x, double y);        // 返回是否成功释放
    bool canUseSkill(int key) const;

    // 特效辅助
    void fxText(double x, double y, const TCHAR* text, unsigned int color, double size, double life);
    void fxNumber(double x, double y, int num, unsigned int color, double size = 14);
    void fxBurst(double x, double y, int n, unsigned int color, double spd, double size, double life);
    void fxRing(double x, double y, double r, unsigned int color, double life);
    void fxFlash(double x, double y, double r, unsigned int color, double life);
    void fxArc(double x1, double y1, double x2, double y2, unsigned int color, double life);
    void fxMushroom(double x, double y, double r);

    void say(const TCHAR* msg, double seconds = 1.8);   // 飘字提示
    void banner(const TCHAR* msg, double seconds = 1.4);// 波次公告

private:
    int nextId = 1;                     // 实体 id 生成器

    void updateWave(double dt);
    void spawnMonster(MonsterKind type);
    void damageMonster(Monster* m, double dmg, bool ignoreArmor, double pierce, bool crit);
    void killMonster(Monster* m);
    void leakMonster(Monster* m);
    void moveMonster(Monster* m, double dt);

    void updateTowerAttack(Tower& t, double dt);
    void updateBullets(double dt);
    void onBulletHit(Bullet& b, Monster* m);
    void spawnIceSpikes(double x, double y, double r, const TowerLevel& st);

    void ensureSoldiers(Tower& barrack);
    void updateSoldiers(double dt);
    static Point nearestPathPoint(double x, double y);

    void updateFx(double dt);

    friend class Renderer;              // 渲染器直接读取状态
};
