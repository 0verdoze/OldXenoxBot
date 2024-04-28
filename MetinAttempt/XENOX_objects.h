#pragma once
#include <deque>
#include <map>
#include <list>
#include <unordered_map>
#include <vector>
#include <set>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#include "PlayerActions.h"

    
constexpr size_t XENOX_BASE = 0x00400000;

static std::map<std::string, std::pair<size_t, std::vector<int>>> ObjectPointers  // pair { offset from base address, {offsets from previous ptr deref} }
{
    { "CPythonCharacterManager", { 0x004d9b1c, { 0x0 }} },  // { 0x004d9b1c, 0x0 }, { 0x004d7860, 0x4 }
    { "CPythonPlayer", { 0x004d785c, { 0x0 }} },
    { "CPythonItem", { 0x004d9bf0, { 0x0 }} },
    { "CurrentAction", { 0x004d9b1c, { 0x10, 0x45c }} }, // CPythonCharacterManager + 0x08 + 0x45c
};

class CPythonCharacterManager;
class CPythonPlayer;
class CPythonItem;

template<class T>
inline T* GetObjectPointer()
{
    std::string name;

    if constexpr (std::is_same<T, CPythonCharacterManager>::value)
        name = "CPythonCharacterManager";
    else if constexpr (std::is_same<T, CPythonPlayer>::value)
        name = "CPythonPlayer";
    else if constexpr (std::is_same<T, CPythonItem>::value)
        name = "CPythonItem";
    // else if constexpr (std::is_same<T, CurrentAction>::value)
    //     name = "CurrentAction";
    else if constexpr (true)
        static_assert(false, "Not invalid object name");

    try
    {
        const auto& r = ObjectPointers.at(name);
        int* ptr = (int*)(XENOX_BASE + r.first);

        for (size_t i = 0; i < r.second.size(); i++)
        {
            int val = *ptr;
            if (!val)
                return 0;

            ptr = (int*)(val + r.second[i]);
        }

        return (T*)ptr;
    }
    catch (...) { return 0; }
}

struct D3DXVECTOR2
{
    FLOAT x, y;
};

struct D3DXVECTOR3
{
    FLOAT x, y, z;
};

typedef struct D3DXVECTOR4
{
    FLOAT x, y, z, w;
} D3DXQUATERNION;


struct D3DXMATRIX {
    union {
        struct {
            float        _11, _12, _13, _14;
            float        _21, _22, _23, _24;
            float        _31, _32, _33, _34;
            float        _41, _42, _43, _44;

        };
        float m[4][4];
    };
};

struct D3DXCOLOR
{
    FLOAT r, g, b, a;
};

typedef D3DXVECTOR3 TPixelPosition;

// XenoxMT2 pragma, count uint16_t instead of byte
#pragma pack(push, 1)
typedef struct TPlayerItemAttribute
{
    BYTE        bType;   // type of ulepszenie
    short       sValue;  // +15, 25000 etc - value
} TPlayerItemAttribute;

typedef struct packet_item
{
    DWORD       vnum;
    uint16_t    count;
    DWORD        flags;
    DWORD        anti_flags;
    long        alSockets[3];
    TPlayerItemAttribute aAttr[7];
} TItemData;
#pragma pack(pop)

typedef struct SQuickSlot
{
    BYTE Type;
    BYTE Position;
} TQuickSlot;

class CAffectFlagContainer
{
public:
    enum
    {
        BIT_SIZE = 64,
        BYTE_SIZE = BIT_SIZE / 8 + (1 * ((BIT_SIZE & 7) ? 1 : 0)),
    };

private:
    typedef unsigned char Element;

    Element m_aElement[BYTE_SIZE];
};

template<typename T>
class CDynamicPool
{
protected:
    std::vector<T*> m_kVct_pkData;
    std::vector<T*> m_kVct_pkFree;

    UINT m_uInitCapacity;
    UINT m_uUsedCapacity;
};

struct CDynamicSphereInstance
{
    D3DXVECTOR3 v3Position;
    D3DXVECTOR3 v3LastPosition;

    float fRadius;
};


class CEaseOutInterpolation
{
protected:
    float m_fRemainingTime;
    float m_fValue;
    float m_fSpeed;
    float m_fAcceleration;

    float m_fStartValue;
    float m_fLastValue;
};

class CPhysicsObject
{
protected:
    float m_fMass;
    float m_fFriction;
    D3DXVECTOR3 m_v3Direction;
    D3DXVECTOR3 m_v3Acceleration;
    D3DXVECTOR3 m_v3Velocity;

    D3DXVECTOR3 m_v3LastPosition;
    CEaseOutInterpolation m_xPushingPosition;
    CEaseOutInterpolation m_yPushingPosition;

    void* m_pActorInstance;
};

class CGraphicCollisionObject
{

};

class CGraphicObjectInstance : public CGraphicCollisionObject
{
public:
    D3DXVECTOR3                m_v3Position;
    D3DXVECTOR3                m_v3Scale;

    float                    m_fYaw;
    float                    m_fPitch;
    float                    m_fRoll;

    D3DXMATRIX                m_mRotation;

    bool                    m_isVisible;
    D3DXMATRIX                m_worldMatrix;

    // Camera Block
    bool                    m_BlockCamera;

    // Bounding Box
    D3DXVECTOR4                m_v4TBBox[8];
    D3DXVECTOR3                m_v3TBBoxMin, m_v3TBBoxMax;
    D3DXVECTOR3                m_v3BBoxMin, m_v3BBoxMax;

    // Portal
    BYTE                    m_abyPortalID[8];

    // Culling
    void* m_CullingHandle;

    std::vector<void*>    m_StaticCollisionInstanceVector;

    void* m_pHeightAttributeInstance;
};

class CGraphicThingInstance : public CGraphicObjectInstance
{
public:
    typedef struct SModelThingSet
    {
        std::vector<void*>    m_pLODThingRefVector;
    } TModelThingSet;

public:
    enum
    {
        ID = 75676576
    };
public:
    bool                                    m_bUpdated;
    float                                    m_fLastLocalTime;
    float                                    m_fLocalTime;
    float                                    m_fDelay;
    float                                    m_fSecondElapsed;
    float                                    m_fAverageSecondElapsed;
    float                                    m_fRadius;
    D3DXVECTOR3                                m_v3Center;
    D3DXVECTOR3                                m_v3Min, m_v3Max;

    std::vector<void*>        m_LODControllerVector;
    std::vector<TModelThingSet>                m_modelThingSetVector;
    std::map<DWORD, void*>    m_roMotionThingMap;


    static CDynamicPool<CGraphicThingInstance>        ms_kPool;

};

class IActorInstance : public CGraphicThingInstance
{
public:
    enum
    {
        ID = 0xabababa
    };
};

class IFlyTargetableObject;

class CFlyTarget // final
{
public:
    enum EType
    {
        TYPE_NONE,
        TYPE_OBJECT,
        TYPE_POSITION,
    };

    mutable D3DXVECTOR3 m_v3FlyTargetPosition;
    IFlyTargetableObject* m_pFlyTarget;

    EType m_eType;
};


class IFlyTargetableObject
{
public:
    friend class CFlyTarget;

    std::set<CFlyTarget*> m_FlyTargeterSet;
};

class CActorInstance : public IActorInstance, public IFlyTargetableObject
{
private:
    uint8_t padding_0x0[0x5c - 16];
public:
    typedef std::vector<CDynamicSphereInstance> CDynamicSphereInstanceVector;

    class IEventHandler
    {
    public:
        struct SState
        {
            TPixelPosition kPPosSelf;
            FLOAT fAdvRotSelf;
        };
    };


    enum EType
    {
        TYPE_ENEMY,
        TYPE_NPC,
        TYPE_STONE,
        TYPE_WARP,
        TYPE_DOOR,
        TYPE_BUILDING,
        TYPE_PC,
        TYPE_POLY,
        TYPE_HORSE,
        TYPE_GOTO,

