#ifndef ProtocolH
#define ProtocolH

// Types from eo-protocol (xml/net/protocol.xml, xml/net/server/protocol.xml and
// xml/game/protocol.xml). Values are verbatim from the specification; constants
// are prefixed with the enum name because C++Builder 5 has no scoped enums.
enum MapType
{
    MapType_Normal = 0,
    MapType_Pk = 3
};

enum MapTimedEffect
{
    MapTimedEffect_None = 0,
    MapTimedEffect_HpDrain = 1,
    MapTimedEffect_TpDrain = 2,
    MapTimedEffect_Quake1 = 3,
    MapTimedEffect_Quake2 = 4,
    MapTimedEffect_Quake3 = 5,
    MapTimedEffect_Quake4 = 6
};

enum MapMusicControl
{
    MapMusicControl_InterruptIfDifferentPlayOnce = 0,
    MapMusicControl_InterruptPlayOnce = 1,
    MapMusicControl_FinishPlayOnce = 2,
    MapMusicControl_InterruptIfDifferentPlayRepeat = 3,
    MapMusicControl_InterruptPlayRepeat = 4,
    MapMusicControl_FinishPlayRepeat = 5,
    MapMusicControl_InterruptPlayNothing = 6
};

enum MapTileSpec
{
    MapTileSpec_Wall = 0,
    MapTileSpec_ChairDown = 1,
    MapTileSpec_ChairLeft = 2,
    MapTileSpec_ChairRight = 3,
    MapTileSpec_ChairUp = 4,
    MapTileSpec_ChairDownRight = 5,
    MapTileSpec_ChairUpLeft = 6,
    MapTileSpec_ChairAll = 7,
    MapTileSpec_Reserved8 = 8,
    MapTileSpec_Chest = 9,
    MapTileSpec_Reserved10 = 10,
    MapTileSpec_Reserved11 = 11,
    MapTileSpec_Reserved12 = 12,
    MapTileSpec_Reserved13 = 13,
    MapTileSpec_Reserved14 = 14,
    MapTileSpec_Reserved15 = 15,
    MapTileSpec_BankVault = 16,
    MapTileSpec_NpcBoundary = 17,
    MapTileSpec_Edge = 18,
    MapTileSpec_FakeWall = 19,
    MapTileSpec_Board1 = 20,
    MapTileSpec_Board2 = 21,
    MapTileSpec_Board3 = 22,
    MapTileSpec_Board4 = 23,
    MapTileSpec_Board5 = 24,
    MapTileSpec_Board6 = 25,
    MapTileSpec_Board7 = 26,
    MapTileSpec_Board8 = 27,
    MapTileSpec_Jukebox = 28,
    MapTileSpec_Jump = 29,
    MapTileSpec_Water = 30,
    MapTileSpec_Reserved31 = 31,
    MapTileSpec_Arena = 32,
    MapTileSpec_AmbientSource = 33,
    MapTileSpec_TimedSpikes = 34,
    MapTileSpec_Spikes = 35,
    MapTileSpec_HiddenSpikes = 36
};

enum SitAction
{
    SitAction_Sit = 1,
    SitAction_Stand = 2
};

enum GuildInfoType
{
    GuildInfoType_Description = 1,
    GuildInfoType_Ranks = 2,
    GuildInfoType_Bank = 3
};

enum TrainType
{
    TrainType_Stat = 1,
    TrainType_Skill = 2
};

enum DialogReply
{
    DialogReply_Ok = 1,
    DialogReply_Link = 2
};

enum FileType
{
    FileType_Emf = 1,
    FileType_Eif = 2,
    FileType_Enf = 3,
    FileType_Esf = 4,
    FileType_Ecf = 5
};

enum StatId
{
    StatId_Str = 1,
    StatId_Int = 2,
    StatId_Wis = 3,
    StatId_Agi = 4,
    StatId_Con = 5,
    StatId_Cha = 6
};

enum SpellTargetType
{
    SpellTargetType_Player = 1,
    SpellTargetType_Npc = 2
};

enum MarriageRequestType
{
    MarriageRequestType_MarriageApproval = 1,
    MarriageRequestType_Divorce = 2
};

