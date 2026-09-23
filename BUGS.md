# Security & Correctness Findings — GameServer.exe reconstruction

**Reviewed:** 2026-09-23 · **Scope:** `src/` (~30k lines, 129 files)
**Focus:** `Packets.cpp` (all remote input), `Mysqlcontrols.cpp`, `Players.cpp`, `Banned.cpp`, `Serial.cpp`

> **Byte-exactness caveat.** This is a byte-exact reconstruction of a 2012 binary, so every
> finding below is presumably a faithful reproduction of a bug in the original `GameServer.exe`.
> **Fixing any of them breaks the MD5 goal.** Treat this as "what an attacker could do against a
> server running this binary", not a patch list for `master` — unless you fork a `playable` branch.

Line numbers refer to the tree as of commit `2ed312a`.

---

## Critical

### 1. SQL injection via guild member name (unsanitized packet bytes)

`src/Packets.cpp:5623`, `:7711`, `:7777`

`Guild/Kick` reads `member_name = data.SubString(5, data.Length() - 4)` — raw decoded packet
bytes, no `Db_SanitizeString` — and splices it into a string literal:

```cpp
"SELECT ident_guild, ident_rank FROM endl_characters WHERE name = '" +
    member_name + "' AND ident_guild = '" + player->guild_tag + "' LIMIT 1"
```

Same for `Guild/Rank` → callback `0x47` (`char_name`, `:7711`) and callback `0x48`, which builds
an **UPDATE** (`:7777`). Nothing in the decode path filters `'`: `Player_HandlePacket`'s 0x80
transform maps bytes into `[0x01,0x7F] ∪ [0x80,0xFF]`, so `0x27` is freely reachable.

**Exploit path:** found a guild (any player can), become leader (`guild_rank_id == 1`), send
`Guild/Kick` naming an *offline* character — the offline branch is the one that queries. Full
statement control against `endl_characters` under the `endless_acc` account.

### 2. Second-order SQL injection via HDID

`src/Packets.cpp:1112`

```cpp
"INSERT INTO endl_banlist (...) VALUES (NOW(),1,'" +
    player->name + "','" + target->remote_ip + "','" + target->hdid + "','perm-ban by HGM'"
```

`hdid` is taken verbatim from the client's init packet (`:9482`, `data.SubString(11, ...)`,
~32 bytes, only length-checked). A player primes a malicious HDID, then gets themselves
perm-banned by a GM to fire the payload.

*(This statement is also syntactically broken — missing closing `)` — so the ban-list insert
never succeeds today.)*

### 3. Ban evasion at the handshake

`src/Packets.cpp:9473`

```cpp
if (data.Length() > 8 && data.Length() < 0x2a) {
    if (...0xff && 0xff...) {
        if (data.Length() > 10) {      // ← ban check lives ONLY here
            ... Banned::IsBanned(...) ...
        }
        // version check, then accept
```

Send a **9- or 10-byte** init packet and both the HDID *and* IP ban checks are skipped entirely.
`data[9]` (the `0x70` magic) is still readable at length 9, so the connection proceeds to
`Server_BuildInitOkReply`. Every in-game ban is bypassable by one byte of packet trimming.

### 4. Out-of-bounds array read → arbitrary pointer deref (trade)

`src/Packets.cpp:4898`, `:4946`, `:5172`

```cpp
Player *target = server->players->by_id[player->trade_partner_id];
```

`trade_partner_id` is initialized to **-1** (`src/Player.cpp:57`) and reset to -1 on every trade
close/warp. `by_id` is `Player*[100000]` at offset `+0x20` of `Players`, so `by_id[-1]` reads the
last 4 bytes of the preceding `vector<Player*>`. The result is then dereferenced
(`target->trade_items.clear()`, `target->map_id`) after only a `== NULL` test.

`Trade/Remove`, `Trade/Agree` and `Trade/Close` are all reachable immediately after login with no
trade open. `Players_GetById` (`src/Players.cpp:1026`) *does* bounds-check — these call sites
bypass it.