        TYPE_OBJECT, // Only For Client
    };

    enum ERenderMode
    {
        RENDER_MODE_NORMAL,
        RENDER_MODE_BLEND,
        RENDER_MODE_ADD,
        RENDER_MODE_MODULATE,
    };

    /////////////////////////////////////////////////////////////////////////////////////
    // Motion Queueing System
    enum EMotionPushType
    {
        MOTION_TYPE_NONE,
        MOTION_TYPE_ONCE,
        MOTION_TYPE_LOOP,
    };

    typedef struct SReservingMotionNode
    {
        EMotionPushType    iMotionType;

        float            fStartTime;
        float            fBlendTime;
        float            fDuration;
        float            fSpeedRatio;

        DWORD            dwMotionKey;
    } TReservingMotionNode;

    struct SCurrentMotionNode
    {
        EMotionPushType    iMotionType;
        DWORD            dwMotionKey;

        DWORD            dwcurFrame;
        DWORD            dwFrameCount;

        float            fStartTime;
        float            fEndTime;
        float            fSpeedRatio;

        int                iLoopCount;
        UINT            uSkill;
    };

    typedef std::deque<TReservingMotionNode> TMotionDeque;
    /////////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////////
    // Motion Event
    typedef struct SMotionEventInstance
    {
        int iType;
        int iMotionEventIndex;
        float fStartingTime;

        const void* c_pMotionData;
    } TMotionEventInstance;

    typedef std::list<TMotionEventInstance> TMotionEventInstanceList;
    typedef TMotionEventInstanceList::iterator TMotionEventInstanceListIterator;
    /////////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////////
    // For Collision Detection
    typedef struct SCollisionPointInstance
    {
        const void* c_pCollisionData;
        BOOL isAttached;
        DWORD dwModelIndex;
        DWORD dwBoneIndex;
        CDynamicSphereInstanceVector SphereInstanceVector;
    } TCollisionPointInstance;
    typedef std::list<TCollisionPointInstance> TCollisionPointInstanceList;
    typedef TCollisionPointInstanceList::iterator TCollisionPointInstanceListIterator;

    typedef std::map<CActorInstance*, float> THittedInstanceMap;
    typedef std::map<const void*, THittedInstanceMap> THitDataMap;
    struct SSplashArea
    {
        BOOL isEnableHitProcess;
        UINT uSkill;
        DWORD MotionKey;
        float fDisappearingTime;
        const void* c_pAttackingEvent;
        CDynamicSphereInstanceVector SphereInstanceVector;

        THittedInstanceMap HittedInstanceMap;
    };

    typedef struct SHittingData
    {
        BYTE byAttackingType;
        DWORD dwMotionKey;
        BYTE byEventIndex;
    } THittingData;
    /////////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////////
    // For Attaching
    enum EAttachEffect
    {
        EFFECT_LIFE_NORMAL,
        EFFECT_LIFE_INFINITE,
        EFFECT_LIFE_WITH_MOTION,
    };

    struct TAttachingEffect
    {
        DWORD dwEffectIndex;
        int iBoneIndex;
        DWORD dwModelIndex;
        D3DXMATRIX matTranslation;
        BOOL isAttaching;

        int iLifeType;
        DWORD dwEndTime;
    };
    /////////////////////////////////////////////////////////////////////////////////////

public:

    void* m_pFlyEventHandler;
    CFlyTarget m_kFlyTarget;
    CFlyTarget m_kBackupFlyTarget;
    std::deque<CFlyTarget> m_kQue_kFlyTarget;

    struct SSetMotionData
    {
        DWORD        dwMotKey;
        float        fSpeedRatio;
        float        fBlendTime;
        int            iLoopCount;
        UINT        uSkill;
    };

    TMotionDeque                    m_MotionDeque;
    SCurrentMotionNode                m_kCurMotNode;
    uint8_t    fix_padding[12];
    WORD                            m_wcurMotionMode;

    TCollisionPointInstanceList        m_BodyPointInstanceList;
    TCollisionPointInstanceList        m_DefendingPointInstanceList;
    SSplashArea                        m_kSplashArea;
    void* m_pAttributeInstance;

    std::vector<void*>    m_WeaponTraceVector;
    CPhysicsObject                m_PhysicsObject;

    DWORD                        m_dwcurComboIndex;

    DWORD                        m_eActorType;

    DWORD                        m_eRace;
    DWORD                        m_eShape;
    DWORD                        m_eHair;
    BOOL                        m_isPreInput;
    BOOL                        m_isNextPreInput;
    DWORD                        m_dwcurComboBackMotionIndex;

    WORD                        m_wcurComboType;

    float                        m_fAtkDirRot;

    void* m_pkCurRaceData;
    void* m_pkCurRaceMotionData;

    // Defender
    float                        m_fInvisibleTime;
    BOOL                        m_isHiding;

    // TODO : State로 통합 시킬 수 있는지 고려해 볼것
    BOOL                        m_isResistFallen;
    BOOL                        m_isSleep;
    BOOL                        m_isFaint;
    BOOL                        m_isParalysis;
    BOOL                        m_isStun;
    BOOL                        m_isRealDead;
    BOOL                        m_isWalking;
    BOOL                        m_isMain;

    // Effect
    DWORD                        m_dwBattleHitEffectID;
    DWORD                        m_dwBattleAttachEffectID;
    /////////////////////////////////////////////////////////////////////////////////////

    // Fishing
    D3DXVECTOR3                    m_v3FishingPosition;
    int                            m_iFishingEffectID;

    // Position
    union
    {
        D3DXVECTOR3 m_currentPos;
        struct {
            float                        m_x;
            float                        m_y;
            float                        m_z;
        };
    };

    D3DXVECTOR3                    m_v3Pos;
    D3DXVECTOR3                    m_v3Movement;
    BOOL                        m_bNeedUpdateCollision;

    DWORD                        m_dwShakeTime;

    float                        m_fReachScale;
    float                        m_fMovSpd;
    float                        m_fAtkSpd;

    // Rotation
    float                        m_fcurRotation;
    float                        m_rotBegin;
    float                        m_rotEnd;
    float                        m_rotEndTime;
    float                        m_rotBeginTime;
    float                        m_rotBlendTime;
    float                        m_fAdvancingRotation;
    float                        m_rotX;
    float                        m_rotY;

    float m_fOwnerBaseTime;

    // Rendering
    int                            m_iRenderMode;
    D3DXCOLOR                    m_AddColor;
    float                        m_fAlphaValue;

    // Part
    DWORD                        m_adwPartItemID[5];

    // Attached Effect
    std::list<TAttachingEffect> m_AttachingEffectList;
    bool                        m_bEffectInitialized;

    // material color
    DWORD                        m_dwMtrlColor;
    DWORD                        m_dwMtrlAlpha;

    TPixelPosition                m_kPPosCur;
    TPixelPosition                m_kPPosSrc;
    TPixelPosition                m_kPPosDst;
    TPixelPosition                m_kPPosAtk;

    TPixelPosition                m_kPPosLast;

    THitDataMap                    m_HitDataMap;

    CActorInstance* m_pkHorse;
    void* m_pkTree;

    DWORD m_dwSelfVID;
    DWORD m_dwOwnerVID;


    bool m_canSkipCollision;

    struct SBlendAlpha
    {
        float m_fBaseTime;
        float m_fBaseAlpha;
        float m_fDuration;
        float m_fDstAlpha;

        DWORD m_iOldRenderMode;
        bool m_isBlending;
    } m_kBlendAlpha;



public:


protected:
    IEventHandler* m_pkEventHandler;

protected:
    static bool ms_isDirLine;
};

class CInstanceBase
{
public:
    struct SCreateData
    {
        BYTE    m_bType;
        DWORD    m_dwStateFlags;
        DWORD    m_dwEmpireID;
        DWORD    m_dwGuildID;
        DWORD    m_dwLevel;
        DWORD    m_dwVID;
        DWORD    m_dwRace;
        DWORD    m_dwMovSpd;
        DWORD    m_dwAtkSpd;
        LONG    m_lPosX;
        LONG    m_lPosY;
        FLOAT    m_fRot;
        DWORD    m_dwArmor;
        DWORD    m_dwWeapon;
        DWORD    m_dwHair;
        DWORD    m_dwMountVnum;