enum PacketFamily
{
    PacketFamily_Connection = 1,
    PacketFamily_Account = 2,
    PacketFamily_Character = 3,
    PacketFamily_Login = 4,
    PacketFamily_Welcome = 5,
    PacketFamily_Walk = 6,
    PacketFamily_Face = 7,
    PacketFamily_Chair = 8,
    PacketFamily_Emote = 9,
    PacketFamily_Attack = 11,
    PacketFamily_Spell = 12,
    PacketFamily_Shop = 13,
    PacketFamily_Item = 14,
    PacketFamily_StatSkill = 16,
    PacketFamily_Global = 17,
    PacketFamily_Talk = 18,
    PacketFamily_Warp = 19,
    PacketFamily_Jukebox = 21,
    PacketFamily_Players = 22,
    PacketFamily_Avatar = 23,
    PacketFamily_Party = 24,
    PacketFamily_Refresh = 25,
    PacketFamily_Npc = 26,
    PacketFamily_PlayerRange = 27,
    PacketFamily_NpcRange = 28,
    PacketFamily_Range = 29,
    PacketFamily_Paperdoll = 30,
    PacketFamily_Effect = 31,
    PacketFamily_Trade = 32,
    PacketFamily_Chest = 33,
    PacketFamily_Door = 34,
    PacketFamily_Message = 35,
    PacketFamily_Bank = 36,
    PacketFamily_Locker = 37,
    PacketFamily_Barber = 38,
    PacketFamily_Guild = 39,
    PacketFamily_Music = 40,
    PacketFamily_Sit = 41,
    PacketFamily_Recover = 42,
    PacketFamily_Board = 43,
    PacketFamily_Cast = 44,
    PacketFamily_Arena = 45,
    PacketFamily_Priest = 46,
    PacketFamily_Marriage = 47,
    PacketFamily_AdminInteract = 48,
    PacketFamily_Citizen = 49,
    PacketFamily_Quest = 50,
    PacketFamily_Book = 51,
    PacketFamily_Error = 250,
    PacketFamily_Init = 255
};

enum PacketAction
{
    PacketAction_Request = 1,
    PacketAction_Accept = 2,
    PacketAction_Reply = 3,
    PacketAction_Remove = 4,
    PacketAction_Agree = 5,
    PacketAction_Create = 6,
    PacketAction_Add = 7,
    PacketAction_Player = 8,
    PacketAction_Take = 9,
    PacketAction_Use = 10,
    PacketAction_Buy = 11,
    PacketAction_Sell = 12,
    PacketAction_Open = 13,
    PacketAction_Close = 14,
    PacketAction_Msg = 15,
    PacketAction_Spec = 16,
    PacketAction_Admin = 17,
    PacketAction_List = 18,
    PacketAction_Tell = 20,
    PacketAction_Report = 21,
    PacketAction_Announce = 22,
    PacketAction_Server = 23,
    PacketAction_Drop = 24,
    PacketAction_Junk = 25,
    PacketAction_Obtain = 26,
    PacketAction_Get = 27,
    PacketAction_Kick = 28,
    PacketAction_Rank = 29,
    PacketAction_TargetSelf = 30,
    PacketAction_TargetOther = 31,
    PacketAction_TargetGroup = 33,
    PacketAction_Dialog = 34,
    PacketAction_Ping = 240,
    PacketAction_Pong = 241,
    PacketAction_Net242 = 242,
    PacketAction_Net243 = 243,
    PacketAction_Net244 = 244,
    PacketAction_Error = 250,
    PacketAction_Init = 255
};

enum QuestPage
{
    QuestPage_Progress = 1,
    QuestPage_History = 2
};

enum PartyRequestType
{
    PartyRequestType_Join = 0,
    PartyRequestType_Invite = 1
};

enum InitReply
{
    InitReply_OutOfDate = 1,
    InitReply_Ok = 2,
    InitReply_Banned = 3,
    InitReply_WarpMap = 4,
    InitReply_FileEmf = 5,
    InitReply_FileEif = 6,
    InitReply_FileEnf = 7,
    InitReply_FileEsf = 8,
    InitReply_PlayersList = 9,
    InitReply_MapMutation = 10,
    InitReply_PlayersListFriends = 11,
    InitReply_FileEcf = 12
};

