# Network Performance & Safety Optimizations (Absolute Build)

This document outlines the specific differences between the optimized "Absolute" networking stack and the stock OpenJK/MB2 engine. These changes prioritize high-throughput stability and implement intelligent entity culling for high-population Movie Battles II servers.

## 🟢 Why the Change? (2003 vs. 2026 Logic)
The stock 2003 engine was built for a low-bandwidth era where snapshots were small. Modern MB2 environments use a massive **49,152 (48KB)** message buffer, which creates "Snapshot Bloat" that the original engine's fragment pacing cannot handle.

* **The Problem:** At high player counts (21-32), the engine's original fragment limits create a "Wait List" for data. This causes a **"Muddy"** movement feel and micro-hitching during heavy combat/death spikes.
* **The Solution:** This build transitions to **High-Burst Networking**. It ensures the network pipe is wide enough to clear a full 32-player data burst in a single millisecond while maintaining server-side isolation for duels.

---

## 1. Dedicated Server "Duel Isolation" (DuelCull)
**Function:** `SV_AddEntitiesVisibleFromPoint` & `DuelCull` (`duel_cull.cpp`)

* **Direct Pointer Passing:** Replaces internal `GetPS` table lookups with direct `playerState_t` pointer passing. This eliminates thousands of redundant lookups per second in the snapshot loop.
* **Performance Gate:** Implements `sv_snapShotDuelCull`. If disabled, the isolation logic exits in a single cycle to conserve CPU.
* **Entity Ghosting:** Culled entities are set to `solid = 0`. This reduces physics trace complexity and network overhead for dueling players.
* **NPC Persistence:** Explicitly excludes `ET_NPC` from culling to ensure training bots/dummies remain visible to everyone.

**Impact:** Significantly reduces snapshot size for the "Multiplayer Hive" by filtering data to only what is relevant to the combatants.

---

## 2. Fragment Burst & 32-Player Scaling
**Functions:** `SV_SendMessageToClient`, `SV_SendClientMessages`, `SV_SendClientGameState` (`sv_snapshot.cpp` / `sv_client.cpp`)

* **2048 Fragment Burst Cap:** The absolute ceiling is raised to **2048 fragments**. This allows the server to clear a worst-case 32-player burst (~1,216 fragments) instantly in one frame.
* **Zero-Wait Transmission:** By aligning the burst cap with the **49,152 MAX_MSGLEN**, the "Wait List" effect is eliminated. Data no longer waits for the next server heartbeat (25ms), removing the "muddy" lag.
* **Integer-Based Pacing:** Uses fixed integer logic for fragment increments to prevent "handshake drift" and the broken `cg_showSnapshot` idling climb.
* **Congestion Circuit-Breaker:** While wide enough for any legitimate MB2 spike, the 2048 cap still protects the VPS CPU from infinite-loop data floods or network exploits.

**Impact:** Provides "Crisp Sync." Movement remains light even during 32-player saber clashes, as the network valve is now sized for the modern MB2 data load.

---

## 3. Snapshot Overflow & Logging
**Function:** `SV_SendClientSnapshot` & `SV_LogPrintf` (`sv_snapshot.cpp` / `sv_main.cpp`)

* **Engine-Side Logging:** Introduces `SV_LogPrintf`, allowing the engine to write `DuelStart` and `DuelEnd` events directly to `games.log` for admin review.
* **Reliable I/O:** Employs direct file writing with `fflush` to ensure data is preserved even during rare server-side physics crashes.
* **Jitter Prevention:** Skips snapshot generation if a client has pending fragments, preventing "packet clumping" that causes players to teleport.

**Impact:** Prevents "Connection Interrupted" errors during high-data events (like multiple player deaths) and provides a reliable audit trail for tournament matches.

---

### Implementation Notes
Verified for **OpenJK/MB2** compatibility. By widening the fragment valve to **2048** and utilizing direct pointer passing for culling, the Absolute Build ensures that the network code is never the bottleneck in 32-player environments.


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