        short    m_sAlignment;
        BYTE    m_byPKMode;
        CAffectFlagContainer    m_kAffectFlags;

        std::string m_stName;

        bool    m_isMain;
    };

public:
    typedef DWORD TType;

    enum EDirection
    {
        DIR_NORTH,
        DIR_NORTHEAST,
        DIR_EAST,
        DIR_SOUTHEAST,
        DIR_SOUTH,
        DIR_SOUTHWEST,
        DIR_WEST,
        DIR_NORTHWEST,
        DIR_MAX_NUM,
    };

    enum
    {
        FUNC_WAIT,
        FUNC_MOVE,
        FUNC_ATTACK,
        FUNC_COMBO,
        FUNC_MOB_SKILL,
        FUNC_EMOTION,
        FUNC_SKILL = 0x80,
    };

    enum
    {
        AFFECT_YMIR,
        AFFECT_INVISIBILITY,
        AFFECT_SPAWN,

        AFFECT_POISON,
        AFFECT_SLOW,
        AFFECT_STUN,

        AFFECT_DUNGEON_READY,            // ´řŔüżˇĽ­ ÁŘşń »óĹÂ
        AFFECT_SHOW_ALWAYS,                // AFFECT_DUNGEON_UNIQUE żˇĽ­ şŻ°ć(Ĺ¬¶óŔĚľđĆ®żˇĽ­ ÄĂ¸µµÇÁöľĘŔ˝)

        AFFECT_BUILDING_CONSTRUCTION_SMALL,
        AFFECT_BUILDING_CONSTRUCTION_LARGE,
        AFFECT_BUILDING_UPGRADE,

        AFFECT_MOV_SPEED_POTION,        // 11
        AFFECT_ATT_SPEED_POTION,        // 12

        AFFECT_FISH_MIND,                // 13

        AFFECT_JEONGWI,                    // 14 Ŕü±ÍČĄ
        AFFECT_GEOMGYEONG,                // 15 °Ë°ć
        AFFECT_CHEONGEUN,                // 16 Ăµ±ŮĂß
        AFFECT_GYEONGGONG,                // 17 °ć°řĽú
        AFFECT_EUNHYEONG,                // 18 ŔşÇüąý
        AFFECT_GWIGEOM,                    // 19 ±Í°Ë
        AFFECT_GONGPO,                    // 20 °řĆ÷
        AFFECT_JUMAGAP,                    // 21 ÁÖ¸¶°©
        AFFECT_HOSIN,                    // 22 ČŁ˝Ĺ
        AFFECT_BOHO,                    // 23 ş¸ČŁ
        AFFECT_KWAESOK,                    // 24 ÄčĽÓ
        AFFECT_HEUKSIN,                    // 25 Čć˝ĹĽöČŁ
        AFFECT_MUYEONG,                    // 26 ą«żµÁř
        AFFECT_REVIVE_INVISIBILITY,        // 27 şÎČ° ą«Ŕű
        AFFECT_FIRE,                    // 28 ÁöĽÓ şŇ
        AFFECT_GICHEON,                    // 29 ±âĂµ ´ë°ř
        AFFECT_JEUNGRYEOK,                // 30 Áő·ÂĽú 
        AFFECT_DASH,                    // 31 ´ë˝¬
        AFFECT_PABEOP,                    // 32 ĆÄąýĽú
        AFFECT_FALLEN_CHEONGEUN,        // 33 ´Ůżî ±×·ąŔĚµĺ Ăµ±ŮĂß
        AFFECT_POLYMORPH,                // 34 Ćú¸®¸đÇÁ
        AFFECT_WAR_FLAG1,                // 35
        AFFECT_WAR_FLAG2,                // 36
        AFFECT_WAR_FLAG3,                // 37
        AFFECT_CHINA_FIREWORK,            // 38
        AFFECT_PREMIUM_SILVER,
        AFFECT_PREMIUM_GOLD,
        AFFECT_RAMADAN_RING,            // 41 ĂĘ˝Â´Ţ ąÝÁö Âřżë Affect

        AFFECT_NUM = 64,

        AFFECT_HWAYEOM = AFFECT_GEOMGYEONG,
    };

    enum
    {
        NEW_AFFECT_MOV_SPEED = 200,
        NEW_AFFECT_ATT_SPEED,
        NEW_AFFECT_ATT_GRADE,
        NEW_AFFECT_INVISIBILITY,
        NEW_AFFECT_STR,
        NEW_AFFECT_DEX,                 // 205
        NEW_AFFECT_CON,
        NEW_AFFECT_INT,
        NEW_AFFECT_FISH_MIND_PILL,

        NEW_AFFECT_POISON,
        NEW_AFFECT_STUN,                // 210
        NEW_AFFECT_SLOW,
        NEW_AFFECT_DUNGEON_READY,
        NEW_AFFECT_DUNGEON_UNIQUE,

        NEW_AFFECT_BUILDING,
        NEW_AFFECT_REVIVE_INVISIBLE,    // 215
        NEW_AFFECT_FIRE,
        NEW_AFFECT_CAST_SPEED,
        NEW_AFFECT_HP_RECOVER_CONTINUE,
        NEW_AFFECT_SP_RECOVER_CONTINUE,

        NEW_AFFECT_POLYMORPH,           // 220
        NEW_AFFECT_MOUNT,

        NEW_AFFECT_WAR_FLAG,            // 222

        NEW_AFFECT_BLOCK_CHAT,          // 223
        NEW_AFFECT_CHINA_FIREWORK,

        NEW_AFFECT_BOW_DISTANCE,        // 225

        NEW_AFFECT_EXP_BONUS = 500, // °ćÇčŔÇ ąÝÁö
        NEW_AFFECT_ITEM_BONUS = 501, // µµµĎŔÇ Ŕĺ°©
        NEW_AFFECT_SAFEBOX = 502, // PREMIUM_SAFEBOX,
        NEW_AFFECT_AUTOLOOT = 503, // PREMIUM_AUTOLOOT,
        NEW_AFFECT_FISH_MIND = 504, // PREMIUM_FISH_MIND,
        NEW_AFFECT_MARRIAGE_FAST = 505, // żřľÓŔÇ ±ęĹĐ (±Ý˝˝),
        NEW_AFFECT_GOLD_BONUS = 506,

        NEW_AFFECT_MALL = 510, // ¸ô ľĆŔĚĹŰ żˇĆĺĆ®
        NEW_AFFECT_NO_DEATH_PENALTY = 511, // żë˝ĹŔÇ °ˇČŁ (°ćÇčÄˇ ĆĐłÎĆĽ¸¦ ÇŃąř ¸·ľĆÁŘ´Ů)
        NEW_AFFECT_SKILL_BOOK_BONUS = 512, // Ľ±ŔÎŔÇ ±łČĆ (ĂĄ Ľö·Ă Ľş°ř Č®·üŔĚ 50% Áő°ˇ)
        NEW_AFFECT_SKILL_BOOK_NO_DELAY = 513, // ÁÖľČ ĽúĽ­ (ĂĄ Ľö·Ă µô·ąŔĚ ľřŔ˝)

        NEW_AFFECT_EXP_BONUS_EURO_FREE = 516, // °ćÇčŔÇ ąÝÁö (ŔŻ·´ ąöŔü 14 ·ąş§ ŔĚÇĎ ±âş» Čż°ú)
        NEW_AFFECT_EXP_BONUS_EURO_FREE_UNDER_15 = 517,

        NEW_AFFECT_AUTO_HP_RECOVERY = 534,        // ŔÚµżą°ľŕ HP
        NEW_AFFECT_AUTO_SP_RECOVERY = 535,        // ŔÚµżą°ľŕ SP