enum InitBanType
{
    InitBanType_Temporary = 1,
    InitBanType_Permanent = 2
};

enum CharacterIcon
{
    CharacterIcon_Player = 1,
    CharacterIcon_Gm = 4,
    CharacterIcon_Hgm = 5,
    CharacterIcon_Party = 6,
    CharacterIcon_GmParty = 9,
    CharacterIcon_HgmParty = 10
};

enum AvatarChangeType
{
    AvatarChangeType_Equipment = 1,
    AvatarChangeType_Hair = 2,
    AvatarChangeType_HairColor = 3
};

enum TalkReply
{
    TalkReply_NotFound = 1
};

enum SitState
{
    SitState_Stand = 0,
    SitState_Chair = 1,
    SitState_Floor = 2
};

enum MapEffect
{
    MapEffect_Quake = 1
};

enum GuildReply
{
    GuildReply_Busy = 1,
    GuildReply_NotApproved = 2,
    GuildReply_AlreadyMember = 3,
    GuildReply_NoCandidates = 4,
    GuildReply_Exists = 5,
    GuildReply_CreateBegin = 6,
    GuildReply_CreateAddConfirm = 7,
    GuildReply_CreateAdd = 8,
    GuildReply_RecruiterOffline = 9,
    GuildReply_RecruiterNotHere = 10,
    GuildReply_RecruiterWrongGuild = 11,
    GuildReply_NotRecruiter = 12,
    GuildReply_JoinRequest = 13,
    GuildReply_NotPresent = 14,
    GuildReply_AccountLow = 15,
    GuildReply_Accepted = 16,
    GuildReply_NotFound = 17,
    GuildReply_Updated = 18,
    GuildReply_RanksUpdated = 19,
    GuildReply_RemoveLeader = 20,
    GuildReply_RemoveNotMember = 21,
    GuildReply_Removed = 22,
    GuildReply_RankingLeader = 23,
    GuildReply_RankingNotMember = 24
};

enum InnUnsubscribeReply
{
    InnUnsubscribeReply_NotCitizen = 0,
    InnUnsubscribeReply_Unsubscribed = 1
};

enum CharacterReply
{
    CharacterReply_Exists = 1,
    CharacterReply_Full = 2,
    CharacterReply_Full3 = 3,
    CharacterReply_NotApproved = 4,
    CharacterReply_Ok = 5,
    CharacterReply_Deleted = 6
};

enum SkillMasterReply
{
    SkillMasterReply_RemoveItems = 1,
    SkillMasterReply_WrongClass = 2
};

enum AccountReply
{
    AccountReply_Exists = 1,
    AccountReply_NotApproved = 2,
    AccountReply_Created = 3,
    AccountReply_ChangeFailed = 5,
    AccountReply_Changed = 6,
    AccountReply_RequestDenied = 7
};

enum LoginReply
{
    LoginReply_WrongUser = 1,
    LoginReply_WrongUserPassword = 2,
    LoginReply_Ok = 3,
    LoginReply_Banned = 4,
    LoginReply_LoggedIn = 5,
    LoginReply_Busy = 6
};

enum DialogEntryType
{
    DialogEntryType_Text = 1,
    DialogEntryType_Link = 2
};

enum QuestRequirementIcon
{
    QuestRequirementIcon_Item = 3,
    QuestRequirementIcon_Talk = 5,
    QuestRequirementIcon_Kill = 8,
    QuestRequirementIcon_Step = 10
};

enum WarpEffect
{
    WarpEffect_None = 0,
    WarpEffect_Scroll = 1,
    WarpEffect_Admin = 2
};

enum WarpType
{
    WarpType_Local = 1,
    WarpType_MapSwitch = 2
};

enum WelcomeCode
{
    WelcomeCode_SelectCharacter = 1,
    WelcomeCode_EnterGame = 2,
    WelcomeCode_ServerBusy = 3,
    WelcomeCode_LoggedIn = 4
};

enum LoginMessageCode
{
    LoginMessageCode_No = 0,
    LoginMessageCode_Yes = 250
};

