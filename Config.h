#pragma once
// ============================================================================
// Config.h  ——  全局配置与数值表
// 对应原版 [M1] 模块：所有可调数值集中于此，方便修改和讲解
// ============================================================================

#include <tchar.h>

// ---------- 地图像素/网格 ----------
const int CELL      = 62;                       // 格子边长
const int COLS      = 13;                       // 列数
const int ROWS      = 9;                        // 行数
const int MAP_W     = 806;                      // 地图宽 = 13 * 62
const int MAP_H     = 558;                      // 地图高 = 9  * 62
const int PAD       = 27;                       // 画布内边距
const int CANVAS_W  = MAP_W + PAD * 2;          // 860
const int CANVAS_H  = MAP_H + PAD * 2;          // 612

// ---------- 窗口布局（地图 + 左右面板 + 顶栏） ----------
const int WIN_W     = 1240;                     // 窗口总宽
const int WIN_H     = 720;                      // 窗口总高
const int TOP_H     = 66;                       // 顶部资源条高度
const int LEFT_W    = 176;                      // 左侧建造塔面板宽度
const int MAP_OX    = 190;                      // 地图左上角在窗口中的 x
const int MAP_OY    = 76;                       // 地图左上角在窗口中的 y
const int RIGHT_X   = 1064;                     // 右侧技能面板起始 x

// ---------- 游戏规则数值 ----------
const int    START_COIN = 320;                  // 初始金币
const int    START_HP   = 20;                   // 初始生命
const double SELL_RATE  = 0.65;                 // 出售返还比例
const int    WAVE_BONUS = 45;                   // 每波基础奖励
const int    WAVE_MAX   = 15;                   // 总波数

const double WAVE_SCALE_HP    = 0.26;           // 每波怪物血量 +26%
const double WAVE_SCALE_SPEED = 0.028;          // 每波怪物速度 +2.8%

// ---------- 基础结构 ----------
struct Point { double x, y; };

// ---------- 路径（格坐标，含地图外的进出场点） ----------
const int PATH_N      = 16;
const int PATH_CELLS[PATH_N][2] = {
    {-1,2},{3,2},{3,0},{7,0},{7,2},{9,2},{9,4},{1,4},
    {1,5},{1,6},{2,6},{8,6},{8,4},{10,4},{10,8},{13,8}
};
const int BASE_CELL[2] = {10, 7};               // 基地所在格

// ============================================================================
// 防御塔等级数值
// ============================================================================
struct TowerLevel {
    int   cost = 0;
    double dmg = 0, range = 0, rate = 1, projSpeed = 10;
    double aoe = 0, pierce = 0, crit = 0, multi = 1, armorPen = 0;
    double slow = 0, slowT = 0, freeze = 0, burn = 0;
    int   soldierHp = 0, soldierDps = 0, count = 0;
    const TCHAR* name = _T("");
};

// 辅助：按语义填充等级数据（用默认参数避免聚合初始化顺序出错）
inline TowerLevel mkLv(
    int cost, const TCHAR* name,
    double dmg = 0, double range = 0, double rate = 1, double projSpeed = 10,
    double aoe = 0, double pierce = 0, double crit = 0, double multi = 1,
    double armorPen = 0, double slow = 0, double slowT = 0,
    double freeze = 0, double burn = 0,
    int soldierHp = 0, int soldierDps = 0, int count = 0)
{
    TowerLevel L;
    L.cost = cost;      L.name = name;
    L.dmg = dmg;        L.range = range;    L.rate = rate;      L.projSpeed = projSpeed;
    L.aoe = aoe;        L.pierce = pierce;  L.crit = crit;      L.multi = multi;
    L.armorPen = armorPen;
    L.slow = slow;      L.slowT = slowT;    L.freeze = freeze;  L.burn = burn;
    L.soldierHp = soldierHp; L.soldierDps = soldierDps; L.count = count;
    return L;
}

// 塔类型枚举
enum TowerKind { TK_ARCHER, TK_MAGE, TK_BARRACKS, TK_ARTILLERY, TK_FROST, TK_SNIPER, TK_COUNT };
// 攻击类型
enum AtkType { ATK_PROJECTILE, ATK_MAGIC, ATK_BARRACKS };