        NEW_AFFECT_DRAGON_SOUL_QUALIFIED = 540,
        NEW_AFFECT_DRAGON_SOUL_DECK1 = 541,
        NEW_AFFECT_DRAGON_SOUL_DECK2 = 542,

        NEW_AFFECT_RAMADAN_ABILITY = 300,
        NEW_AFFECT_RAMADAN_RING = 301,            // ¶ó¸¶´Ü ŔĚşĄĆ®żë ĆŻĽöľĆŔĚĹŰ ĂĘ˝Â´ŢŔÇ ąÝÁö Âřżë ŔŻą«

        NEW_AFFECT_NOG_POCKET_ABILITY = 302,

        NEW_AFFECT_QUEST_START_IDX = 1000,
    };

    enum
    {
        STONE_SMOKE1 = 0,    // 99%
        STONE_SMOKE2 = 1,    // 85%
        STONE_SMOKE3 = 2,    // 80%
        STONE_SMOKE4 = 3,    // 60%
        STONE_SMOKE5 = 4,    // 45%
        STONE_SMOKE6 = 5,    // 40%
        STONE_SMOKE7 = 6,    // 20%
        STONE_SMOKE8 = 7,    // 10%
        STONE_SMOKE_NUM = 4,
    };

    enum EBuildingAffect
    {
        BUILDING_CONSTRUCTION_SMALL = 0,
        BUILDING_CONSTRUCTION_LARGE = 1,
        BUILDING_UPGRADE = 2,
    };

    enum
    {
        WEAPON_DUALHAND,
        WEAPON_ONEHAND,
        WEAPON_TWOHAND,
        WEAPON_NUM,
    };

    enum
    {
        EMPIRE_NONE,
        EMPIRE_A,
        EMPIRE_B,
        EMPIRE_C,
        EMPIRE_NUM,
    };

    enum
    {
        NAMECOLOR_MOB,
        NAMECOLOR_NPC,
        NAMECOLOR_PC,
        NAMECOLOR_PC_END = NAMECOLOR_PC + EMPIRE_NUM,
        NAMECOLOR_NORMAL_MOB,
        NAMECOLOR_NORMAL_NPC,
        NAMECOLOR_NORMAL_PC,
        NAMECOLOR_NORMAL_PC_END = NAMECOLOR_NORMAL_PC + EMPIRE_NUM,
        NAMECOLOR_EMPIRE_MOB,
        NAMECOLOR_EMPIRE_NPC,
        NAMECOLOR_EMPIRE_PC,
        NAMECOLOR_EMPIRE_PC_END = NAMECOLOR_EMPIRE_PC + EMPIRE_NUM,
        NAMECOLOR_FUNC,
        NAMECOLOR_PK,
        NAMECOLOR_PVP,
        NAMECOLOR_PARTY,
        NAMECOLOR_WARP,
        NAMECOLOR_WAYPOINT,
        NAMECOLOR_EXTRA = NAMECOLOR_FUNC + 10,
        NAMECOLOR_NUM = NAMECOLOR_EXTRA + 10,
    };

    enum
    {
        ALIGNMENT_TYPE_WHITE,
        ALIGNMENT_TYPE_NORMAL,
        ALIGNMENT_TYPE_DARK,
    };

    enum
    {
        EMOTICON_EXCLAMATION = 1,
        EMOTICON_FISH = 11,
        EMOTICON_NUM = 128,

        TITLE_NUM = 9,
        TITLE_NONE = 4,
    };

    enum    //ľĆ·ˇ ąřČŁ°ˇ ąŮ˛î¸é registerEffect ÂĘµµ ąŮ˛Ůľî Áŕľß ÇŃ´Ů.
    {
        EFFECT_REFINED_NONE,

        EFFECT_SWORD_REFINED7,
        EFFECT_SWORD_REFINED8,
        EFFECT_SWORD_REFINED9,

        EFFECT_BOW_REFINED7,
        EFFECT_BOW_REFINED8,
        EFFECT_BOW_REFINED9,

        EFFECT_FANBELL_REFINED7,
        EFFECT_FANBELL_REFINED8,
        EFFECT_FANBELL_REFINED9,

        EFFECT_SMALLSWORD_REFINED7,
        EFFECT_SMALLSWORD_REFINED8,
        EFFECT_SMALLSWORD_REFINED9,

        EFFECT_SMALLSWORD_REFINED7_LEFT,
        EFFECT_SMALLSWORD_REFINED8_LEFT,
        EFFECT_SMALLSWORD_REFINED9_LEFT,

        EFFECT_BODYARMOR_REFINED7,
        EFFECT_BODYARMOR_REFINED8,
        EFFECT_BODYARMOR_REFINED9,

        EFFECT_BODYARMOR_SPECIAL,    // °©żĘ 4-2-1
        EFFECT_BODYARMOR_SPECIAL2,    // °©żĘ 4-2-2

        EFFECT_REFINED_NUM,
    };

    enum DamageFlag
    {
        DAMAGE_NORMAL = (1 << 0),
        DAMAGE_POISON = (1 << 1),
        DAMAGE_DODGE = (1 << 2),
        DAMAGE_BLOCK = (1 << 3),
        DAMAGE_PENETRATE = (1 << 4),
        DAMAGE_CRITICAL = (1 << 5),
        // ąÝ-_-»ç
    };

    enum
    {
        EFFECT_DUST,
        EFFECT_STUN,
        EFFECT_HIT,
        EFFECT_FLAME_ATTACK,
        EFFECT_FLAME_HIT,
        EFFECT_FLAME_ATTACH,
        EFFECT_ELECTRIC_ATTACK,
        EFFECT_ELECTRIC_HIT,
        EFFECT_ELECTRIC_ATTACH,
        EFFECT_SPAWN_APPEAR,
        EFFECT_SPAWN_DISAPPEAR,
        EFFECT_LEVELUP,
        EFFECT_SKILLUP,
        EFFECT_HPUP_RED,
        EFFECT_SPUP_BLUE,
        EFFECT_SPEEDUP_GREEN,
        EFFECT_DXUP_PURPLE,
        EFFECT_CRITICAL,
        EFFECT_PENETRATE,
        EFFECT_BLOCK,
        EFFECT_DODGE,
        EFFECT_FIRECRACKER,
        EFFECT_SPIN_TOP,
        EFFECT_WEAPON,
        EFFECT_WEAPON_END = EFFECT_WEAPON + WEAPON_NUM,
        EFFECT_AFFECT,
        EFFECT_AFFECT_GYEONGGONG = EFFECT_AFFECT + AFFECT_GYEONGGONG,
        EFFECT_AFFECT_KWAESOK = EFFECT_AFFECT + AFFECT_KWAESOK,
        EFFECT_AFFECT_END = EFFECT_AFFECT + AFFECT_NUM,
        EFFECT_EMOTICON,
        EFFECT_EMOTICON_END = EFFECT_EMOTICON + EMOTICON_NUM,
        EFFECT_SELECT,
        EFFECT_TARGET,
        EFFECT_EMPIRE,
        EFFECT_EMPIRE_END = EFFECT_EMPIRE + EMPIRE_NUM,
        EFFECT_HORSE_DUST,
        EFFECT_REFINED,
        EFFECT_REFINED_END = EFFECT_REFINED + EFFECT_REFINED_NUM,
        EFFECT_DAMAGE_TARGET,
        EFFECT_DAMAGE_NOT_TARGET,
        EFFECT_DAMAGE_SELFDAMAGE,
        EFFECT_DAMAGE_SELFDAMAGE2,
        EFFECT_DAMAGE_POISON,
        EFFECT_DAMAGE_MISS,
        EFFECT_DAMAGE_TARGETMISS,
        EFFECT_DAMAGE_CRITICAL,
        EFFECT_SUCCESS,
        EFFECT_FAIL,
        EFFECT_FR_SUCCESS,
        EFFECT_LEVELUP_ON_14_FOR_GERMANY,    //·ąş§ľ÷ 14ŔĎ¶§ ( µ¶ŔĎŔüżë )
        EFFECT_LEVELUP_UNDER_15_FOR_GERMANY,//·ąş§ľ÷ 15ŔĎ¶§ ( µ¶ŔĎŔüżë )
        EFFECT_PERCENT_DAMAGE1,
        EFFECT_PERCENT_DAMAGE2,
        EFFECT_PERCENT_DAMAGE3,
        EFFECT_AUTO_HPUP,
        EFFECT_AUTO_SPUP,
        EFFECT_RAMADAN_RING_EQUIP,            // ĂĘ˝Â´Ţ ąÝÁö Âřżë Ľř°Łżˇ ąßµżÇĎ´Â ŔĚĆĺĆ®
        EFFECT_HALLOWEEN_CANDY_EQUIP,        // ÇŇ·ÎŔ© »çĹÁ Âřżë Ľř°Łżˇ ąßµżÇĎ´Â ŔĚĆĺĆ®
        EFFECT_HAPPINESS_RING_EQUIP,                // ÇŕşąŔÇ ąÝÁö Âřżë Ľř°Łżˇ ąßµżÇĎ´Â ŔĚĆĺĆ®
        EFFECT_LOVE_PENDANT_EQUIP,                // ÇŕşąŔÇ ąÝÁö Âřżë Ľř°Łżˇ ąßµżÇĎ´Â ŔĚĆĺĆ®
        EFFECT_TEMP,
        EFFECT_NUM,
    };