enum AdminMessageType
{
    AdminMessageType_Message = 1,
    AdminMessageType_Report = 2
};

enum PlayerKilledState
{
    PlayerKilledState_Alive = 1,
    PlayerKilledState_Killed = 2
};

enum NpcKillStealProtectionState
{
    NpcKillStealProtectionState_Unprotected = 1,
    NpcKillStealProtectionState_Protected = 2
};

enum MapDamageType
{
    MapDamageType_TpDrain = 1,
    MapDamageType_Spikes = 2
};

enum MarriageReply
{
    MarriageReply_AlreadyMarried = 1,
    MarriageReply_NotMarried = 2,
    MarriageReply_Success = 3,
    MarriageReply_NotEnoughGold = 4,
    MarriageReply_WrongName = 5,
    MarriageReply_ServiceBusy = 6,
    MarriageReply_DivorceNotification = 7
};

enum PriestReply
{
    PriestReply_NotDressed = 1,
    PriestReply_LowLevel = 2,
    PriestReply_PartnerNotPresent = 3,
    PriestReply_PartnerNotDressed = 4,
    PriestReply_Busy = 5,
    PriestReply_DoYou = 6,
    PriestReply_PartnerAlreadyMarried = 7,
    PriestReply_NoPermission = 8
};

enum PartyReplyCode
{
    PartyReplyCode_AlreadyInAnotherParty = 0,
    PartyReplyCode_AlreadyInYourParty = 1,
    PartyReplyCode_PartyIsFull = 2
};

enum AdminLevel
{
    AdminLevel_Player = 0,
    AdminLevel_Spy = 1,
    AdminLevel_LightGuide = 2,
    AdminLevel_Guardian = 3,
    AdminLevel_GameMaster = 4,
    AdminLevel_HighGameMaster = 5
};

enum Direction
{
    Direction_Down = 0,
    Direction_Left = 1,
    Direction_Up = 2,
    Direction_Right = 3
};

enum Emote
{
    Emote_Happy = 1,
    Emote_Depressed = 2,
    Emote_Sad = 3,
    Emote_Angry = 4,
    Emote_Confused = 5,
    Emote_Surprised = 6,
    Emote_Hearts = 7,
    Emote_Moon = 8,
    Emote_Suicidal = 9,
    Emote_Embarrassed = 10,
    Emote_Drunk = 11,
    Emote_Trade = 12,
    Emote_LevelUp = 13,
    Emote_Playful = 14,
    Emote_Bard = 15
};

enum Gender
{
    Gender_Female = 0,
    Gender_Male = 1
};

enum Element
{
    Element_None = 0,
    Element_Light = 1,
    Element_Dark = 2,
    Element_Earth = 3,
    Element_Wind = 4,
    Element_Water = 5,
    Element_Fire = 6
};

enum ItemType
{
    ItemType_General = 0,
    ItemType_Reserved1 = 1,
    ItemType_Currency = 2,
    ItemType_Heal = 3,
    ItemType_Teleport = 4,
    ItemType_Reserved5 = 5,
    ItemType_ExpReward = 6,
    ItemType_Reserved7 = 7,
    ItemType_Reserved8 = 8,
    ItemType_Key = 9,
    ItemType_Weapon = 10,
    ItemType_Shield = 11,
    ItemType_Armor = 12,
    ItemType_Hat = 13,
    ItemType_Boots = 14,
    ItemType_Gloves = 15,
    ItemType_Accessory = 16,
    ItemType_Belt = 17,
    ItemType_Necklace = 18,
    ItemType_Ring = 19,
    ItemType_Armlet = 20,
    ItemType_Bracer = 21,
    ItemType_Alcohol = 22,
    ItemType_EffectPotion = 23,
    ItemType_HairDye = 24,
    ItemType_CureCurse = 25,
    ItemType_Reserved26 = 26,
    ItemType_Reserved27 = 27,
    ItemType_Reserved28 = 28,
    ItemType_Reserved29 = 29
};

enum ItemSubtype
{
    ItemSubtype_None = 0,
    ItemSubtype_Ranged = 1,
    ItemSubtype_Arrows = 2,
    ItemSubtype_Wings = 3,
    ItemSubtype_Reserved4 = 4
};