### 5. Unbounded vector index in `Mapcontrol_ReadRawFile`

`src/Mapcontrol.cpp` (`Mapcontrol_ReadRawFile`), reached from `src/Packets.cpp:2655`

Every other `MapContainer` accessor guards with `map_id > 0 && map_id <= maps.size()`. This one
does not:

```cpp
if (self->maps[map_id - 1].buf == "") { ... }
```

`Welcome/Agree` code 1 passes a **fully attacker-controlled 2-byte map id** (0–64008) straight in.
Reads an `AnsiString` from far outside the vector and compares it — wild pointer deref. Reliable
remote crash, plausible info leak.

### 6. Bounds check in `Player_Warp` is unsatisfiable

`src/Packets.cpp:9937`

```cpp
if (target_map < 1 && (int)server->map_control->maps.size() < target_map)
    return;
if (server->map_control->maps[target_map - 1].width < 1 || ...)
```

`&&` where `||` is required — the guard can never be true (a value cannot be both `< 1` and
`> size`). The next line indexes `maps[target_map - 1]` unconditionally. Any warp tile, quest
action, scroll, or inn spawn pointing at a nonexistent map id crashes the server. `Warp/Accept`
(`:3600`) and `Attack_Execute` (`:10946`) then index `maps[map_id - 1]` again with no check of
their own.

---

## High

### 7. Password and credential handling

- `Account_EncodePassword` / `Account_DecodePassword` (`src/Packets.cpp:12494`, `:12540`) are
  **the same function** — a reversible involution (reverse the string, complement digits and
  lowercase letters). Not a hash.
- Stored via MySQL `ENCODE(..., 'eoeokeyendl')` — a broken stream cipher with a key hardcoded in
  ~8 places. DB read access ⇒ every plaintext password.
- `Db_SanitizeString` runs on the password *before* comparison and storage: it **lowercases** it
  and deletes ``' % [ * ] ( , ) & { # } " : ; = / \`` and two high bytes. Effective password
  alphabet is roughly `[a-z0-9]` plus a handful of symbols — a large keyspace reduction users
  never consented to.
- DB credentials `endless_acc` / `xitz9ak4` are embedded in the binary under the same trivial
  cipher (`src/Mysqlcontrols.cpp:14-17`).

### 8. Cross-service session-token confusion

One `player->session_token` field authorizes Bank, Shop, Skillmaster, Inn, Guild, Priest, Lawyer,
Barber and Quest. Some handlers range-check it, most don't:

| Setter | Value |
|---|---|
| `Shop/Open` (`:4202`) | `type_info.behavior_id` — small |
| `StatSkill/Open` (`:6726`) | `type_info.behavior_id` — small |
| `Citizen/Open` (`:4667`) | `behavior_id - 1` — small |
| `Quest/Accept` (`:6352`) | `RandRange(0x2710)` → 0–9999 |
| Bank / Guild / Priest / Lawyer | large, range-checked at use |

`StatSkill/Take`, `StatSkill/Junk`, `StatSkill/Remove`, `Shop/Buy`, `Shop/Sell`, `Shop/Create` and
`Bank/Take` compare `session_token` but never range-check it. So: open any shop NPC with behavior
id *N*, then send `StatSkill/Take` with token *N* to **learn any skill from skillmaster record
*N*** without ever visiting that skillmaster — or `StatSkill/Junk` to reset stats anywhere.
`Bank/Add` has the range check (`:4694`); `Bank/Take` (`:4713`) is missing it.

### 9. Message board: delete any post

`src/Packets.cpp:4390`

```cpp
int board = ...; int post_id = ...;
MsgBoardController::DeletePost(GUI->msgboard_control, board, post_id);
```

No ownership check, no admin check, no rate limit. Any logged-in player can enumerate and delete
every post on all 8 boards. (`DeletePost` itself bounds-checks the board index, so it's an
authorization hole rather than a memory one.)