    enum
    {
        DUEL_NONE,
        DUEL_CANNOTATTACK,
        DUEL_START,
    };


private:
    uint8_t padding_0x0[8];
public:
    static std::string ms_astAffectEffectAttachBone[EFFECT_NUM];
    static DWORD ms_adwCRCAffectEffect[EFFECT_NUM];
    static float ms_fDustGap;
    static float ms_fHorseDustGap;

    struct SHORSE
    {
        bool m_isMounting;
        CActorInstance* m_pkActor;
    } m_kHorse;

    BOOL m_isTextTail;

    // Instance Data
    std::string                m_stName;

    DWORD                    m_awPart[5];

    DWORD                    m_dwLevel;
    DWORD                    m_dwEmpireID;
    DWORD                    m_dwGuildID;

    CAffectFlagContainer    m_kAffectFlagContainer;
    DWORD                    m_adwCRCAffectEffect[AFFECT_NUM];

    struct SEffectContainer
    {
        typedef std::map<DWORD, DWORD> Dict;
        Dict m_kDct_dwEftID;
    } m_kEffectContainer;

    struct SStoneSmoke
    {
        DWORD m_dwEftID;
    } m_kStoneSmoke;


    // Emoticon
    //DWORD                    m_adwCRCEmoticonEffect[EMOTICON_NUM];

    BYTE                    m_eType;
    BYTE                    m_eRaceType;
    DWORD                    m_eShape;
    DWORD                    m_dwRace;
    DWORD                    m_dwVirtualNumber;
    short                    m_sAlignment;
    BYTE                    m_byPKMode;
    bool                    m_isKiller;
    bool                    m_isPartyMember;

    // Movement
    int                        m_iRotatingDirection;

    DWORD                    m_dwAdvActorVID;
    DWORD                    m_dwLastDmgActorVID;

    LONG                    m_nAverageNetworkGap;
    DWORD                    m_dwNextUpdateHeightTime;

    bool                    m_isGoing;

    TPixelPosition            m_kPPosDust;

    DWORD                    m_dwLastComboIndex;

    DWORD                    m_swordRefineEffectRight;
    DWORD                    m_swordRefineEffectLeft;
    DWORD                    m_armorRefineEffect;

    struct SMoveAfterFunc
    {
        UINT eFunc;
        UINT uArg;

        // For Emotion Function
        UINT uArgExpanded;
        TPixelPosition kPosDst;
    };

    SMoveAfterFunc m_kMovAfterFunc;

    float m_fDstRot;
    float m_fAtkPosTime;
    float m_fRotSpd;
    float m_fMaxRotSpd;

    BOOL m_bEnableTCPState;

    // Graphic Instance
    CActorInstance m_GraphicThingInstance;

    struct SCommand
    {
        DWORD    m_dwChkTime;
        DWORD    m_dwCmdTime;
        float    m_fDstRot;
        UINT     m_eFunc;
        UINT     m_uArg;
        UINT    m_uTargetVID;
        TPixelPosition m_kPPosDst;
    };

    typedef std::list<SCommand> CommandQueue;

    DWORD        m_dwBaseChkTime;
    DWORD        m_dwBaseCmdTime;

    DWORD        m_dwSkipTime;

    CommandQueue m_kQue_kCmdNew;

    BOOL        m_bDamageEffectType;

    struct SEffectDamage
    {
        DWORD damage;
        BYTE flag;
        BOOL bSelf;
        BOOL bTarget;
    };

    typedef std::list<SEffectDamage> CommandDamageQueue;
    CommandDamageQueue m_DamageQueue;

    struct SWarrior
    {
        DWORD m_dwGeomgyeongEffect;
    };

    SWarrior m_kWarrior;

    static CDynamicPool<CInstanceBase>    ms_kPool;

    static DWORD ms_dwUpdateCounter;
    static DWORD ms_dwRenderCounter;
    static DWORD ms_dwDeformCounter;

    DWORD                    m_dwDuelMode;
    DWORD                    m_dwEmoticonTime;
};

// int i = sizeof(CInstanceBase);

struct PyObject;

class CPythonPlayer
{
public:
    enum
    {
        CATEGORY_NONE = 0,
        CATEGORY_ACTIVE = 1,
        CATEGORY_PASSIVE = 2,
        CATEGORY_MAX_NUM = 3,

        STATUS_INDEX_ST = 1,
        STATUS_INDEX_DX = 2,
        STATUS_INDEX_IQ = 3,
        STATUS_INDEX_HT = 4,
    };

    enum
    {
        MBT_LEFT,
        MBT_RIGHT,
        MBT_MIDDLE,
        MBT_NUM,
    };

    enum
    {
        MBF_SMART,
        MBF_MOVE,
        MBF_CAMERA,
        MBF_ATTACK,
        MBF_SKILL,
        MBF_AUTO,
    };

    enum
    {
        MBS_CLICK,
        MBS_PRESS,
    };

    enum EMode
    {
        MODE_NONE,
        MODE_CLICK_POSITION,
        MODE_CLICK_ITEM,
        MODE_CLICK_ACTOR,
        MODE_USE_SKILL,
    };

    enum EEffect
    {
        EFFECT_PICK,
        EFFECT_NUM,
    };

    enum EMetinSocketType
    {
        METIN_SOCKET_TYPE_NONE,
        METIN_SOCKET_TYPE_SILVER,
        METIN_SOCKET_TYPE_GOLD,
    };

    typedef struct SSkillInstance
    {
        DWORD dwIndex;
        int iType;
        int iGrade;
        int iLevel;
        float fcurEfficientPercentage;
        float fnextEfficientPercentage;
        BOOL isCoolTime;

        float fCoolTime;            // NOTE : Ađ?¸?O Aß?I ˝??ł ˝˝·O?≫
        float fLastUsedTime;        //        Au?˘?ˇ μi·?C? ¶§ ≫c?eC?´A ???o
        BOOL bActive;
    } TSkillInstance;

    enum EKeyBoard_UD
    {
        KEYBOARD_UD_NONE,
        KEYBOARD_UD_UP,
        KEYBOARD_UD_DOWN,
    };

    enum EKeyBoard_LR
    {
        KEYBOARD_LR_NONE,
        KEYBOARD_LR_LEFT,
        KEYBOARD_LR_RIGHT,
    };

    enum
    {
        DIR_UP,
        DIR_DOWN,
        DIR_LEFT,
        DIR_RIGHT,
    };