struct TowerDef {
    const TCHAR* key;
    const TCHAR* name;
    const TCHAR* desc;
    int buildCost;
    AtkType type;
    unsigned int color;                 // 0xRRGGBB 主题色
    TowerLevel levels[4];               // 1~4 级
    const TCHAR* maxFx;
};

static const TowerDef TOWER_DATA[TK_COUNT] = {
    { _T("archer"), _T("箭塔"), _T("树上弓箭手，快速单体物理伤害"), 70, ATK_PROJECTILE, 0x8a5a2a,
      { mkLv(0,   _T("幼树箭台"), 16, 165, 0.62, 11),
        mkLv(60,  _T("双人哨树"), 30, 180, 0.52, 11),
        mkLv(110, _T("巨树哨站"), 52, 196, 0.42, 12, 0,0,0.15),
        mkLv(210, _T("皇家神树"), 92, 214, 0.34, 13, 0,0,0.30,3) },
      _T("三箭齐射 + 30% 暴击") },

    { _T("mage"), _T("法师塔"), _T("紫色电弧贯穿敌人，无视护甲"), 100, ATK_MAGIC, 0xa060e8,
      { mkLv(0,   _T("紫焰石柱"), 26, 155, 1.15, 8, 42),
        mkLv(90,  _T("紫焰法塔"), 48, 170, 1.05, 8, 50),
        mkLv(160, _T("紫焰尖塔"), 84, 186, 0.95, 9, 60),
        mkLv(300, _T("紫焰圣塔"),150, 205, 0.85,10, 78) },
      _T("奥术新星：巨大的魔法爆炸") },

    { _T("barracks"), _T("兵营"), _T("召唤士兵拦截怪物，近战肉搏"), 90, ATK_BARRACKS, 0xc8a040,
      { mkLv(0,   _T("简陋哨站"), 0,0,0,0, 0,0,0,1, 0,0,0,0,0, 110,13,2),
        mkLv(85,  _T("守卫营地"), 0,0,0,0, 0,0,0,1, 0,0,0,0,0, 190,20,2),
        mkLv(155, _T("骑士驻地"), 0,0,0,0, 0,0,0,1, 0,0,0,0,0, 300,30,3),
        mkLv(280, _T("圣殿军堡"), 0,0,0,0, 0,0,0,1, 0,0,0,0,0, 520,48,3) },
      _T("圣殿骑士：超强血量与攻击") },

    { _T("artillery"), _T("炮塔"), _T("熔岩炮弹从天而降，范围爆炸破甲"), 120, ATK_PROJECTILE, 0xe88030,
      { mkLv(0,   _T("熔岩石堆"), 38, 185, 1.75, 5.5, 62, 0.40),
        mkLv(110, _T("火山炮座"), 70, 200, 1.60, 5.8, 74, 0.55),
        mkLv(190, _T("岩浆要塞"),120, 218, 1.45, 6.2, 88, 0.70, 0,1, 0,0,0,12),
        mkLv(340, _T("炼狱火山"),210, 236, 1.30, 6.8,110, 1.00, 0,1, 0,0,0,20) },
      _T("爆心双倍伤害 + 完全破甲") },

    { _T("frost"), _T("冰霜塔"), _T("一群投冰手，冰弹落地生成冰锥丛减速"), 110, ATK_PROJECTILE, 0x6fd0f0,
      { mkLv(0,   _T("冰晶石台"), 12, 160, 1.20, 8,   34, 0,0,1, 0, 0.34,1.8),
        mkLv(95,  _T("霜寒哨站"), 22, 174, 1.10, 8.4, 42, 0,0,1, 0, 0.44,2.2),
        mkLv(170, _T("寒冰祭坛"), 38, 190, 1.00, 9,   52, 0,0,1, 0, 0.54,2.6,0.18),
        mkLv(300, _T("永冬冰宫"), 64, 206, 0.88, 9.6, 66, 0,0,1, 0, 0.66,3.0,0.35) },
      _T("霜冻新星：大范围减速 66% + 35% 概率冻结") },

    { _T("sniper"), _T("狙击塔"), _T("高耸瞭望塔，红色子弹穿透护甲"), 140, ATK_PROJECTILE, 0xb04878,
      { mkLv(0,   _T("简易瞭望台"), 46, 265, 2.10, 20, 0,0,0,1, 0.35),
        mkLv(130, _T("木石瞭望塔"), 82, 290, 1.95, 21, 0,0,0,1, 0.50),
        mkLv(210, _T("高耸瞭望塔"),140, 315, 1.80, 22, 0,0,0.20,1, 0.70),
        mkLv(380, _T("要塞瞭望塔"),240, 345, 1.60, 24, 0,0,0.40,1, 1.00) },
      _T("破甲狙击：完全无视护甲 + 40% 暴击") },
};