### 10. Missing authentication checks

- **`PacketFamily_Message`** (`:7228`) — the *entire family* lacks a `logged_in` check. An
  unauthenticated socket can pull server stats, the top-player table, the top-guild table, and
  `Message_BuildServerStatus` (every online player's name, title, level, exp, gender, admin
  level). Also a cheap pre-auth amplification vector: 1-byte request → multi-KB response.
- **`StatSkill/Add`** (`:6738`) — the only in-game handler in the file missing
  `if (!player->logged_in) return false;`. Impact is limited because a pre-login `Player` has
  `stat_points == 0`, but it's a real gap in an otherwise uniform pattern.

### 11. Bank overflow → gold duplication

`src/Packets.cpp:4702`, `:4713`; `money_bank` is `int` (`src/Player.h:116`)

```cpp
if (player->money_bank > 100000000000) return true;   // dead code: int can't exceed 2^31
player->money_bank = player->money_bank + amount;      // unchecked overflow
...
if (player->money_bank < amount) return true;          // int vs unsigned int → int promoted
```

The guard constant exceeds `INT_MAX` so it never fires. Once `money_bank` goes negative, the
withdrawal comparison promotes it to ~4×10⁹ and passes, so you can withdraw up to `UINT_MAX` gold
per call. Bootstrapping needs ~2.1×10⁹ deposited gold, which #12 supplies.

### 12. No cap on item stacks; signed/unsigned mismatch in the checks

`src/Players.cpp:646` (`Player_AddItem`) does `iter->amount = iter->amount + amount` with no
ceiling and no overflow test. Every consumer then compares with unsigned casts against the signed
field:

```cpp
// Player_RemoveItem, :675
if ((unsigned int)amount > (unsigned int)iter->amount) return false;
// Trade/Add, :4845
if ((unsigned int)amount > (unsigned int)have || (unsigned int)amount < 1) return true;
```

A negative (overflowed) stack reads as ~4 billion, so the "do you have enough?" test passes for
any quantity — and `Player_AddItem` credits the *partner* with a real positive stack. That's a
working dupe once you can overflow one stack.

### 13. Predictable randomness

`Random_Seed()` is `srand(time(0))` (`src/Packets.cpp:12476`) and is called only from
`Connection_Ping`. `RandRange` is `_lrand() % max` (`src/Players.cpp:1087`) — modulo bias, and
seeded from wall-clock seconds. Everything security-relevant derives from it: `session_id`,
`session_token`, `client_encryption_multiple`, `server_encryption_multiple`, `query_id`. The
`ping_history` anti-replay value is server-global and broadcast to every client, so it isn't a
per-client secret either.

---

## Medium