    typedef struct SPlayerStatus
    {
        TItemData            aItem[180];
        TItemData            aDSItem[960];
        TQuickSlot            aQuickSlot[36];
        TSkillInstance        aSkill[255];
        long                m_alPoint[255];
        long                lQuickPageIndex;
    } TPlayerStatus;

    typedef struct SPartyMemberInfo
    {

        DWORD dwVID;
        DWORD dwPID;
        std::string strName;
        BYTE byState;
        BYTE byHPPercentage;
        short sAffects[7];
    } TPartyMemberInfo;

    enum EPartyRole
    {
        PARTY_ROLE_NORMAL,
        PARTY_ROLE_LEADER,
        PARTY_ROLE_ATTACKER,
        PARTY_ROLE_TANKER,
        PARTY_ROLE_BUFFER,
        PARTY_ROLE_SKILL_MASTER,
        PARTY_ROLE_BERSERKER,
        PARTY_ROLE_DEFENDER,
        PARTY_ROLE_MAX_NUM,
    };

    enum
    {
        SKILL_NORMAL,
        SKILL_MASTER,
        SKILL_GRAND_MASTER,
        SKILL_PERFECT_MASTER,
    };

    // ?Uμ??°?? ≫o?A °u·? ???­ ±¸A¶??.. ??·±˝A?C ???­ ?ł¸® ???÷?≫ ?? C?·A°i ?O´eC? łe·ACßAo¸¸ ˝C?đC?°i °a±? ???­?ł¸®.
    struct SAutoPotionInfo
    {

        bool bActivated;                    // ?°???­ μC?u´A°ˇ?            
        long currentAmount;                    // Co?c ł˛?? ?c
        long totalAmount;                    // ?u?? ?c
        long inventorySlotIndex;            // ≫c?eAß?I ???????C ?I???a¸®≫o ˝˝·O ?Iμ|˝?
    };

    enum EAutoPotionType
    {
        AUTO_POTION_TYPE_HP = 0,
        AUTO_POTION_TYPE_SP = 1,
        AUTO_POTION_TYPE_NUM
    };

private:
    uint8_t padding_0x0[8];

public:
    PyObject* m_ppyGameWindow;

    // Client Player Data
    std::map<DWORD, DWORD>    m_skillSlotDict;
    std::string                m_stName;

    DWORD                    m_dwMainCharacterIndex;
    DWORD                    m_dwRace;
    DWORD                    m_dwWeaponMinPower;
    DWORD                    m_dwWeaponMaxPower;
    DWORD                    m_dwWeaponMinMagicPower;
    DWORD                    m_dwWeaponMaxMagicPower;
    DWORD                    m_dwWeaponAddPower;

    // Todo
    DWORD                    m_dwSendingTargetVID;
    float                    m_fTargetUpdateTime;

    // Attack
    DWORD                    m_dwAutoAttackTargetVID;

    // NEW_Move
    EMode                    m_eReservedMode;
    float                    m_fReservedDelayTime;

    float                    m_fMovDirRot;

    bool                    m_isUp;
    bool                    m_isDown;
    bool                    m_isLeft;
    bool                    m_isRight;
    bool                    m_isAtkKey;
    bool                    m_isDirKey;
    bool                    m_isCmrRot;
    bool                    m_isSmtMov;
    bool                    m_isDirMov;

    float                    m_fCmrRotSpd;

    TPlayerStatus            m_playerStatus;
private:
    uint8_t padding_Daleko[25836];
public:
    UINT                    m_iComboOld;
    DWORD                    m_dwVIDReserved;
    DWORD                    m_dwIIDReserved;

    DWORD                    m_dwcurSkillSlotIndex;
    DWORD                    m_dwSkillSlotIndexReserved;
    DWORD                    m_dwSkillRangeReserved;

    TPixelPosition            m_kPPosInstPrev;
    TPixelPosition            m_kPPosReserved;

    // Emotion
    BOOL                    m_bisProcessingEmotion;

    // Dungeon
    BOOL                    m_isDestPosition;
    int                        m_ixDestPos;
    int                        m_iyDestPos;
    int                        m_iLastAlarmTime;

    // Party
    std::map<DWORD, TPartyMemberInfo>    m_PartyMemberMap;

    // PVP
    std::set<DWORD>            m_ChallengeInstanceSet;
    std::set<DWORD>            m_RevengeInstanceSet;
    std::set<DWORD>            m_CantFightInstanceSet;

    // Private Shop
    bool                    m_isOpenPrivateShop;
    bool                    m_isObserverMode;

    // Stamina
    BOOL                    m_isConsumingStamina;
    float                    m_fCurrentStamina;
    float                    m_fConsumeStaminaPerSec;

    // Guild
    DWORD                    m_inGuildAreaID;

    // Mobile
    BOOL                    m_bMobileFlag;

    // System
    BOOL                    m_sysIsCoolTime;
    BOOL                    m_sysIsLevelLimit;

    // Game Cursor Data
    TPixelPosition            m_MovingCursorPosition;
    float                    m_fMovingCursorSettingTime;
    DWORD                    m_adwEffect[EFFECT_NUM];

    DWORD                    m_dwVIDPicked;
    DWORD                    m_dwIIDPicked;
    int                        m_aeMBFButton[MBT_NUM];

    DWORD                    m_dwTargetVID;
    DWORD                    m_dwTargetEndTime;
    DWORD                    m_dwPlayTime;

    SAutoPotionInfo            m_kAutoPotionInfo[AUTO_POTION_TYPE_NUM];

    float                    MOVABLE_GROUND_DISTANCE;

    std::map<DWORD, DWORD> m_kMap_dwAffectIndexToSkillIndex;
};

class CPythonCharacterManager
{
private:
    uint8_t padding_0x0[8];
public:
    CInstanceBase* m_pkInstMain;
    CInstanceBase* m_pkInstPick;
    CInstanceBase* m_pkInstBind;

    D3DXVECTOR2    m_v2PickedInstProjPos;

    std::map<DWORD, CInstanceBase*> m_kAliveInstMap;
    std::list<CInstanceBase*> m_kDeadInstList;

    std::vector<CInstanceBase*>    m_kVct_pkInstPicked;
    DWORD m_adwPointEffect[255];

    decltype(m_kAliveInstMap)::iterator m_it;
};

typedef struct SAttachingData
{
    DWORD dwType;

    BOOL isAttaching;
    DWORD dwAttachingModelIndex;
    std::string strAttachingBoneName;

    void* pCollisionData;
    void* pEffectData;
    void* pObjectData;
} TAttachingData;

typedef std::vector<TAttachingData> TAttachingDataVector;

class CItemData
{
public:
    enum
    {
        ITEM_NAME_MAX_LEN = 24,
        ITEM_LIMIT_MAX_NUM = 2,
        ITEM_VALUES_MAX_NUM = 6,
        ITEM_SMALL_DESCR_MAX_LEN = 256,
        ITEM_APPLY_MAX_NUM = 3,
        ITEM_SOCKET_MAX_NUM = 3,
    };