// ============================================================================
// 怪物数据
// ============================================================================
enum MonsterKind { MK_ORC, MK_GOBLIN, MK_SKELETON, MK_SHADOW, MK_ARMORED, MK_BOSS, MK_COUNT };
struct MonsterDef {
    const TCHAR* name;
    double hp, speed;
    int armor, reward, r;
    bool boss;
    unsigned int color;
};
static const MonsterDef MONSTER_DATA[MK_COUNT] = {
    { _T("兽人"),      78,  52,  0,   9, 13, false, 0x6aa848 },
    { _T("哥布林"),    46,  78,  0,   7, 10, false, 0x9ac848 },
    { _T("骷髅"),      58,  64,  1,   8, 11, false, 0xe0dcc8 },
    { _T("暗影"),      52, 112,  0,  12, 10, false, 0x7048a0 },
    { _T("重甲战士"), 300,  36,  9,  26, 14, false, 0x88949c },
    { _T("巨魔首领"),2400,  25, 13, 120, 24, true,  0x9a4020 },
};

// ============================================================================
// 波次编排：[怪物类型, 数量, 间隔秒]
// ============================================================================
struct SpawnGroup { MonsterKind type; int count; double gap; };
static const SpawnGroup WAVE_DATA[WAVE_MAX][2] = {
    { {MK_ORC,7,1.1},                    {MK_ORC,0,0} },
    { {MK_ORC,8,0.9},                    {MK_GOBLIN,6,0.55} },
    { {MK_GOBLIN,10,0.45},               {MK_ORC,7,0.75} },
    { {MK_SKELETON,10,0.5},              {MK_ORC,8,0.65} },
    { {MK_ARMORED,3,1.7},                {MK_ORC,10,0.6} },
    { {MK_SHADOW,9,0.45},                {MK_SKELETON,8,0.5} },
    { {MK_ORC,12,0.5},                   {MK_ARMORED,4,1.3} },
    { {MK_SHADOW,14,0.32},               {MK_GOBLIN,10,0.4} },
    { {MK_ARMORED,6,1.1},                {MK_SKELETON,10,0.42} },
    { {MK_BOSS,1,0},                     {MK_ORC,10,0.55} },
    { {MK_GOBLIN,16,0.28},               {MK_SHADOW,8,0.35} },
    { {MK_SKELETON,16,0.32},             {MK_ARMORED,7,1.0} },
    { {MK_SHADOW,16,0.26},               {MK_ORC,14,0.4} },
    { {MK_ARMORED,10,0.8},               {MK_SKELETON,14,0.3} },
    { {MK_BOSS,2,7},                     {MK_ARMORED,10,0.7} },
};
// 每波实际组数（1 或 2）
static const int WAVE_GROUP_N[WAVE_MAX] = {
    1,2,2,2,2,2,2,2,2,2,2,2,2,2,2
};

// ============================================================================
// 英雄技能
// ============================================================================
enum SkillKind { SK_METEOR, SK_THUNDER, SK_MILITIA, SK_FLAME, SK_COUNT };
struct SkillDef {
    const TCHAR* name;
    const TCHAR* desc;
    int cost, cd;
    unsigned int color;
};
static const SkillDef SKILL_DATA[SK_COUNT] = {
    { _T("天降陨石"), _T("点击区域，陨石砸落造成巨额范围伤害"), 100, 28, 0xff8a20 },
    { _T("闪电风暴"), _T("点击怪物，闪电连锁跳跃打击"),         80, 24, 0x60c0ff },
    { _T("援兵降临"), _T("点击路径，召唤援兵落地阻挡怪物"),     70, 32, 0xe0c040 },
    { _T("烈焰冲击"), _T("点击区域，烈焰持续灼烧范围内敌人"),   90, 26, 0xe04020 },
};
