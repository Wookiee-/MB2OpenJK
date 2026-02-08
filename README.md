# Network Performance & Safety Optimizations (Absolute Build)

This document outlines the specific differences between the optimized "Absolute" networking stack and the stock OpenJK/MB2 engine. These changes are designed to stabilize high-latency (200ms+) Player-to-Player combat and implement intelligent entity culling for high-population duel servers.

## 🟢 Why the Change? (2003 vs. 2026 Logic)
The stock 2003 engine was built for a low-bandwidth era where the main goal was simply getting data through the pipe. In modern high-ping scenarios (215ms+), the primary enemy isn't just speed—it's **jitter and packet clumping**. 

* **The Problem:** Stock logic is "passive." If the network is congested, snapshots often pile up and arrive at the client all at once, causing "teleporting" and "vibration" during close-quarters combat.
* **The Solution:** This build transitions to **"Active Pacing."** It enforces strict spacing between snapshots and caps physics processing to the server's native rhythm, ensuring that high-latency play feels like a smooth "glide" rather than a jagged stutter.

---

## 1. Delta Compression & Snapshot Pacing
**Function:** `SV_WriteSnapshotToClient` (`sv_snapshot.cpp`)

| Optimization | Stock / MB2 Logic | Absolute Optimized Logic |
| :--- | :--- | :--- |
| **Congestion Management** | Basic rate tracking. | **Refined Pacing:** Flags `rateDelayed` to explicitly prevent snapshots from bunching together. |
| **Delta Window** | Static reset thresholds. | **Stock Padding:** Uses `PACKET_BACKUP - 3` to maintain Delta mode during jitter. |
| **Snap Flag Bit** | Standard flags. | Integrates `svs.snapFlagServerBit` for improved server-side state signaling. |

**Impact:** Eliminates "Snapshot Clumping." By ensuring snapshots are correctly spaced according to the client's rate, it removes the "face-hugging" jitter seen at 200ms.

---

## 2. Dedicated Server "Duel Isolation" (DuelCull)
**Function:** `SV_AddEntitiesVisibleFromPoint` & `DuelCull` (`duel_cull.cpp`)

* **Pointer-Based Optimization:** Unlike stock logic which performs heavy lookups, the engine now passes the `playerState_t` pointer directly to `DuelCull`. This removes thousands of redundant table lookups per second.
* **Performance Gate:** Implements a high-priority "Master Switch" (`sv_snapShotDuelCull`). If disabled, the function exits immediately, ensuring zero CPU overhead for non-duel servers.
* **Entity Ghosting:** Automatically sets `state->solid = 0` for culled entities, reducing the CPU and network overhead for dueling players.
* **NPC Safety:** Explicitly excludes `ET_NPC` from culling to ensure training bots and dummies remain solid and visible for all players.

**Impact:** Massively reduces CPU "tax" and snapshot size. Dueling players gain significant FPS and network stability by only processing data relevant to their combatant.

---

## 3. Rate-Corrected Fragment Management
**Functions:** `SV_SendMessageToClient` & `SV_SendClientMessages` (`sv_snapshot.cpp`)

* **Restored Stock Rate Logic:** Removes aggressive "blast" overrides that caused ping spikes. It now correctly uses `SV_RateMsec` to calculate exactly how long the server must wait before sending again based on packet size.
* **Fragment-Aware Pacing:** If the network pipe is full (`unsentFragments`), the server calculates the wait time for the next fragment based on the client's rate rather than skipping delays.
* **Congestion Bypass:** If fragments are pending, the server avoids building new snapshots entirely, preventing the "clumping" effect that occurs when multiple snapshots are queued behind a large fragment.

**Impact:** Provides "Smooth Delivery." The server heartbeat remains synchronized with the player's actual bandwidth, preventing the massive latency spikes caused by fragment flooding.

---

## 4. Physics Heartbeat & UserMove Safety
**Function:** `SV_UserMove` (`sv_client.cpp`)

* **Time-Sync Safety:** Implements a strict `serverTime` check (`cmds[i].serverTime <= cl->lastUsercmd.serverTime`). This prevents the engine from re-processing duplicate or out-of-order packets—a common cause of "physics stutter".
* **Clean Physics Timeline:** By discarding old commands (included when `cl_packetdup > 0`), the server ensures each movement step is processed exactly once.

**Impact:** Eliminates "double-stepping" and micro-teleports. Even with high packet duplication, the server maintains a perfectly clean movement timeline for all players.

---

## 5. Snapshot Overflow & Logging
**Function:** `SV_SendClientSnapshot` & `SV_LogPrintf` (`sv_snapshot.cpp` / `sv_main.cpp`)

* **Emergency Recovery:** If a snapshot message overflows, the server triggers an emergency data reduction pass. It clears the message and re-sends only essential server commands and the delta-compressed snapshot.
* **Engine-Side Logging:** Introduces `SV_LogPrintf`, a bridge function that allows the engine to write `DuelStart` and `DuelEnd` events directly to `games.log` with high-precision engine timestamps (`mins:secs`).
* **Efficient Signaling:** Uses `MSG_WriteString` for gamedir signaling, replacing older manual byte-writing loops.

**Impact:** Prevents "Connection Interrupted" errors during heavy combat and provides reliable, timestamped logging for tournament administration.

---

### Implementation Notes
All optimizations have been verified for **OpenJK/MB2** compatibility. By leveraging direct pointer passing and enforcing strict time-syncing, the Absolute Build provides a stable, "stock-feel" experience optimized for 2026 network conditions.

# OpenJK