    enum EItemType
    {
        ITEM_TYPE_NONE,                    //0
        ITEM_TYPE_WEAPON,                //1//?≪±a
        ITEM_TYPE_ARMOR,                //2//°ⓒ??
        ITEM_TYPE_USE,                    //3//?????? ≫c?e
        ITEM_TYPE_AUTOUSE,                //4
        ITEM_TYPE_MATERIAL,                //5
        ITEM_TYPE_SPECIAL,                //6 //˝??a?? ??????
        ITEM_TYPE_TOOL,                    //7
        ITEM_TYPE_LOTTERY,                //8//??±C
        ITEM_TYPE_ELK,                    //9//μ·
        ITEM_TYPE_METIN,                //10
        ITEM_TYPE_CONTAINER,            //11
        ITEM_TYPE_FISH,                    //12//ł￢˝?
        ITEM_TYPE_ROD,                    //13
        ITEM_TYPE_RESOURCE,                //14
        ITEM_TYPE_CAMPFIRE,                //15
        ITEM_TYPE_UNIQUE,                //16
        ITEM_TYPE_SKILLBOOK,            //17
        ITEM_TYPE_QUEST,                //18
        ITEM_TYPE_POLYMORPH,            //19
        ITEM_TYPE_TREASURE_BOX,            //20//?¸?°≫o?U
        ITEM_TYPE_TREASURE_KEY,            //21//?¸?°≫o?U ?­??
        ITEM_TYPE_SKILLFORGET,            //22
        ITEM_TYPE_GIFTBOX,                //23
        ITEM_TYPE_PICK,                    //24
        ITEM_TYPE_HAIR,                    //25//¸O¸®
        ITEM_TYPE_TOTEM,                //26//?a??
        ITEM_TYPE_BLEND,                //27//≫y??μE¶§ ·Ł´yC?°O ?O???? ??´A ???°
        ITEM_TYPE_DS,                    //29 //?e???®
        ITEM_TYPE_EXTRACT,                    //31 ?ß?aμμ±¸.
        ITEM_TYPE_SECONDARY_COIN,            //32 ¸iμμ?u.
        ITEM_TYPE_BELT,                        //34 ?§?®

        ITEM_TYPE_MAX_NUM,
    };

    enum EWeaponSubTypes
    {
        WEAPON_SWORD,
        WEAPON_DAGGER,    //??μμ·?
        WEAPON_BOW,
        WEAPON_TWO_HANDED,
        WEAPON_BELL,
        WEAPON_FAN,
        WEAPON_ARROW,
        WEAPON_NUM_TYPES,

        WEAPON_NONE = WEAPON_NUM_TYPES + 1,
    };

    enum EMaterialSubTypes
    {
        MATERIAL_LEATHER,
        MATERIAL_BLOOD,
        MATERIAL_ROOT,
        MATERIAL_NEEDLE,
        MATERIAL_JEWEL,
        MATERIAL_DS_REFINE_NORMAL,
        MATERIAL_DS_REFINE_BLESSED,
        MATERIAL_DS_REFINE_HOLLY,
    };

    enum EArmorSubTypes
    {
        ARMOR_BODY,
        ARMOR_HEAD,
        ARMOR_SHIELD,
        ARMOR_WRIST,
        ARMOR_FOOTS,
        ARMOR_NECK,
        ARMOR_EAR,
        ARMOR_NUM_TYPES
    };

    enum ECostumeSubTypes
    {
        COSTUME_NUM_TYPES,
    };

    enum EUseSubTypes
    {
        USE_POTION,                    // 0
        USE_TALISMAN,
        USE_TUNING,
        USE_MOVE,
        USE_TREASURE_BOX,
        USE_MONEYBAG,
        USE_BAIT,
        USE_ABILITY_UP,
        USE_AFFECT,
        USE_CREATE_STONE,
        USE_SPECIAL,                // 10
        USE_POTION_NODELAY,
        USE_CLEAR,
        USE_INVISIBILITY,
        USE_DETACHMENT,
        USE_BUCKET,
        USE_POTION_CONTINUE,
        USE_CLEAN_SOCKET,
        USE_CHANGE_ATTRIBUTE,
        USE_ADD_ATTRIBUTE,
        USE_ADD_ACCESSORY_SOCKET,    // 20
        USE_PUT_INTO_ACCESSORY_SOCKET,
        USE_ADD_ATTRIBUTE2,
        USE_RECIPE,
        USE_CHANGE_ATTRIBUTE2,
        USE_BIND,
        USE_UNBIND,
        USE_TIME_CHARGE_PER,
        USE_TIME_CHARGE_FIX,                // 28
        USE_PUT_INTO_BELT_SOCKET,            // 29 ?§?® ??A??ˇ ≫c?eC? ?o ?O´A ?????? 
    };

    enum EDragonSoulSubType
    {
        DS_SLOT1,
        DS_SLOT2,
        DS_SLOT3,
        DS_SLOT4,
        DS_SLOT5,
        DS_SLOT6,
        DS_SLOT_NUM_TYPES = 6,
    };

    enum EMetinSubTypes
    {
        METIN_NORMAL,
        METIN_GOLD,
    };

    enum ELimitTypes
    {
        LIMIT_NONE,

        LIMIT_LEVEL,
        LIMIT_STR,
        LIMIT_DEX,
        LIMIT_INT,
        LIMIT_CON,
        LIMIT_PCBANG,

        LIMIT_REAL_TIME,

        /// ?????? ≫c?e˝? socket1?ˇ ≫c?e ?˝?o°ˇ ?U?÷°i socket0?ˇ unix_timestamp ?¸?O?C ??¸?˝?°Ł?? ?U??.
        LIMIT_REAL_TIME_START_FIRST_USE,

        /// ???????≫ A??e Aß?? ¶§¸¸ ≫c?e ˝?°Ł?? A÷°¨μC´A ??????
        LIMIT_TIMER_BASED_ON_WEAR,

        LIMIT_MAX_NUM
    };

    enum EItemAntiFlag
    {
    };

    enum EItemFlag
    {
    };

    enum EWearPositions
    {
        WEAR_BODY,          // 0
        WEAR_HEAD,          // 1
        WEAR_FOOTS,         // 2
        WEAR_WRIST,         // 3
        WEAR_WEAPON,        // 4
        WEAR_NECK,          // 5
        WEAR_EAR,           // 6
        WEAR_UNIQUE1,       // 7
        WEAR_UNIQUE2,       // 8
        WEAR_ARROW,         // 9
        WEAR_SHIELD,        // 10
        WEAR_MAX_NUM,
    };

    enum EItemWearableFlag
    {
    };