| # | Finding | Location |
|---|---|---|
| 14 | `Client_SendEncoded` logs `"Too large encoded packet dropped"` at >20000 bytes but **does not return** — it proceeds into `EO_Encode_Interleave`, which writes unbounded into `packet_buffer[65000]`. Same non-drop bug in `Client_SendRaw` at 62000. | `:9858`, `:9571` |
| 15 | `by_id[socket->SocketHandle]` unchecked in `Server_ClientRead` / `Server_RemovePlayer`, though `Players_Add` correctly rejects handles ≥ 100000. A rejected large-handle socket still indexes past the array. | `:394`, `:309` |
| 16 | The in-game `AddBan(…, char ban_type, …)` overload never parses the 4th IP octet, leaving it `0x100` (wildcard) — so **every** kick/ban is effectively a `/24` subnet ban. Collateral bans of unrelated players. | `src/Banned.cpp:78` |
| 17 | `Shop/Sell` calls `Player_RemoveItem` **before** validating `price < 0`. Sell an item the shop doesn't buy → item destroyed, no gold. | `:4165` |
| 18 | Hardcoded name backdoor: `$d` lets characters named `vult-r` or `arglon` broadcast arbitrary chat **attributed to any other player**. Keyed on character name only. Related: `Logins` hardcodes reserved names `vult-r`, `aengie`, `angel`. | `:825`, `src/Logins.cpp:13` |
| 19 | `Logins::AddLogin` appends to an unbounded `TList` on every malformed packet / failed init, and `HandleAddress` scans it linearly per connection. Memory + CPU DoS from a connection flood. | `src/Logins.cpp:84` |
| 20 | `Talk/Tell` (whisper) has no rate limiting, unlike say (`say_chat_tokens`) and world chat (`world_chat_tokens`). Unlimited targeted spam. | `:1475` |
| 21 | `Client_SendEncoded` does `std::basic_string<char> range(out.c_str())` **before** the 0x80 transform, so a `0x00` byte anywhere in a player-supplied string (chat under 60 chars skips `NormalizePlayerText` entirely) silently truncates the outgoing packet for every recipient. Protocol-desync griefing. | `:9886` |
| 22 | `skillblob.Delete(256, invblob.Length() - 255)` — wrong variable. Skill data is truncated by the *inventory* length, corrupting saved spells. | `src/Players.cpp:349` |
| 23 | `ItemValues::GetByIndex`: `(unsigned)index > self->record_list.size() - 1` underflows to `SIZE_MAX` when the list is empty, so any index passes. | `src/Itemvalues.cpp:390` |
| 24 | `ItemValues::GetType` uses `item_id < GetCount()` where `<=` is correct — the last item in the pub file is permanently unusable. | `src/Itemvalues.cpp:372` |
| 25 | Item pickup (`Item/Get`), chest take, and locker take call `Player_AddItem` with **no weight and no inventory-size enforcement** — `weight_current` is only clamped for display. Unlimited carry. | `:3444`, `:3857` |
| 26 | Inline `hp * 100 / max_hp` in `Item/Use` (heal) and `Spell_Execute` divide without the `max_hp < 1` guard that `Player::HpPercent` has. | `:3236`, `:11578` |
| 27 | Marriage engagement stores `partner_name = name.SubString(1, 3)` (3 chars), but divorce requires `partner_name.Length() >= 4`. Pay 500 gold to engage, then you can never divorce — the gold is unrecoverable. | `:6859` |

---

## Systemic observations

**`Db_SanitizeString` is a blacklist, not an escape.** It deletes ~19 characters and lowercases.
It's applied inconsistently — roughly 30 of the 38 query sites in `Packets.cpp` use it, and the
ones that don't (#1) are the exploitable ones. There is no parameterized-query path anywhere;
every statement is built by string concatenation and handed to `TQuery::SQL->Add`.

**The `Character_BuildSaveQuery` path is only accidentally safe.** `title`, `partner_name`,
`guild_tag`, `guild_name` and `guild_rank_name` all go into an UPDATE inside quotes with no
escaping (`src/Players.cpp:358-364`). They happen to be sanitized at their assignment sites
(`:2281-2289` on load, `Mysql_SanitizeString` on guild ops), so it holds — but it's one missed
sanitizer away from a persistent injection.

**Defence-in-depth gaps.** `catch (...) {}` wraps large blocks throughout, swallowing the
exceptions that would otherwise surface the OOB accesses above. There is no ASLR/DEP-era
hardening (2012 Borland, static VCL), no per-connection packet rate limiting beyond the 500-byte
receive buffer, and `player->sequence` desync causes packets to be silently ignored
(`return true`) rather than rejected.

---

## Suggested fix order (playable fork only)

1. **#3** — one-line length check at `Server_HandleInit`
2. **#6** — `&&` → `||` in `Player_Warp`
3. **#5** — add the bounds check the sibling `MapContainer` accessors already have
4. **#4** — route the three trade sites through `Players_GetById`
5. **#1 / #2** — sanitize `member_name`, `char_name`, `hdid`
