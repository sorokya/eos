# CLEANUP.md — cosmetic review of `src/`

Scope: every reconstructed `.cpp`/`.h` under [`src/`](src) (30,225 lines, 130 files),
reviewed for (1) inconsistent names, (2) missing names, (3) literals that belong in
[`src/Protocol.h`](src/Protocol.h), (4) literals that belong in named `#define`s, and
(5) forward declarations that have real implementations.

**Status (updated):** this file is a snapshot taken at one point in the reconstruction and
has since gone partly stale. What is applied and what remains:

*Applied and verified byte-neutral* (fresh `scripts/build_asm.sh`, `make verify` 496/496,
`make track` 1790/1801 rows / 407,957 of 668,128 bytes — unchanged before and after):

| Commit | Content |
| --- | --- |
| `810239a` | §2.4 (5 missing enum values + `enum WarpEffect`), §4.1/§4.2 constants added to `Protocol.h`, §1.10 (6 header guards), §1.4 (3 stale comments), §1.14 (hex case), §1.13 (`Serial.cpp` bool literals) |
| `d186973` | §1.5 (explicit-receiver naming: `Serial`, `Filecache`), §1.9 (`field_0xNN`/`pad_0xNN` unified across the non-cluster headers), §1.11 (`values`→`record_list`, `GetCount` in the `*values` units), §1.12 (no drift found in those units) |
| `325ea7b` | cluster (`Packets`/`Players`/`Mapcontrol`/`Mainform`/`Jukeboxcontrol`/`Npccontrol`/`Map.h`/`Player.h`/`Settings.h`/`Npc.h`): §2.1/§2.2 (`Account_DecodePassword`/`Account_EncodePassword`, `Server_BuildInit*Reply`; the rest were already named), §5.3/§5.4/§5.5 prototype hygiene, §1.7 (`PlayerCommand` `family`/`action`), §1.8 (`Player::account_ident`, `Players_IsAccountIdentOnline`), §1.9, §1.6/§2.3 (`Server *self`, `Mapcontrol *map_control`) |
| `835f401` | §3.x so far: §3.1 (reply codes), §3.2 (action/family pairs + guild replies), §3.3 (`WarpEffect`), §3.4 (`Direction`), §3.5 (`AdminLevel`), §3.6 (`SitAction`), §3.7, §3.8 (`Emote`/`Gender`) — the remainder of §3.x in *other* units' call sites is still open |

Each was gated on a full rebuild with no `Error E[0-9]+`, `make verify` 496/496 and
`make track` 1790/1801; nothing regressed.

*Already true in the tree, so the item below is stale:* §2.1's `Packets.cpp`/`Mapcontrol.cpp`
renames (e.g. `FUN_00463d40` is already `Server_BroadcastToAll`) and **§5.2** — the
duplicate `int,int` overload does not exist; `Server_Shutdown` already calls the
`unsigned char,unsigned char` form. Verify each remaining item against the tree before
acting on it.

*Not applied* — the cross-file cluster `src/Packets.*`, `src/Players.*`, `src/Mapcontrol.*`,
`src/Mainform.*`, `src/Jukeboxcontrol.*`, `src/Npccontrol.*`, `src/Map.h`, `src/Player.h`,
`src/Settings.h`, `src/Npc.h`: the rest of §2.1/§2.2, §5.3–§5.5 (remainder), §1.7, §1.8,
§1.9 there, §1.6/§2.3 (parameter types), §3.x (literals → enums) and §4.x (literals →
defines); plus the remaining §1.5/§1.11/§1.12 pockets elsewhere. §1.6/§1.7/§1.8/§3.x/§4.x
are pure renames or literal substitutions and are byte-neutral by construction, but a
rename must be applied atomically across every call site or the tree stops compiling.

*Verification hazard found while applying this:* `scripts/build_asm.sh` is **incremental**
(mtime-based), so a broken `src/` can be masked by a stale `build/*.asm`; a rename that is
interrupted mid-way leaves the tree unbuildable. Always confirm `build_asm.sh` exits 0 with
no `Error E[0-9]+` before trusting `make track`/`make verify`, and keep the pre-change commit
handy to revert a partial rename.

---

## 0. Ground rules this review used

### 0.1 What is safe to rename (verified against the reference)

Only the **unit file base names** reach the linked image as symbols. Everything else
(free-function names, member-function names, parameter names, local names, enum tags)
is erased by the compiler.

```
$ strings -a GameServer.exe | grep -E '\$qqr'
@Borlndmm@SysGetMem$qqri
@Borlndmm@SysFreeMem$qqrpv
@Borlndmm@SysReallocMem$qqrpv
@Borlndmm@HeapAddRef$qqrv
@Borlndmm@HeapRelease$qqrv          <- all five are DLL imports, not unit code

$ strings -a GameServer.exe | grep -c '@@[A-Za-z]*@Initialize'
   (one per unit; these come from the FILE name, per AGENTS.md rule 3)
```

**But class and struct names DO reach the image, as RTTI type strings.** Verified:

```
$ strings -a GameServer.exe | grep -nx 'ItemValues\|FileCache\|mySQLtask\|MySQLthread'
25:ItemValues *
72:ItemValues
724:mySQLtask *
730:mySQLtask
733:FileCache
736:std::vector<mySQLtask *,std::allocator<mySQLtask *> >
6194:MySQLthread
```

I enumerated every `class`/`struct` in `src/*.h` against the binary's string table.
The result splits them into two groups:

| Group | Names | Rename risk |
| --- | --- | --- |
| **RTTI-pinned — DO NOT RENAME** | `Asocketban` `Banned` `ChestController` `ChestItem` `ClassValue` `ClassValues` `DoorController` `EffectController` `EventController` `FileCache` `InnValue` `InnValues` `ItemValue` `ItemValues` `JukeBox` `JukeBoxController` `KillCounter` `KillCounters` `LearnItemVal` `LearnValue` `LearnValues` `Logins` `MapChest` `MapContainer` `MapItem` `MapObject` `MapWarp` `MsgBoard` `MsgBoardController` `mySQLbuffer` `mySQLtask` `MySQLthread` `Npc` `NpcController` `NpcDropItem` `NpcValue` `NpcValues` `Player` `PlayerCommand` `PlayerInventory` `PlayerQuest` `Players` `PlayerSkill` `Quest` `QuestAction` `QuestCounter` `QuestCounterList` `QuestCounters` `QuestRule` `QuestState` `QuestType` `Server` `Settings` `ShopCraftVal` `ShopItemVal` `ShopValue` `ShopValues` `SkillValue` `SkillValues` `TGUI` `Wedding` `WeddingController` | **Changes bytes.** Their spelling is ground truth. |
| **Not in the image — free to rename** | `EOEncodedObj` `FilecacheEntry` `FilecacheEntryB` `Gamecontrol` `GroundItemInfo` `ItemElement` `ItemSpecXY` `ItemStack` `Mapcontrol` `MapCoord` `Mysqlcontrols` `Newscontrol` `NpcDropInfo` `NpcTypeInfo` `Questengine` `Serial` `ShopCraftIngredient` `SkillDamage` `SkillElement` `WeaponmapEntry` | Cosmetic only. |

This resolves an apparent inconsistency that should **not** be "fixed":
`FileCache` next to `FilecacheEntry` in the same header, and lower-case `mySQLtask` /
`mySQLbuffer`, look wrong but are **correct** — they are recovered from RTTI. Only the
*invented* neighbours (`FilecacheEntry`, `FilecacheEntryB`) are adjustable.

### 0.2 The one construct where renaming *can* change codegen