    enum EApplyTypes
    {
        APPLY_NONE,                 // 0
        APPLY_MAX_HP,               // 1
        APPLY_MAX_SP,               // 2
        APPLY_CON,                  // 3
        APPLY_INT,                  // 4
        APPLY_STR,                  // 5
        APPLY_DEX,                  // 6
        APPLY_ATT_SPEED,            // 7
        APPLY_MOV_SPEED,            // 8
        APPLY_CAST_SPEED,           // 9
        APPLY_HP_REGEN,             // 10
        APPLY_SP_REGEN,             // 11
        APPLY_POISON_PCT,           // 12
        APPLY_STUN_PCT,             // 13
        APPLY_SLOW_PCT,             // 14
        APPLY_CRITICAL_PCT,         // 15
        APPLY_PENETRATE_PCT,        // 16
        APPLY_ATTBONUS_HUMAN,       // 17
        APPLY_ATTBONUS_ANIMAL,      // 18
        APPLY_ATTBONUS_ORC,         // 19
        APPLY_ATTBONUS_MILGYO,      // 20
        APPLY_ATTBONUS_UNDEAD,      // 21
        APPLY_ATTBONUS_DEVIL,       // 22
        APPLY_STEAL_HP,             // 23
        APPLY_STEAL_SP,             // 24
        APPLY_MANA_BURN_PCT,        // 25
        APPLY_DAMAGE_SP_RECOVER,    // 26
        APPLY_BLOCK,                // 27
        APPLY_DODGE,                // 28
        APPLY_RESIST_SWORD,         // 29
        APPLY_RESIST_TWOHAND,       // 30
        APPLY_RESIST_DAGGER,        // 31
        APPLY_RESIST_BELL,          // 32
        APPLY_RESIST_FAN,           // 33
        APPLY_RESIST_BOW,           // 34
        APPLY_RESIST_FIRE,          // 35
        APPLY_RESIST_ELEC,          // 36
        APPLY_RESIST_MAGIC,         // 37
        APPLY_RESIST_WIND,          // 38
        APPLY_REFLECT_MELEE,        // 39
        APPLY_REFLECT_CURSE,        // 40
        APPLY_POISON_REDUCE,        // 41
        APPLY_KILL_SP_RECOVER,      // 42
        APPLY_EXP_DOUBLE_BONUS,     // 43
        APPLY_GOLD_DOUBLE_BONUS,    // 44
        APPLY_ITEM_DROP_BONUS,      // 45
        APPLY_POTION_BONUS,         // 46
        APPLY_KILL_HP_RECOVER,      // 47
        APPLY_IMMUNE_STUN,          // 48
        APPLY_IMMUNE_SLOW,          // 49
        APPLY_IMMUNE_FALL,          // 50
        APPLY_SKILL,                // 51
        APPLY_BOW_DISTANCE,         // 52
        APPLY_ATT_GRADE_BONUS,            // 53
        APPLY_DEF_GRADE_BONUS,            // 54
        APPLY_MAGIC_ATT_GRADE,      // 55
        APPLY_MAGIC_DEF_GRADE,      // 56
        APPLY_CURSE_PCT,            // 57
        APPLY_MAX_STAMINA,            // 58
        APPLY_ATT_BONUS_TO_WARRIOR, // 59
        APPLY_ATT_BONUS_TO_ASSASSIN,// 60
        APPLY_ATT_BONUS_TO_SURA,    // 61
        APPLY_ATT_BONUS_TO_SHAMAN,  // 62
        APPLY_ATT_BONUS_TO_MONSTER, // 63
        APPLY_MALL_ATTBONUS,        // 64 °?°Y·A +x%
        APPLY_MALL_DEFBONUS,        // 65 ???i·A +x%
        APPLY_MALL_EXPBONUS,        // 66 °?C?Aˇ +x%
        APPLY_MALL_ITEMBONUS,       // 67 ?????? μ?·O?˛ x/10??
        APPLY_MALL_GOLDBONUS,       // 68 μ· μ?·O?˛ x/10??
        APPLY_MAX_HP_PCT,           // 69 ?O´e ≫y¸i·A +x%
        APPLY_MAX_SP_PCT,           // 70 ?O´e A¤˝?·A +x%
        APPLY_EXTRACT_HP_PCT,        //75
        APPLY_PC_BANG_EXP_BONUS,        //76
        APPLY_PC_BANG_DROP_BONUS,        //77
        APPLY_RESIST_WARRIOR,            //78
        APPLY_RESIST_ASSASSIN,            //79
        APPLY_RESIST_SURA,                //80
        APPLY_RESIST_SHAMAN,            //81
        APPLY_ENERGY,                    //82
        APPLY_COSTUME_ATTR_BONUS,        // 84 AU˝??￢ ???????ˇ ???? ?O??Aˇ ?¸ł?˝?
        APPLY_MAGIC_ATTBONUS_PER,        // 85 ¸¶?y °?°Y·A +x%
        APPLY_MELEE_MAGIC_ATTBONUS_PER,            // 86 ¸¶?y + ?đ¸® °?°Y·A +x%

        APPLY_RESIST_ICE,        // 87 ł?±a ?uC×
        APPLY_RESIST_EARTH,        // 88 ´eAo ?uC×
        APPLY_RESIST_DARK,        // 89 ?iμ? ?uC×

        APPLY_ANTI_CRITICAL_PCT,    //90 ?ⓒ¸®??A? ?uC×
        APPLY_ANTI_PENETRATE_PCT,    //91 °u?e?¸°Y ?uC×

        MAX_APPLY_NUM,              // 
    };

    enum EImmuneFlags
    {
    };

    typedef struct SItemLimit
    {
        BYTE        bType;
        long        lValue;
    } TItemLimit;

    typedef struct SItemApply
    {
        BYTE        bType;
        long        lValue;
    } TItemApply;

    typedef struct SItemTable
    {
        DWORD       dwVnum;
        DWORD       dwVnumRange;
        char        szName[ITEM_NAME_MAX_LEN + 1];
        char        szLocaleName[ITEM_NAME_MAX_LEN + 1];
        BYTE        bType;
        BYTE        bSubType;

        BYTE        bWeight;
        BYTE        bSize;

        DWORD       dwAntiFlags;
        DWORD       dwFlags;
        DWORD       dwWearFlags;
        DWORD       dwImmuneFlag;

        DWORD       dwIBuyItemPrice;
        DWORD        dwISellItemPrice;

        TItemLimit  aLimits[ITEM_LIMIT_MAX_NUM];
        TItemApply  aApplies[ITEM_APPLY_MAX_NUM];
        long        alValues[ITEM_VALUES_MAX_NUM];
        long        alSockets[ITEM_SOCKET_MAX_NUM];
        DWORD       dwRefinedVnum;
        WORD        wRefineSet;
        BYTE        bAlterToMagicItemPct;
        BYTE        bSpecular;
        BYTE        bGainSocketPct;
    } TItemTable;

public:
    std::string m_strModelFileName;
    std::string m_strSubModelFileName;
    std::string m_strDropModelFileName;
    std::string m_strIconFileName;
    std::string m_strDescription;
    std::string m_strSummary;
    std::vector<std::string> m_strLODModelFileNameVector;

    void* m_pModelThing;
    void* m_pSubModelThing;
    void* m_pDropModelThing;
    void* m_pIconImage;
    std::vector<void*> m_pLODModelThingVector;

    TAttachingDataVector m_AttachingDataVector;
    DWORD        m_dwVnum;
    TItemTable m_ItemTable;

    static CDynamicPool<CItemData>        ms_kPool;
};

class CPythonItem
{
public:
    enum
    {
        INVALID_ID = 0xffffffff,
    };

    enum
    {
        VNUM_MONEY = 1,
    };

    enum
    {
        USESOUND_NONE,
        USESOUND_DEFAULT,
        USESOUND_ARMOR,
        USESOUND_WEAPON,
        USESOUND_BOW,
        USESOUND_ACCESSORY,
        USESOUND_POTION,
        USESOUND_PORTAL,
        USESOUND_NUM,
    };

    enum
    {
        DROPSOUND_DEFAULT,
        DROPSOUND_ARMOR,
        DROPSOUND_WEAPON,
        DROPSOUND_BOW,
        DROPSOUND_ACCESSORY,
        DROPSOUND_NUM
    };

    typedef struct SGroundItemInstance
    {
    private:
        uint8_t padding_0x0[4];
    public:
        DWORD                    dwVirtualNumber;
        D3DXVECTOR3                v3EndPosition;

        D3DXVECTOR3                v3RotationAxis;
        D3DXQUATERNION            qEnd;
        D3DXVECTOR3                v3Center;
        CGraphicThingInstance    ThingInstance;
        uint8_t padding_0x1[4];
        DWORD                    dwStartTime;
        DWORD                    dwEndTime;

        DWORD                    eDropSoundType;

        bool                    bAnimEnded;

        DWORD                    dwEffectInstanceIndex;
        std::string                stOwnership;

        static std::string        ms_astDropSoundFileName[DROPSOUND_NUM];

    } TGroundItemInstance;

    typedef std::map<DWORD, TGroundItemInstance*>    TGroundItemInstanceMap;

private:
    // no padding here
    uint8_t padding[4];
public:

    TGroundItemInstanceMap                m_GroundItemInstanceMap;
    CDynamicPool<TGroundItemInstance>    m_GroundItemInstancePool;

    DWORD m_dwDropItemEffectID;
    DWORD m_dwPickedItemID;

    int m_nMouseX;
    int m_nMouseY;

    std::string m_astUseSoundFileName[USESOUND_NUM];

    std::vector<CItemData*> m_NoGradeNameItemData;
};

inline CurrentAction GetCurrentAction()
{
    return GetObjectPointer<CPythonCharacterManager>()->m_pkInstMain->m_GraphicThingInstance.m_kCurMotNode.dwMotionKey;
}

constexpr int offsetStatus = (int)offsetof(CPythonPlayer, m_playerStatus);
constexpr int offsetCombo = (int)offsetof(CPythonPlayer, m_iComboOld);

constexpr int size = 90824 - offsetCombo;

static_assert(((int)offsetof(CPythonPlayer, m_iComboOld)) == 90824);

constexpr int WeaponOffset = offsetof(CPythonPlayer::TPlayerStatus, aDSItem[4]);