enum ItemSpecial
{
    ItemSpecial_Normal = 0,
    ItemSpecial_Rare = 1,
    ItemSpecial_Legendary = 2,
    ItemSpecial_Unique = 3,
    ItemSpecial_Lore = 4,
    ItemSpecial_Cursed = 5
};

enum ItemSize
{
    ItemSize_Size1x1 = 0,
    ItemSize_Size1x2 = 1,
    ItemSize_Size1x3 = 2,
    ItemSize_Size1x4 = 3,
    ItemSize_Size2x1 = 4,
    ItemSize_Size2x2 = 5,
    ItemSize_Size2x3 = 6,
    ItemSize_Size2x4 = 7
};

enum NpcType
{
    NpcType_Friendly = 0,
    NpcType_Passive = 1,
    NpcType_Aggressive = 2,
    NpcType_Reserved3 = 3,
    NpcType_Reserved4 = 4,
    NpcType_Reserved5 = 5,
    NpcType_Shop = 6,
    NpcType_Inn = 7,
    NpcType_Reserved8 = 8,
    NpcType_Bank = 9,
    NpcType_Barber = 10,
    NpcType_Guild = 11,
    NpcType_Priest = 12,
    NpcType_Lawyer = 13,
    NpcType_Trainer = 14,
    NpcType_Quest = 15
};

enum SkillNature
{
    SkillNature_Spell = 0,
    SkillNature_Skill = 1
};

enum SkillType
{
    SkillType_Heal = 0,
    SkillType_Attack = 1,
    SkillType_Bard = 2
};

enum SkillTargetRestrict
{
    SkillTargetRestrict_Npc = 0,
    SkillTargetRestrict_Friendly = 1,
    SkillTargetRestrict_Opponent = 2
};

enum SkillTargetType
{
    SkillTargetType_Normal = 0,
    SkillTargetType_Self = 1,
    SkillTargetType_Reserved2 = 2,
    SkillTargetType_Group = 3
};

// Endless Online number codec: base 253, "no value" and field-separator bytes.
#define EO_NUM_MAX 0xfd
#define EO_NUM_MAX_2 0xfa09
#define EO_NUM_MAX_3 0xf71ae5
#define EO_NUM_EMPTY 0xfe
#define EO_BREAK_BYTE 0xff

#define SECONDS_PER_DAY 0x15180
#define MS_PER_SECOND 1000

// The two-int record every by-value pair accessor returns.  In the reference
// there is exactly ONE such type: the frame-only empty constructor at 0x44f58c
// (emitted by Packets, the first unit that default-constructs one) is the call
// target of every one of these sites -- MapContainer's coordinate/stack pairs,
// Itemvalues' element and spec pairs, Skillvalues' damage and element pairs,
// Npcvalues' type and drop pairs and Shopvalues' craft ingredients all call it.
// Distinct reconstructed structs would each emit their own constructor COMDAT
// (33 bytes apiece, in a different module), which is how the duplication was
// found.  The field names are not recoverable; the union keeps each accessor's
// reading of the two slots legible.  The fields must sit in one anonymous
// aggregate member: bcc32 emits the single memcpy-style move to the caller's
// return slot only then (two plain scalar members yield a member-wise copy).
struct MapCoord
{
    union
    {
        struct
        {
            int x;
            int y;
        };
        struct
        {
            int element;
            int element_damage;
        };
        struct
        {
            int spec2;
            int spec3;
        };
        struct
        {
            int type;
            int behavior_id;
        };
        struct
        {
            int item_id;
            int amount;
        };
        struct
        {
            int min_damage;
            int max_damage;
        };
        struct
        {
            int id;
            int element_power;
        };
    };
    MapCoord()
    {
    }
};

typedef MapCoord ItemStack;
typedef MapCoord ItemElement;
typedef MapCoord ItemSpecXY;
typedef MapCoord NpcTypeInfo;
typedef MapCoord NpcDropInfo;
typedef MapCoord SkillDamage;
typedef MapCoord SkillElement;
typedef MapCoord ShopCraftIngredient;

#endif