Replacing an `int` literal with an **enum constant** is byte-identical *except* when it
changes overload resolution. There is exactly one place in the tree where that matters
today — see [§5.2](#52-conflicting-redeclaration-of-fun_00463d40). Everywhere else the
target parameter is a single `unsigned char`/`int`, so `2` and `WarpEffect_Admin` lower
identically.

### 0.3 Suggested verification for every change below

Per [AGENTS.md](AGENTS.md) rule 4, each batch should be proven, not assumed:

```sh
scripts/compare_asm.py <unit> <function>     # per-function instruction diff
make verify                                  # full rebuild + object/MD5 comparison
```

A pure-rename batch should produce **0 normalized instruction differences**.

---

## 1. Inconsistent names

### 1.1 `Mapcontrol` members split across two prefixes

[`src/Mapcontrol.h:55-152`](src/Mapcontrol.h) declares one class whose static members use
four different prefixes:

| Prefix | Count | Examples |
| --- | --- | --- |
| `Mapcontrol_` | 18 | `Mapcontrol_LoadMap`, `Mapcontrol_AddWarp`, `Mapcontrol_ToggleDoor` |
| `Map_` | 10 | `Map_GetWarpMap`, `Map_GetTileSpec`, `Map_IsTileWalkable`, `Map_GetWarpDoorAt` |
| `Itemchest_` | 1 | `Itemchest_GetSlot` (a `std::vector<MapItem>` helper, not an Itemchest op) |
| `Pub_` | 1 | `Pub_DecodeNumber_Map` |

Two members are also *doubly* prefixed: `Mapcontrol::Mapcontrol_AppendEncoded`,
`Mapcontrol::Mapcontrol_inc_player_count`. And `Mapcontrol_inc_player_count` /
`Mapcontrol_dec_player_count` are the only `snake_case` members in the whole tree
(everything else is `PascalCase` after the prefix).

**Suggested:** pick one prefix (`Mapcontrol_`) or drop prefixes entirely inside the class,
and rename `inc_player_count`/`dec_player_count` → `IncPlayerCount`/`DecPlayerCount`.

### 1.2 The per-unit EO number codec has 14 different names for 2 functions

Every unit that touches pub files carries its own copy of the EO number encoder/decoder
(this duplication is faithful to the reference — the *naming* is not):

| Unit | Encoder | Decoder |
| --- | --- | --- |
| `Packets` | `EO_EncodeNumber` | `EO_DecodeNumber` |
| `Chestcontrol` | `ChestController::AppendEncoded` | — |
| `Eventcontrol` | `EventController::AppendEncoded` | — |
| `Effectcontrol` | `EffectController::AppendEncoded` | — |
| `Msgboardcontrol` | `MsgBoardController::AppendEncoded` | `MsgBoardController::DecodeNumber` |
| `Questengine` | `Questengine::AppendEncoded` | — |
| `Mapcontrol` | `Mapcontrol::Mapcontrol_AppendEncoded` | `Mapcontrol::Pub_DecodeNumber_Map` |
| `Npccontrol` | `NpcController::Packet_AppendEncoded` | — |
| `Jukeboxcontrol` | `JukeBoxController::EncodeNumber` | — |
| `Shopvalues` | `ShopValues::EncodeNumber` | `ShopValues::DecodeNumber` |
| `Learnvalues` | `LearnValues::Pub_EncodeNumber_Learn` | `LearnValues::Pub_DecodeNumber_Learn` |
| `Classvalues` | — | `ClassValues::DecodeInt` |
| `Innvalues` / `Itemvalues` / `Npcvalues` / `Skillvalues` | — | `…::DecodeNumber` |

Five spellings of the encoder (`AppendEncoded`, `Mapcontrol_AppendEncoded`,
`Packet_AppendEncoded`, `EncodeNumber`, `Pub_EncodeNumber_Learn`) and three of the
decoder (`DecodeNumber`, `DecodeInt`, `Pub_DecodeNumber_*`).

**Suggested:** `<Owner>::EncodeNumber` / `<Owner>::DecodeNumber` everywhere; keep the
free `EO_EncodeNumber` / `EO_DecodeNumber` in `Packets`.

`ClassValues::DecodeInt` ([`src/Classvalues.cpp:134`](src/Classvalues.cpp)) is the odd one
out and its body is identical to the others — rename to `DecodeNumber`.

### 1.3 `Pub_` prefix applied to 8 of ~40 pub-file operations

```
Pub_DecodeNumber_Learn  Pub_DecodeNumber_Map  Pub_EncodeNumber_Learn
Pub_LoadDrops  Pub_LoadNpcs  Pub_LoadShops  Pub_LoadSkillMasters  Pub_LoadTalk
```

Their siblings do the same job unprefixed: `LoadItems`, `LoadClasses`, `LoadSpells`,
`LoadInns`, `LoadShops`. Note also `Pub_LoadShops` is referenced only in a **comment**
([`src/Shopvalue.h:15`](src/Shopvalue.h)) — the real symbol is `ShopValues::LoadShops`.

**Suggested:** drop the `Pub_` prefix and the `_Learn`/`_Map` suffixes.

### 1.4 Stale names left in comments

| File | Comment says | Actual symbol |
| --- | --- | --- |
| [`src/Npcvalues.h:10`](src/Npcvalues.h) | `Enf_GetType` | `NpcValues::GetType` |
| [`src/Npcvalue.h:14`](src/Npcvalue.h) | `Enf_GetExp/MaxHp/Type` | `NpcValues::GetExp` / `GetMaxHp` / `GetType` |
| [`src/Shopvalue.h:15`](src/Shopvalue.h) | `Pub_LoadShops` | `ShopValues::LoadShops` |

(`Eif_Get*` in [`src/Itemvalue.h:11`](src/Itemvalue.h) *is* accurate — `ItemValues` really
does use that prefix. Which is itself the inconsistency: only the item table got an
`Eif_` prefix; `NpcValues`/`SkillValues`/`ClassValues` did not.)

### 1.5 The "self" parameter has five spellings

Free-function-form members take the object explicitly. Across `src/*.cpp`:

| Spelling | Count |
| --- | --- |
| `*self` | 188 |
| `*map_control` | 33 (all of `Mapcontrol`) |
| `*players` | 6 |
| `*settings` | 3 |
| `*s` | 13 (all of `Serial`) |
| `*context` | 1 (`NpcController::Packet_AppendEncoded`) |
| `*mysql`, `*db_handle`, `*map` | 1 each |

`Players.cpp` is internally inconsistent — **39** `Players *self` vs **5** `Players *players`:

```
src/Players.cpp:210  void Players::Player_LevelUp(Players *players, Player *player)
src/Players.cpp:233  int  Players::Player_TryLevelUp(Players *players, Player *player)
src/Players.cpp:245  void Players::Party_AddNewMember(Players *players, …)
src/Players.cpp:262  void Players::Player_LeaveParty(Players *players, Player *player)
src/Players.cpp:350  bool Players::CharName_Validate(Players *players, Player *player, String name)
```

Constructor parameters are separate: `Banned::Banned(Mysqlcontrols *db_handle)`
([`src/Banned.cpp:9`](src/Banned.cpp)) vs `Logins::Logins(Mysqlcontrols *mysql)`
([`src/Logins.cpp:8`](src/Logins.cpp)) vs `Players::Players(Settings *settings,
Mysqlcontrols *mysql_controls)` ([`src/Players.cpp:28`](src/Players.cpp)).

**Suggested:** `*self` for the receiver everywhere; `mysql_controls` for a
`Mysqlcontrols*` argument.

### 1.6 `void *self` where the argument is always `Server *`

[`src/Packets.h:190-206`](src/Packets.h) declares 9 functions with an untyped receiver:

```cpp
int          EO_DecodeNumber(void *self, String data);
int          EO_DecodeByte(void *self, char value);
char         EO_GetBreakByte(void *self, int value);
unsigned int Server_DecodePacketLength(void *self, String data);
String       PacketReader_GetBreakStringAt(void *reader, …);
bool         Coords_IsAdjacent(void *self, int x1, int y1, int x2, int y2);
bool         Server_InViewRange(void *self, …);
bool         Server_InViewRing(void *self, …);
bool         Server_InViewRangeReverse(void *self, …);
bool         Server_InItemViewRing(void *self, …);
```

Every call site passes a `Server *` — 182 for `EO_GetBreakByte`, 181 for
`EO_DecodeNumber`, 12 for `Server_InViewRange`, 6 for `Coords_IsAdjacent`, 0 otherwise.
The parameter is unused in all of them.

**Suggested:** `Server *self`. Same 4-byte argument, same code; removes an implicit
`Server* → void*` conversion at 380+ call sites.

Same for [`src/Weaponmap.h:23`](src/Weaponmap.h) `bool Combat_IsRangedWeapon(void *unused, int doll_graphic_id)`.

### 1.7 `PlayerCommand` field names are the wrong way round

[`src/Playercommand.h:11-19`](src/Playercommand.h):

```cpp
struct PlayerCommand { int action; int arg; String text; };
```

Every producer passes `(family, action, data)` — 14 sites, e.g.
[`src/Packets.cpp:146`](src/Packets.cpp):

```cpp
PlayerCommand command(family, action, data);
```

and the single consumer, [`src/Players.cpp:877-903`](src/Players.cpp), dispatches on
`.action` using **family** values and forwards `.arg` as the **action**:

```cpp
if ((*iter)->action_queue[0].action == 6)                   // PacketFamily_Walk
    Walk_Execute(server, *iter, (*iter)->action_queue[0].arg, …);
if ((*iter)->action_queue[0].action == 11)                  // PacketFamily_Attack
    Attack_Execute(server, *iter, (*iter)->action_queue[0].arg, …);
```

So the field called `action` holds the family and the field called `arg` holds the action.

**Suggested:** `int family; int action; String data;` — and fix the ctor parameter names
to match. Purely a rename; the struct layout (int, int, AnsiString) is unchanged.

### 1.8 Names that encode a raw offset

[`src/Players.h:77`](src/Players.h) / [`src/Players.cpp:515`](src/Players.cpp):

```cpp
static bool Players_HasField0C(Players *self, int field_c);
```

`Player::field_0xc` is provably the **account ident**: it is initialised to `-1`
([`src/Player.cpp:41`](src/Player.cpp)) and used to build
`"… WHERE ident = '" + IntToStr((unsigned int)player->field_0xc) + "' LIMIT 1"`
([`src/Packets.cpp:1402`](src/Packets.cpp)) and
`" AND ident_account = " + IntToStr((unsigned int)player->field_0xc)`
([`src/Packets.cpp:1497`](src/Packets.cpp)). The helper's caller passes the `ident` column
read straight from the DB ([`src/Packets.cpp:10141`](src/Packets.cpp)).

**Suggested:** `Player::account_ident`, and `Players_HasField0C` →
`Players_IsAccountIdentOnline`.

### 1.9 Three naming schemes for unknown/padding members

| Scheme | Where | Count |
| --- | --- | --- |
| `field_0xNN` | `Map.h`, `Packets.h`, `Player.h` | 13 |
| `field_NN` (hex, no prefix) | `Banned.h` `Classvalues.h` `Filecache.h` `Gamecontrol.h` `Innvalues.h` `Itemvalues.h` `Learnvalues.h` `Logins.h` `Mainform.h` `Msgboardcontrol.h` `Mysqlcontrols.h` `Mysqltask.h` `Mysqlthread.h` `Npcvalues.h` `Shopvalues.h` `Skillvalues.h` | 39 distinct |
| `padding_0xNN` | `Packets.h:45`, `Packets.h:75` | 2 |
| `pad_NN` | 18 headers | 32 distinct |

Within `pad_`, the zero-padding is inconsistent for the **same offset**:

```
src/Filecache.h:38     char pad_01[3];      <- offset 0x01
src/Learnvalues.h:21   char pad_01[3];      <- offset 0x01
src/Innvalues.h:17     char pad_1[3];       <- offset 0x01, different spelling
src/Shopvalues.h:33    char pad_1[3];       <- offset 0x01, different spelling
```

**Suggested:** one scheme — `field_0xNN` and `pad_0xNN`, offsets lower-case hex, no
zero-padding beyond what the offset needs; fold `padding_0x34`/`padding_0xc4` into it.

### 1.10 Header-guard macros disagree with their file names

6 of 65 headers deviate from `<Basename>H`:

| File | Guard | Expected |
| --- | --- | --- |
| [`src/Classvalue.h:2`](src/Classvalue.h) | `ClassValueH` | `ClassvalueH` |
| [`src/Classvalues.h:2`](src/Classvalues.h) | `ClassValuesH` | `ClassvaluesH` |
| [`src/Npcdrop.h:2`](src/Npcdrop.h) | `NpcDropItemH` | `NpcdropH` |
| [`src/Playerinventory.h:2`](src/Playerinventory.h) | `PlayerInventoryH` | `PlayerinventoryH` |
| [`src/Playerskill.h:2`](src/Playerskill.h) | `PlayerSkillH` | `PlayerskillH` |
| [`src/Skillvalues.h:2`](src/Skillvalues.h) | `SkillValuesH` | `SkillvaluesH` |

Preprocessor-only; cannot affect codegen.

### 1.11 Parallel "values" units diverge on member names

Structurally identical classes, different spellings:

| | `ItemValues` | `ClassValues` | `NpcValues` | `SkillValues` |
| --- | --- | --- | --- | --- |
| record count | `num_items` | `num_classes` | `count` | `num_skills` |
| resource ids | `rid_1` / `rid_2` | `rid_1` / `rid_2` | `rid1` / `rid2` | `rid1` / `rid2` |
| record vector | `values` | `values` | `record_list` | `record_list` |
| count accessor | `GetCount` (static) | `size()` (member) | `GetCount` (static) | `GetCount()` (member) |

`LearnValues`/`ShopValues`/`InnValues` add a fourth accessor spelling: `GetRecordCount`
(static) and `GetCount()` (member).

**Suggested:** `num_records` (or keep the domain-specific name consistently), `rid_1`/`rid_2`,
`record_list`, and one `GetCount` form.

**Caution:** switching a member function between *non-static* (`int ItemValues::DecodeNumber(String)`)
and *static-with-explicit-self* (`ShopValues::EncodeNumber(ShopValues *self, …)`) is **not**
cosmetic — it changes the `this` convention and the mangled name. AGENTS.md prefers the
static form; treat that as a fidelity task, not part of this cleanup.

### 1.12 Declaration/definition parameter-name drift

| Symbol | Declaration | Definition |
| --- | --- | --- |
| `Character_BuildSaveQuery` | `int flag` ([`Players.h:16`](src/Players.h), [`Packets.cpp:7003`](src/Packets.cpp)) | `int flags` ([`Players.cpp:915`](src/Players.cpp)) |
| `Player_ApplyQuestActions` | `bool flag` ([`Packets.cpp:94`](src/Packets.cpp), `:7460`) | `bool repeat` ([`Packets.cpp:10846`](src/Packets.cpp)) |
| `Attack_Execute` | `Player *player, String *data` ([`Players.cpp:18`](src/Players.cpp)) | `Player *caster, String *reader` ([`Packets.cpp:11450`](src/Packets.cpp)) |
| `Spell_Execute` | `Player *player, String *data` ([`Players.cpp:19`](src/Players.cpp)) | `Player *caster, String *packet_data` ([`Packets.cpp:11602`](src/Packets.cpp)) |
| `Mapcontrol_AddArenaSpawn` | `Mapcontrol *map` ([`Mainform.cpp:37`](src/Mainform.cpp)) | `Mapcontrol *map_control` ([`Mapcontrol.cpp:331`](src/Mapcontrol.cpp)) |
| `FUN_004aa4e4` | `JukeBoxController *jukebox_control` ([`Mapcontrol.cpp:21`](src/Mapcontrol.cpp)) | `JukeBoxController *self` ([`Jukeboxcontrol.cpp:122`](src/Jukeboxcontrol.cpp)) |
| `RandRange` | `int range` ([`Packets.cpp:40`](src/Packets.cpp)); `int max` (3 headers) | `int max` ([`Players.cpp:23`](src/Players.cpp)) |

Also inconsistent: `Attack_Execute`/`Spell_Execute` name the third parameter `String *reader`
and `String *packet_data` while the identical parameter in `Walk_Execute`, `Face_Execute` and
`Chair_Execute` is `String *data`.

### 1.13 `bool` members assigned `0`/`1`

43 sites assign an int literal to a `bool` member (38 in `Packets.cpp`, 3 in
`Mapcontrol.cpp`, 2 in `Serial.cpp`), e.g.:

```
src/Packets.cpp:2523     player->sitting = 1;
src/Packets.cpp:2524     player->on_chair = 0;
src/Mapcontrol.cpp:1482  map->can_scroll = 1;
```

while neighbouring lines in the same functions already use `true`/`false`
(`src/Mapcontrol.cpp:1484 map->boss_alive = false;`). `bool b = 1;` and `bool b = true;`
both lower to `mov byte ptr [..],1`.

### 1.14 Hex literal case is mixed for the *same* constant

| Constant | Upper | Lower |
| --- | --- | --- |
| `0xFD` / `0xfd` | 2 | 20 |
| `0xFA09` / `0xfa09` | 1 | 9 |
| `0xF71AE5` / `0xf71ae5` | 1 | 9 |
| `0xFE` / `0xfe` | 11 | 11 |

The uppercase forms are concentrated in [`src/Packets.cpp:9530-9541`](src/Packets.cpp)
and `:9662`; the same three constants appear lower-case in 15 other units.
See [§4.1](#41-the-eo-number-base-constants) — naming them removes the issue entirely.

---

## 2. Missing names

### 2.1 `FUN_xxxxxxxx` in real (non-stub) code — 26 symbols

These are live, fully reconstructed functions still carrying their Ghidra address name.
(The ~340 other `FUN_` occurrences are inside the `// STUB(…)` block of
[`src/Packets.cpp:9873+`](src/Packets.cpp) and [`src/Players.cpp:1150+`](src/Players.cpp);
those are placeholders and are out of scope.)

**Defined in [`src/Mapcontrol.cpp`](src/Mapcontrol.cpp):**

| Symbol | Line | What the body does | Suggested name |
| --- | --- | --- | --- |
| `FUN_0047c3a4` | 944 | if `quest_cooldown < 1`, set it to 10 and return 1 | `Mapcontrol_TryTakeQuestCooldown` |
| `FUN_0047c3f0` | 959 | returns `map->can_scroll` | `Mapcontrol_GetCanScroll` |
| `FUN_0047c428` | 967 | returns `{relog_x, relog_y}`, clamped to `{0,0}` if out of bounds | `Mapcontrol_GetRelogCoords` |
| `FUN_0047c634` | 988 | npc id for an npc index, `0xffffffff` if absent | `Mapcontrol_GetNpcIdByIndex` |
| `FUN_0047c6c0` | 1010 | npc `{x,y}` for an npc index, `{-1,-1}` if absent | `Mapcontrol_GetNpcCoordsByIndex` |
| `FUN_0047c890` | 1035 | — | `Mapcontrol_*` (needs a read) |
| `FUN_0047c27c` | 1089 | tile-spec value at (x,y) via the tile-bit index | `Mapcontrol_GetTileSpecValueAt` |
| `FUN_00486e64` | 1124 | chest `key_id` at coords | `Mapcontrol_GetChestKeyAt` |
| `FUN_0047cd28` | 1147 | counts blocked neighbours of (x,y) | `Mapcontrol_CountBlockedNeighbors` |
| `FUN_004879b0` | 1179 | ground-item drop allowed at (x,y) for a player (≤9 own items, protect expired) | `Mapcontrol_CanDropItemAt` |
| `FUN_00487ac0` | 1218 | ground item info by index, `x=-2` if still protected | `Mapcontrol_TakeGroundItemInfo` |
| `FUN_0047badc` | 1262 | — | `Mapcontrol_*` (needs a read) |
| `FUN_00481e0c` | 1300 | clears every per-map container and resets w/h | `Mapcontrol_ResetMap` |
| `FUN_004876c0` | 1334 | erases + deletes the ground item with a given index | `Mapcontrol_RemoveGroundItem` |
| `FUN_004827c8` | 1363 | reset → jukebox reset → reload | `Mapcontrol_ReloadMap` |
| `FUN_00482834` | 1420 | parses an EMF into a `MapContainer` | `Mapcontrol_ParseMapFile` |

`FUN_00486e64` is an especially clear case: it is **byte-for-byte the same shape** as
`Mapcontrol::Mapcontrol_GetChestSlotCount` ([`src/Mapcontrol.cpp:572`](src/Mapcontrol.cpp))
and `Mapcontrol::Map_GetWarpDoorAt` (`:549`) — same guard, same iteration — differing only
in the field it returns (`key_id` vs `slots.size()` vs `value`).

**Defined in [`src/Packets.cpp`](src/Packets.cpp):**

| Symbol | Line | What the body does | Suggested name |
| --- | --- | --- | --- |
| `FUN_004728f8` | 12983 | rolls `sent_bytes` into KB/MB | `Server_AddSentBytes` |
| `FUN_00472944` | 12999 | rolls `received_bytes` into KB/MB | `Server_AddReceivedBytes` |
| `FUN_004731d0` | 13026 | formats sent traffic as `"n.nn Mb"` | `Server_FormatSentTraffic` |
| `FUN_00473540` | 13036 | same for received | `Server_FormatReceivedTraffic` |
| `FUN_004738b0` | 13046 | true once every >5 s since `start_time` | `Server_TickOncePerFiveSeconds` |
| `FUN_00473920` | 13060 | appends a timestamped line to `.\logs\chatNN.log` | `Server_AppendChatLog` |
| `FUN_00466840` | 9005 | pushes the map's quake/drain/spike flags onto every player on it | `Server_SyncMapHazardFlags` |
| `FUN_00463750` | 9260 | sends to party members **on the same map** | `Server_BroadcastToPartyOnMap` |
| `FUN_004639b8` | 9274 | sends to everyone on a map **plus all admins** | `Server_BroadcastToMapAndAdmins` |
| `FUN_00463be8` | 9289 | sends to all admins except the given player | `Admin_BroadcastToOtherAdmins` |
| `FUN_00463d40` | 9305 | sends to every logged-in player | `Server_BroadcastToAll` |
| `FUN_00473124` | 13015 | `Coords_IsAdjacent` with radius 2 | `Coords_IsWithinTwo` |

`FUN_00463750`…`FUN_00463d40` sit **inside** an otherwise fully named family
(`Server_BroadcastToParty`, `Guild_BroadcastToAll`, `Server_BroadcastAdjacent`,
`Server_BroadcastNearTile`, `Admin_BroadcastToAll`, `Admin_ReportToGMs`,
`Admin_BroadcastToAdmins`, `Server_BroadcastNearby`, `Server_BroadcastToMap`) — the five
unnamed ones are the most visible remaining gap in `Packets.cpp`.

**Elsewhere:** `FUN_004aa4e4` ([`src/Jukeboxcontrol.cpp:122`](src/Jukeboxcontrol.cpp)),
`FUN_00403080` ([`src/Mainform.cpp:41`](src/Mainform.cpp), a `TGUI` serial/unlock check),
`FUN_0047060c` / `FUN_004708d4` ([`src/Packets.cpp:12442`, `:12487`](src/Packets.cpp),
password-string transforms), `FUN_004628b0` / `FUN_00462bf8` / `FUN_00462e38`
(`:11351`, `:11370`, `:11381`).

### 2.2 `FUN_` names that leaked into public headers

```
src/Mapcontrol.h:155  int    FUN_00486e64(int map_control, int map_id, MapCoord coords);
src/Mapcontrol.h:156  String FUN_0047badc(Mapcontrol *map_control, int map_id, MapCoord coords);
src/Packets.h:188     int    FUN_0044f73c(void *range);
src/Packets.h:189     int    FUN_0044f710(void *range);
```

These are the only unnamed symbols in the header API surface.
`FUN_0044f73c`/`FUN_0044f710` have no definition at all (only `_Stub`s) — they look like
`std::basic_string` iterator helpers given the `void *range` parameter.

### 2.3 `int map_control` where the type is known

Every function listed in §2.1 under `Mapcontrol.cpp` takes `int map_control` and then
casts it back on every single use:

```cpp
// src/Mapcontrol.cpp:1124
int FUN_00486e64(int map_control, int map_id, MapCoord coords)
{
    if (map_id > 0 && map_id <= Mapcontrol_GetCount((Mapcontrol *)map_control))
    { … Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1) … }
}
```

and callers cast on the way in: `FUN_0047cd28((int)server->map_control, …)`
([`src/Packets.cpp:289`](src/Packets.cpp) and 3 more sites).

Changing the parameter to `Mapcontrol *map_control` deletes 40+ casts. A 4-byte pointer
and a 4-byte int are passed identically, and the casts are no-ops, so this should be
byte-neutral — but it touches many functions, so verify with `compare_asm.py` before
committing.

The same applies to the `int` receiver in
`FUN_0047c3a4`, `FUN_0047c3f0`, `FUN_0047c428`, `FUN_0047c634`, `FUN_0047c6c0`,
`FUN_0047c890`, `FUN_0047c27c`, `FUN_0047cd28`, `FUN_004879b0`, `FUN_00487ac0`,
`FUN_00481e0c`, `FUN_004876c0`, `FUN_004827c8`.

### 2.4 Missing enum values in `Protocol.h`

Diffed [`src/Protocol.h`](src/Protocol.h) against the checked-out spec
(`ref/eo-protocol/xml/**/protocol.xml`). Five values and one whole enum are absent, and
**every one of them is used as a bare literal in the code today**:

| Missing | Value | Used at |
| --- | --- | --- |
| `LoginReply_Banned` | 4 | [`src/Packets.cpp:10126`](src/Packets.cpp) (the `banned > 0` branch) |
| `LoginReply_Busy` | 6 | [`src/Packets.cpp:1353`, `:1363`](src/Packets.cpp) (access lock / connection threshold) |
| `CharacterReply_Full` | 2 | [`src/Packets.cpp:1622`](src/Packets.cpp) (3 slots taken, on Create) |
| `CharacterReply_Full3` | 3 | [`src/Packets.cpp:1575`](src/Packets.cpp) (3 slots taken, on Request) |
| `InitReply_Banned` | 3 | not yet used |
| `enum WarpEffect` | `None=0`, `Scroll=1`, `Admin=2` | `Player_Warp`'s `warp_anim` — see [§3.3](#33-warpeffect) |

Spec paths: `ref/eo-protocol/xml/net/server/protocol.xml`.
Everything else in `Protocol.h` matches the spec exactly — no wrong values, no wrong names.

### 2.5 Declared but never implemented

| Symbol | Declared | Status |
| --- | --- | --- |
| `Players_Iter_Begin` | [`Packets.cpp:39`](src/Packets.cpp), [`Npccontrol.cpp:16`](src/Npccontrol.cpp) | no definition anywhere in `src/` |
| `Players_Iter_End` | [`Packets.cpp:48`](src/Packets.cpp), [`Npccontrol.cpp:17`](src/Npccontrol.cpp) | no definition anywhere in `src/` |
| `Server_BroadcastToMapExceptSelf` | [`Packets.cpp:7005`](src/Packets.cpp) | no definition |
| `Sock_Send` | [`Packets.cpp:7276`](src/Packets.cpp) | no definition (external `__fastcall`) |
| `FUN_00462374` | [`Packets.cpp:7002`](src/Packets.cpp) as `bool(Server*,Player*,String)` | only `FUN_00462374_Stub(int,int)` exists — the declaration and the stub disagree on arity *and* types |
| `FUN_0044f73c`, `FUN_0044f710` | [`Packets.h:188-189`](src/Packets.h) | only `_Stub`s |
| `FUN_00403080` | [`Mainform.cpp:41`](src/Mainform.cpp) | no definition |

`Players_Iter_Begin`/`End` are used ~40 times across `Packets.cpp` and `Npccontrol.cpp`
and belong in [`src/Players.h`](src/Players.h) next to the rest of the `Players` API, not
re-declared per `.cpp`.

---

## 3. Literals that should be `Protocol.h` enum values

Adoption is currently very uneven: `PacketFamily_*` (416 uses) and `PacketAction_*` (503)
are near-complete, but **22 of the 56 enums in `Protocol.h` have zero uses**:

```
MapType  MapTimedEffect  MapMusicControl  DialogReply  FileType  MarriageRequestType
QuestPage  PartyRequestType  InitReply  InitBanType  CharacterIcon  AvatarChangeType
TalkReply  SitState  MapEffect  InnUnsubscribeReply  CharacterReply  SkillMasterReply
AccountReply  LoginReply  DialogEntryType  QuestRequirementIcon  WelcomeCode
LoginMessageCode  AdminMessageType  PlayerKilledState  NpcKillStealProtectionState
MapDamageType  MarriageReply  PartyReplyCode  Emote  Gender  ItemSize  SkillNature
SkillTargetRestrict
```

### 3.1 Reply codes — 45 confirmed sites

Extracted by matching `Client_SendEncoded(server, player, <Action>, <Family>,
EO_EncodeNumber(server, <literal>, …))`. Every one maps cleanly onto an existing (or
§2.4-added) enum constant:

| Line(s) in `Packets.cpp` | Family | Literal | Replace with |
| --- | --- | --- | --- |
| 1353, 1363 | Login | 6 | `LoginReply_Busy` **(add)** |
| 10112 | Login | 1 | `LoginReply_WrongUser` |
| 10120 | Login | 2 | `LoginReply_WrongUserPassword` |
| 10126 | Login | 4 | `LoginReply_Banned` **(add)** |
| 10137, 10143 | Login | 5 | `LoginReply_LoggedIn` |
| 1411, 1420 | Account | 7 | `AccountReply_RequestDenied` |
| 10261 | Account | 1 | `AccountReply_Exists` |
| 10324 | Account | 3 | `AccountReply_Created` |
| 10352 | Account | 5 | `AccountReply_ChangeFailed` |
| 10359 | Account | 6 | `AccountReply_Changed` |
| 1575 | Character | 3 | `CharacterReply_Full3` **(add)** |
| 1622 | Character | 2 | `CharacterReply_Full` **(add)** |
| 1631 | Character | 4 | `CharacterReply_NotApproved` |
| 10199 | Character | 1 | `CharacterReply_Exists` |
| 1669, 1881 | Welcome | 3 | `WelcomeCode_ServerBusy` |
| 4207, 4216 | Citizen | 0 | `InnUnsubscribeReply_NotCitizen` |
| 4225 | Citizen | 1 | `InnUnsubscribeReply_Unsubscribed` |
| 4944 | Party | 1 | `PartyReplyCode_AlreadyInYourParty` |
| 4954, 4984 | Party | 2 | `PartyReplyCode_PartyIsFull` |
| 4974 | Party | 0 | `PartyReplyCode_AlreadyInAnotherParty` |
| 6191 | StatSkill | 2 | `SkillMasterReply_WrongClass` |
| 6257 | StatSkill | 1 | `SkillMasterReply_RemoveItems` |
| 6459 | Marriage | 1 | `MarriageReply_AlreadyMarried` |
| 6487 | Marriage | 2 | `MarriageReply_NotMarried` |
| 6468, 6508, 6535 | Marriage | 4 | `MarriageReply_NotEnoughGold` |
| 6496 | Marriage | 5 | `MarriageReply_WrongName` |
| 6526 | Marriage | 6 | `MarriageReply_ServiceBusy` |
| 6515 | Marriage | 7 | `MarriageReply_DivorceNotification` |
| 6626, 6635 | Priest | 1 | `PriestReply_NotDressed` |
| 6617 | Priest | 2 | `PriestReply_LowLevel` |
| 6671, 6680, 6689 | Priest | 3 | `PriestReply_PartnerNotPresent` |
| 6726, 6735 | Priest | 4 | `PriestReply_PartnerNotDressed` |
| 6603 | Priest | 5 | `PriestReply_Busy` |
| 6698 | Priest | 7 | `PriestReply_PartnerAlreadyMarried` |
| 6707, 6717 | Priest | 8 | `PriestReply_NoPermission` |

Two sites have no spec enum and should stay literal (or get a local `#define`):
`Packets.cpp:4402` (Jukebox reply `1`) and `Packets.cpp:6960` (Message Pong `2`).

Sanity check that the mapping is right — [`src/Packets.cpp:10110-10145`](src/Packets.cpp):

```cpp
if ((*MAINFORM)->myquery->RecordCount < 1)                           // no such account
    Client_SendEncoded(server, player, 3, 4, EO_EncodeNumber(server, 1, 2) + "NO");
if (password != FUN_0047060c(server, …"password"))                   // bad password
    Client_SendEncoded(server, player, 3, 4, EO_EncodeNumber(server, 2, 2) + "NO");
if (Mysqlcontrols::Db_GetInt(server->mysql_controls, "banned") > 0)  // banned
    Client_SendEncoded(server, player, 3, 4, EO_EncodeNumber(server, 4, 2) + "NO");
```

`1 = WrongUser`, `2 = WrongUserPassword`, `4 = Banned` — exactly the spec's `LoginReply`.

### 3.2 Bare `action, family` pairs — 33 sites, all in `Packets.cpp`

Everywhere else in the tree these two arguments are named enums; in the guild/party/attack
callbacks they are still numbers:

| Lines | Call | Literals | Replace with |
| --- | --- | --- | --- |
| 10112, 10120, 10126, 10137, 10143 | `Client_SendEncoded` | `3, 4` | `PacketAction_Reply, PacketFamily_Login` |
| 10401, 10406, 10413, 10427, 10433, 10442, 10453, 10459, 10470, 10534, 10562, 10691, 10701, 10713, 10809 | `Client_SendEncoded` | `3, 0x27` | `PacketAction_Reply, PacketFamily_Guild` |
| 10411 | `Client_SendEncoded` | `2, 0x27` | `PacketAction_Accept, PacketFamily_Guild` |
| 10481 | `Client_SendEncoded` | `9, 0x27` | `PacketAction_Take, PacketFamily_Guild` |
| 10514 | `Client_SendEncoded` | `0x1d, 0x27` | `PacketAction_Rank, PacketFamily_Guild` |
| 10521 | `Client_SendEncoded` | `12, 0x27` | `PacketAction_Sell, PacketFamily_Guild` |
| 10555 | `Client_SendEncoded` | `0x14, 0x27` | `PacketAction_Tell, PacketFamily_Guild` |
| 10684 | `Client_SendEncoded` | `0x15, 0x27` | `PacketAction_Report, PacketFamily_Guild` |
| 10704 | `Server_BroadcastToMap` | `1, 0x27` | `PacketAction_Request, PacketFamily_Guild` |
| 10757 | `Guild_BroadcastToAll` | `6, 0x27` | `PacketAction_Create, PacketFamily_Guild` |
| 10761 | `Client_SendEncoded` | `6, 0x27` | `PacketAction_Create, PacketFamily_Guild` |
| 10808 | `Client_SendEncoded` | `5, 0x27` | `PacketAction_Agree, PacketFamily_Guild` |
| 11560 | `Server_BroadcastToParty` | `5, 0x18` | `PacketAction_Agree, PacketFamily_Party` |
| 11593 | `Server_BroadcastNearby` | `8, 0xb` | `PacketAction_Player, PacketFamily_Attack` |
| 11623 | `Server_BroadcastNearby` | `1, 0xc` | `PacketAction_Request, PacketFamily_Spell` |

Also 10 guild **reply codes** in the same block, where `GuildReply_*` already exists and is
used 27 times elsewhere:

```
10401, 10427  0x18 -> GuildReply_RankingNotMember
10406, 10433  0x17 -> GuildReply_RankingLeader
10413, 10442  0x12 -> GuildReply_Updated
10453         0x14 -> GuildReply_RemoveLeader
10459         0x15 -> GuildReply_RemoveNotMember
10470         0x16 -> GuildReply_Removed
10534, 10562  0x11 -> GuildReply_NotFound
10691          5   -> GuildReply_Exists
10701          6   -> GuildReply_CreateBegin
10713          4   -> GuildReply_NoCandidates
10809         0x10 -> GuildReply_Accepted
```

### 3.3 `WarpEffect`

`Player_Warp`'s 5th parameter is named `warp_anim` and takes only `0`, `1`, `2` across
12 call sites — exactly the spec's `WarpEffect`:

| Value | Sites | Meaning |
| --- | --- | --- |
| `0` | 7, incl. [`Packets.cpp:225`](src/Packets.cpp), `:242`, `:1047`, `:4192` | `WarpEffect_None` |
| `1` | 2: `:386`, `:2912` (scroll/teleport item) | `WarpEffect_Scroll` |
| `2` | 2: `:311`, `:351` (the `$t` and `$w` admin commands) | `WarpEffect_Admin` |

**Suggested:** add `enum WarpEffect { WarpEffect_None = 0, WarpEffect_Scroll = 1,
WarpEffect_Admin = 2 };` to `Protocol.h` and rename the parameter `warp_effect`.

### 3.4 `Direction` — 19 raw sites vs 17 named

`Direction_*` is already used in `Npccontrol.cpp:105-168` and `Packets.cpp:7317-7403`, but
the same comparisons are numeric in 5 other blocks:

```
src/Npccontrol.cpp:444,458      npc->direction == 1 / == 0
src/Packets.cpp:275-281         player->direction == 1 / 2 / 3 / 0
src/Packets.cpp:335-341         target->direction == 1 / 2 / 3 / 0
src/Packets.cpp:11484-11490     caster->direction == 0 / 1 / 2 / 3
src/Packets.cpp:12216,12231-46  direction = 3 / == 0 / == 1 / == 2 / == 3
```

`0/1/2/3` → `Direction_Down` / `Direction_Left` / `Direction_Up` / `Direction_Right`.

### 3.5 `AdminLevel` — 43 raw sites vs 3 named

`AdminLevel_*` is used only at [`Eventcontrol.cpp:119`](src/Eventcontrol.cpp),
[`Player.cpp:30`](src/Player.cpp) and [`Packets.cpp:9222`](src/Packets.cpp). The other 43
comparisons are numeric, e.g.:

```
src/Packets.cpp:191   if (player->admin_level > 0 …)     -> > AdminLevel_Player
src/Packets.cpp:195   if (player->admin_level < 2)       -> < AdminLevel_LightGuide
src/Packets.cpp:217   if (target->admin_level > 1)       -> > AdminLevel_Spy
src/Packets.cpp:249   if (player->admin_level < 3)       -> < AdminLevel_Guardian
src/Packets.cpp:665   if (player->admin_level < 4)       -> < AdminLevel_GameMaster
src/Packets.cpp:876   if (player->admin_level >= 4)      -> >= AdminLevel_GameMaster
```

Full list of lines in `Packets.cpp`: 191, 195, 217, 249, 333, 395, 407, 421, 442, 504, 556,
593, 608, 641, 665, 705, 760, 768, 876, 924, 932, 970, 1005, 1029, 1179, 1212, 1231, 8360,
8362, 8373, 8375, 8623, 8625, 8635, 8637, 8676, 8678, 8688, 8690, 9281.

### 3.6 `SitAction` — one named site, one numeric

```
src/Packets.cpp:7339  if (sit_action == SitAction_Sit)     <- Chair_Execute
src/Packets.cpp:2519  if (type == 1)                       <- the Sit handler, same value
```

The local is also named `sit_action` in one and `type` in the other.

### 3.7 `PacketFamily` in the action-queue dispatcher

[`src/Players.cpp:877-897`](src/Players.cpp) — `Players.cpp` already includes
`Protocol.h`, so this is a drop-in:

```
.action == 6   -> PacketFamily_Walk
.action == 11  -> PacketFamily_Attack
.action == 12  -> PacketFamily_Spell
.action == 8   -> PacketFamily_Chair
.action == 7   -> PacketFamily_Face
```

(See [§1.7](#17-playercommand-field-names-are-the-wrong-way-round) — the field should be
called `family`.)

### 3.8 `Emote` and `Gender`

```
src/Packets.cpp:2489-2490   > 10 && != 14   -> > Emote_Embarrassed && != Emote_Playful
src/Packets.cpp:1615        gender < 0 || gender > 1  -> < Gender_Female || > Gender_Male
```

### 3.9 Units that use protocol values but do not include `Protocol.h`

Only 12 of 60 `.cpp` files include it:

```
Chestcontrol Effectcontrol Eventcontrol Gamecontrol Itemvalues Mapcontrol
Npccontrol Packets Player Players Skillvalues Weddings
```

`Jukeboxcontrol.cpp`, `Doorcontrol.cpp`, `Msgboardcontrol.cpp`, `Mainform.cpp` and the
remaining `*values.cpp` units carry raw protocol/pub constants without it. Adding the
include is free (`Protocol.h` has no `Protocol.cpp` — it is a pure header, not a unit, so
it cannot alter the link layout).

---

## 4. Literals that should be named `#define`s

The project already has this convention, file-locally, in
[`src/Mysqlcontrols.cpp:15-26`](src/Mysqlcontrols.cpp) and
[`src/Serial.cpp:8-13`](src/Serial.cpp). Constants shared between units belong in
`Protocol.h` (pure header, no unit).

### 4.1 The EO number-base constants

`253`, `253²`, `253³` and the terminator byte — **42 occurrences across 17 units**, with
mixed hex case (§1.14):

```
Chestcontrol.cpp:164     Classvalues.cpp:152,154,156   Eventcontrol.cpp:45
Effectcontrol.cpp:51     Innvalues.cpp:200,202,204     Itemvalues.cpp:481,483,485
Jukeboxcontrol.cpp:40    Learnvalues.cpp:307,309,311   Mapcontrol.cpp:878,880,882,920
Msgboardcontrol.cpp:46,361,363,365             Npccontrol.cpp:225
Npcvalues.cpp:468,470,472                      Questengine.cpp:534
Packets.cpp:9497,9537,9539,9541,9669           Shopvalues.cpp:335,440,442,444
Skillvalues.cpp:325,327,329                    Weddings.cpp:405
```

**Suggested (in `Protocol.h`):**

```c
#define EO_NUM_MAX      0xfd        /* 253      — per-byte radix           */
#define EO_NUM_MAX_2    0xfa09      /* 253^2    = 64009                    */
#define EO_NUM_MAX_3    0xf71ae5    /* 253^3    = 16194277                 */
#define EO_NUM_EMPTY    0xfe        /* 254      — "no value" byte          */
#define EO_BREAK_BYTE   0xff        /* 255      — string/field separator   */
```

`0xff` alone appears **187 times**, almost all as `EO_GetBreakByte(server, 0xff)`.

### 4.2 `SECONDS_PER_DAY` — already defined, still duplicated

[`src/Mysqlcontrols.cpp:20`](src/Mysqlcontrols.cpp) has `#define SECONDS_PER_DAY 0x15180`,
but three other units open-code the same value in the same `TTimeStamp` idiom:

```
src/Banned.cpp:147        int elapsed = secs / 1000 + days * 0x15180;
src/Mapcontrol.cpp:1208   elapsed = ms / 1000 + elapsed * 0x15180;
src/Mapcontrol.cpp:1241   elapsed = ms / 1000 + elapsed * 0x15180;
src/Packets.cpp:13052     int elapsed = millis / 1000 + days * 0x15180;
```

**Suggested:** move `SECONDS_PER_DAY` (and `MS_PER_SECOND 1000`, which is open-coded in all
four) into `Protocol.h` or a small shared `Timeconst.h`.

### 4.3 Walk / packet-queue timing

```
src/Packets.cpp:140,1265,1300,2418,…   if (delay > 0x7270e0)   // 7,500,000 ms
src/Packets.cpp:141,1266,…             delay = 0x15e;          // 350 ms
src/Packets.cpp:142,1267,…             if (delay < 0x15e)
src/Packets.cpp:144,1269,…             if (player->action_queue.size() > 10)
src/Packets.cpp:106                    if (player->sequence > 9)
```

7 sites for `0x7270e0`, 10 for `0x15e`. Suggested:
`WALK_DELAY_SANITY_MS`, `WALK_MIN_DELAY_MS`, `ACTION_QUEUE_MAX`, `SEQUENCE_MAX`.

### 4.4 Party size

[`src/Player.h:197`](src/Player.h) declares `int party_ids[10];` (decimal) but every loop
over it uses hex:

```
src/Packets.cpp:9120  for (int i = 0; i < 0xa; i++)   // Server_BroadcastToPartyExceptSelf
src/Packets.cpp:9138  for (int i = 0; i < 0xa; i++)   // Server_BroadcastToParty
src/Packets.cpp:9266  for (int i = 0; i < 0xa; i++)   // FUN_00463750
```

plus the same bound inside `Party_EncodeMemberList` (`:8987`) and `Party_ShareExp`
(`:9034`, `:9049`). Suggested `#define PARTY_MAX_MEMBERS 10` and use it for both the array
and the loops.

### 4.5 Traffic-counter units

[`src/Packets.cpp:12983-13012`](src/Packets.cpp) — 8 occurrences of `0x3ff`/`0x400`:

```cpp
while (server->sent_bytes > 0x3ff) { server->sent_kilobytes++; server->sent_bytes -= 0x400; }
```

Suggested `#define BYTES_PER_KB 0x400`. (Note `0x67` = 103 in the sibling formatter at
`:13030` is a *reference quirk* — 1024/10 rounded up — keep it literal with a comment, or
name it `TRAFFIC_FRACTION_DIVISOR`; do not "correct" it to 102.)

### 4.6 Character-creation validation limits

[`src/Packets.cpp:1611-1617`](src/Packets.cpp):

```cpp
if (name.Length() > 0xc) return false;
if (gender < 0 || gender > 1 || hair_style < 1 || hair_style > 0x14 ||
    hair_color < 0 || hair_color > 9 || skin < 0 || skin > 3 ||
    name.Length() < 4)
```

Suggested: `CHARNAME_MIN_LENGTH 4`, `CHARNAME_MAX_LENGTH 0xc`, `HAIRSTYLE_MAX 0x14`,
`HAIRCOLOR_MAX 9`, `SKIN_MAX 3` (and `Gender_Female`/`Gender_Male` for the gender bounds,
§3.8).

### 4.7 Session-token ranges

Eight distinct `RandRange(...) + ...` recipes with no names
([`src/Packets.cpp:1491`, `:2256`, `:4130`, `:4364`, `:5872`, `:5975`, `:6007`, `:6575`,
`:6642`](src/Packets.cpp)):

```cpp
player->session_id    = RandRange(0xc350) + 0x2710;   // 10000 .. 60000
player->session_token = RandRange(0x2710) + 0x30d41;  // 200001 .. 210000
player->session_token = RandRange(0x2710) + 0x186a1;  // 100001 .. 110000
player->session_token = RandRange(0x2710) + 0x493e1;  // 300001 .. 310000
player->session_token = RandRange(0x2710) + 0xdbba1;  // 900001 .. 910000
player->session_token = RandRange(0x2710) + 0xc3501;  // 800001 .. 810000
```

Each base identifies a *feature* (barber, bank, guild, priest, lawyer…). Naming them
(`SESSION_BASE_BANK`, `SESSION_BASE_GUILD`, …) would make the packet handlers far easier
to follow. The `0x2710` span is shared — `SESSION_TOKEN_SPAN`.

### 4.8 Other repeated bare values worth a name

| Value | Sites | Meaning |
| --- | --- | --- |
| `0x1c20` | [`Packets.cpp:298`](src/Packets.cpp), `:647` | 7200 s = the 2-hour ban the message text advertises (`"[2hr. ban]"`) |
| `5` | `target->level < 5` at [`Packets.cpp:204`](src/Packets.cpp), `:259`, `:6615`, … | minimum level for admin-action protection |
| `10` | [`Mapcontrol.cpp:951`](src/Mapcontrol.cpp) `quest_cooldown = 10` | map quest cooldown ticks |
| `9` | [`Mapcontrol.cpp:1200`](src/Mapcontrol.cpp) `if (count > 9)` | max own ground items per tile |
| `0x4c` + `{9, 0xb}` | [`Packets.cpp:239-242`](src/Packets.cpp) | the `$f` (free-from-jail) destination map and tile |
| `100000` | [`Players.h:27`](src/Players.h) `Player *by_id[100000];`, [`Players.cpp:35`](src/Players.cpp), `:50` | socket-handle index bound, open-coded 3x |

---

## 5. Forward declarations that have real implementations

I cross-referenced every top-level prototype against every top-level definition in `src/`.
Most prototype/definition pairs are legitimate (a `.h` declaring what its `.cpp` defines).
The following are genuinely redundant or wrong.

### 5.1 Redundant with the header the same file already includes

`Packets.cpp` includes `Packets.h`, then re-declares one symbol from it:

```
src/Packets.h:119    void Server_BroadcastToParty(Server*, Player*, unsigned char, unsigned char, String);
src/Packets.cpp:43   void Server_BroadcastToParty(Server*, Player*, unsigned char, unsigned char, String);   <- delete
```

This is the *only* one of the 44 prototypes in `Packets.cpp:39-94` that duplicates
`Packets.h` — the rest declare unit-local or cross-unit symbols that `Packets.h` does not
expose (see §5.5).

`Mainform.cpp` includes `Packets.h` and `Mapcontrol.h`, then declares:

```
src/Mainform.cpp:36  void Server_RemovePlayer(Server*, TCustomWinSocket*);      <- defined in Packets.cpp:9773
src/Mainform.cpp:37  void Mapcontrol_AddArenaSpawn(Mapcontrol*, int,…);         <- defined in Mapcontrol.cpp:331
```

Neither is in the header it belongs to. These should move to `Packets.h` / `Mapcontrol.h`
rather than be re-declared locally.

### 5.2 Conflicting redeclaration of `FUN_00463d40`

```
src/Packets.cpp:72    void FUN_00463d40(Server *server, unsigned char action, unsigned char family, String data);
src/Packets.cpp:9305  void FUN_00463d40(Server *server, unsigned char action, unsigned char family, String data)  // definition
src/Packets.cpp:9369  void FUN_00463d40(Server *server, int action, int family, String data);   // <- different overload
```

Line 9369 declares a **second overload** taking `int, int`, and nothing defines it. The
very next statement is:

```cpp
// src/Packets.cpp:9372
FUN_00463d40(server, PacketAction_Close, PacketFamily_Message, "r");
```

Enum → `int` is an integral promotion and beats enum → `unsigned char` (a conversion), so
overload resolution selects the **undefined** `int, int` version.

**This is the one item in this document that is not purely cosmetic.** Deleting line 9369
changes which overload the call binds to — verify `Server_Shutdown` with `compare_asm.py`
before and after rather than assuming.

It is also the concrete example of the rule in [§0.2](#02-the-one-construct-where-renaming-can-change-codegen):
whenever a replacement enum constant is passed to a narrow parameter that has a
competing overload, the substitution is not automatically byte-neutral.

### 5.3 Duplicate prototypes inside one file

| Symbol | Declared at | Defined at | Note |
| --- | --- | --- | --- |
| `Player_FireQuestTriggers` | `Packets.cpp:54` **and** `:7458` | `:7681` | `:7458` is redundant — `:54` already covers it |
| `Player_ApplyQuestActions` | `Packets.cpp:94` **and** `:7460` | `:10846` | same |
| `Player_HandlePacket` | `Packets.cpp:49` | `:99` | 50 lines apart with no intervening use |
| `Login_SendCharacterList` | `Packets.cpp:10090` | `:11104` | needed (used at `:10300`), but the prototype and definition disagree on line wrapping only |

### 5.4 Prototypes repeated in several `.cpp` files

`RandRange` is declared **four** times and defined once
([`src/Players.cpp:23`](src/Players.cpp)):

```
src/Npc.h:7          int RandRange(int max);
src/Npcvalues.h:8    int RandRange(int max);
src/Player.h:14      int RandRange(int max);
src/Packets.cpp:40   int RandRange(int range);     <- also a different parameter name
```

It belongs in [`src/Players.h`](src/Players.h) (its defining unit), declared once.

Same pattern, less severe:

| Symbol | Declared in | Defined in |
| --- | --- | --- |
| `Walk_Execute` `Attack_Execute` `Spell_Execute` `Face_Execute` `Chair_Execute` | `Players.cpp:17-21` **and** `Packets.cpp:79-87` (and `Packets.h:104-105` for Face/Chair) | `Packets.cpp` |
| `Mapcontrol_GetCount` `Mapcontrol_GetByIndex` `MapVector_End` `Map_NpcIter_End` | `Mapcontrol.cpp:17-24` **and** `Packets.h:92-94,168` | `Packets.cpp:7219-7244` |
| `FUN_004731d0` `FUN_00473540` | `Mainform.cpp:39-40` **and** `Packets.cpp:89-90` | `Packets.cpp:13026,13036` |
| `Character_BuildSaveQuery` | `Players.h:16` **and** `Packets.cpp:7003` | `Players.cpp:915` |
| `Players_Iter_Begin/End` | `Packets.cpp:39,48` **and** `Npccontrol.cpp:16-17` | *nowhere* (§2.5) |
| `Map_GetTileSpecObject` | `Packets.cpp:7004` | `Mapcontrol.cpp:1165` |
| `Map_ReadRawFile` | `Packets.cpp:86` | `Mapcontrol.cpp:1378` |
| `MysqlCallback_Dispatch` | `Mysqlthread.cpp:13` (`extern`) | `Packets.cpp:10096` |

`Mapcontrol.cpp` and `Players.cpp` do **not** include `Packets.h`, so their local copies
are currently load-bearing — but they should be resolved by including the header rather
than by hand-copying prototypes.

### 5.5 Cross-unit symbols missing from `Packets.h`

`Packets.cpp:39-94` hand-declares 44 prototypes for symbols that live in *other* units or
later in the same file. These are the cross-unit ones that have a real definition and a
natural home header:

| Symbol | Defined in | Belongs in |
| --- | --- | --- |
| `FUN_0047c27c` `FUN_0047c3a4` `FUN_0047c3f0` `FUN_0047c428` `FUN_0047c634` `FUN_0047c6c0` `FUN_0047c890` `FUN_0047cd28` `FUN_004827c8` `FUN_004876c0` `FUN_004879b0` `FUN_00487ac0` `Map_ReadRawFile` `Map_GetTileSpecObject` | `Mapcontrol.cpp` | `Mapcontrol.h` |
| `Player_HandlePacket` `Player_CalculateStats` `Player_SerializeAvatar` `Player_SerializePaperdoll` `Server_RemovePlayer` | `Packets.cpp` | `Packets.h` |
| `FUN_004aa4e4` | `Jukeboxcontrol.cpp` | `Jukeboxcontrol.h` |
| `Mapcontrol_AddArenaSpawn` | `Mapcontrol.cpp` | `Mapcontrol.h` |

### 5.6 Orphaned definition

[`src/Packets.cpp:13013`](src/Packets.cpp) defines `FUN_00473124` — a copy of
`Coords_IsAdjacent` with the threshold `<= 2` instead of `<= 1`. It has no declaration and
no caller anywhere in `src/`. Either it is dead, or a caller is still unreconstructed;
worth a note in [PLAN.md](PLAN.md) either way.

---

## 6. Suggested order of work

Grouped so each batch can be proven independently with `compare_asm.py` / `make verify`.

| # | Batch | Files touched | Byte risk |
| --- | --- | --- | --- |
| 1 | Add the 5 missing enum values + `WarpEffect` to `Protocol.h` (§2.4) | 1 | none (nothing uses them yet) |
| 2 | Header guards (§1.10) | 6 | none (preprocessor) |
| 3 | Stale comment names (§1.4) | 3 | none (comments) |
| 4 | Hex-case normalisation (§1.14) | ~6 | none |
| 5 | `bool = 0/1` → `false/true` (§1.13) | 3 | none |
| 6 | Reply-code + action/family literals → enums (§3.1, §3.2, §3.3, §3.4, §3.5, §3.6, §3.7, §3.8) | ~5 | none *except* where a narrow-parameter overload exists — see §5.2 |
| 7 | New `#define`s (§4) | ~20 | none |
| 8 | Delete the redundant/conflicting prototypes (§5.1, §5.2, §5.3) | 3 | **§5.2 only** — verify `Server_Shutdown` |
| 9 | Move cross-unit prototypes into headers (§5.4, §5.5) | ~8 | none |
| 10 | Rename the 26 `FUN_` symbols (§2.1, §2.2) | ~6 | none |
| 11 | `void *self` → `Server *self`; `int map_control` → `Mapcontrol *` (§1.6, §2.3) | 3 | none, but wide — verify per function |
| 12 | Structural renames: `PlayerCommand` fields, `field_0xc`, `Pub_`/`Map_` prefixes, `*self`, `field_`/`pad_` scheme, `*values` members (§1.1, §1.3, §1.5, §1.7, §1.8, §1.9, §1.11, §1.12) | most | none — **but do not rename any class in the RTTI-pinned list of §0.1** |

Do **not** attempt: renaming RTTI-pinned classes (§0.1), renaming unit files, or converting
member functions between the static-with-`self` and non-static forms (§1.11) — all three
change the emitted bytes.