OpenJK is an effort by the JACoders group to maintain and improve the game engines on which the Jedi Academy (JA) and Jedi Outcast (JO) games run on, while maintaining *full backwards compatibility* with the existing games. *This project does not attempt to rebalance or otherwise modify core gameplay*.

Our aims are to:
* Improve the stability of the engine by fixing bugs and improving performance.
* Provide a clean base from which new JO and JA code modifications can be made.
* Make available this engine to more operating systems. To date, we have ports on Linux and macOS.

Currently, the most stable portion of this project is the Jedi Academy multiplayer code, with the single player code in a reasonable state.

Rough support for Jedi Outcast single player is also available, however this should be considered heavily work in progress. This is not currently actively worked on or tested. OpenJK does not have Jedi Outcast multiplayer support.

Please use discretion when making issue requests on GitHub. The [JKHub sub-forum](https://jkhub.org/forums/forum/49-openjk/) is a better place for support queries, discussions, and feature requests.

<a href="https://discord.gg/dPNCfeQ"><img src="https://img.shields.io/badge/discord-join-7289DA.svg?logo=discord&longCache=true&style=flat" /></a>
[![Forum](https://img.shields.io/badge/forum-JKHub.org%20OpenJK-brightgreen.svg)](https://jkhub.org/forums/forum/49-openjk/)

[![Coverity Scan Build Status](https://scan.coverity.com/projects/1153/badge.svg)](https://scan.coverity.com/projects/1153)

## License

OpenJK is licensed under GPLv2 as free software. You are free to use, modify and redistribute OpenJK following the terms in LICENSE.txt.


## For players

To install OpenJK, you will first need Jedi Academy installed. If you don't already own the game you can buy it from online stores such as [Steam](http://store.steampowered.com/app/6020/), [Amazon](http://www.amazon.com/Star-Wars-Jedi-Knight-Academy-Pc/dp/B0000A2MCN) or [GOG](https://www.gog.com/game/star_wars_jedi_knight_jedi_academy).

Installing and running OpenJK:

1. [Download the latest build](http://builds.openjk.org) for your operating system.
2. Extract the contents of the file into the Jedi Academy `GameData/` folder. For Steam users, this will be in `<Steam Folder>/steamapps/common/Jedi Academy/GameData`.
3. Run `openjk.x86.exe` (Windows), `openjk.i386` (Linux 32-bit), `openjk.x86_64` (Linux 64-bit) or the `OpenJK` app bundle (macOS), depending on your operating system.


**Linux Instructions**

If you do not have a windows partition and need to download the game base.

1. Download  and Install SteamCMD [SteamCMD](https://developer.valvesoftware.com/wiki/SteamCMD#Linux) .
2. Set the download path using steamCMD, force_install_dir <path> .
3. Using SteamCMD Set the platform to windows to download any windows game on steam. @sSteamCmdForcePlatformType "windows"
4. Using SteamCMD download the game,  app_update 6020.
5. [Download the latest build](http://builds.openjk.org) for your operating system.
6. Extract the contents of the file into the Jedi Academy `GameData/` folder. For Steam users, this will be in `<Steam Folder>/steamapps/common/Jedi Academy/GameData`.


**macOS Instructions**

If you have the Mac App Store Version of Jedi Academy, follow these steps to get OpenJK runnning under macOS:

1. Install [Homebrew](http://brew.sh/) if you don't have it.
2. Open the Terminal app, and enter the command `brew install sdl2`.
3. Extract the contents of the OpenJK DMG ([Download the latest build](http://builds.openjk.org)) into the game directory `/Applications/Star Wars Jedi Knight: Jedi Academy.app/Contents/`
4. Run `OpenJK.app` or `OpenJK SP.app` 
5. Savegames, Config Files and Log Files are stored in `/Users/<USER>/Library/Application Support/OpenJK/`


## For Developers


### Vulkan support

Support Initially started by porting to [Quake-III-Arena-Kenny-Edition](https://github.com/kennyalive/Quake-III-Arena-Kenny-Edition).<br />
After that, I found [vkQuake3](https://github.com/suijingfeng/vkQuake3/tree/master/code), hence the file structure.

Lastly, I stumbled across [Quake3e](https://github.com/ec-/Quake3e).<br />
Which is highly maintained, and is packed with many additions compared to the other repositories.

Therefore the vulkan renderer is now based on Quake3e. <br />A list of the additions can be found on [here](https://github.com/ec-/Quake3e#user-content-vulkan-renderer).

Many thanks to all the contributors that worked & are still working hard on improving the Q3 engine!

~Sunny

### Building OpenJK

* [Compilation guide](https://github.com/JACoders/OpenJK/wiki/Compilation-guide)
* [Debugging guide](https://github.com/JACoders/OpenJK/wiki/Debugging)


### Contributing to OpenJK

* [Fork](https://github.com/JACoders/OpenJK/fork) the project on GitHub
* Create a new branch and make your changes
* Send a [pull request](https://help.github.com/articles/creating-a-pull-request) to upstream (JACoders/OpenJK)


### Using OpenJK as a base for a new mod

* [Fork](https://github.com/JACoders/OpenJK/fork) the project on GitHub
* Change the GAMEVERSION define in codemp/game/g_local.h from "OpenJK" to your project name
* If you make a nice change, please consider back-porting to upstream via pull request as described above. This is so everyone benefits without having to reinvent the wheel for every project.


## Maintainers (in alphabetical order)

* Ensiform
* Razish
* Xycaleth


## Significant contributors (in alphabetical order)

* eezstreet
* exidl
* ImperatorPrime
* mrwonko
* redsaurus
* Scooper
* Sil
* smcv
