# Network Performance & Safety Optimizations (Absolute Build)

This document outlines the specific differences between the optimized "Absolute" networking stack and the stock OpenJK engine. These changes stabilize high-latency (200ms+) Player-to-Player combat and implement intelligent entity culling for high-population duel servers.

## 1. Delta Compression & Snapshot Pacing
**Function:** `SV_WriteSnapshotToClient` (sv_snapshot.cpp)

| Optimization | Stock Logic | Absolute Optimized Logic |
| :--- | :--- | :--- |
| **Congestion Gate** | Passive waiting for rate limits. | **Early Exit:** Aborts snapshot generation if `unsentFragments` exist. |
| **Clumping Prevention** | Simple rate tracking. | Explicitly flags `rateDelayed` to prevent snapshots from bunching together. |
| **Delta Window** | Static reset thresholds. | **Stock Padding:** Uses `PACKET_BACKUP - 3` to maintain Delta mode during jitter. |

**Impact:** Prevents "Snapshot Clumping." By returning early when the network pipe is full, it ensures data arrives in evenly-spaced packets, eliminating the "face-hugging" jitter seen at 200ms.

## 2. Dedicated Server "Duel Isolation" (DuelCull)
**Function:** `SV_AddEntitiesVisibleFromPoint` & `DuelCull` (duel_cull.cpp)

* **Isolation Logic:** Implements a specialized `DuelCull()` system to hide bystanders and irrelevant entities for players in an active duel.
* **Entity Ghosting:** Automatically sets `state->solid = 0` for culled entities, reducing the CPU and network overhead for dueling players.
* **NPC Safety:** Specifically excludes NPCs from culling to ensure training bots and dummies remain solid and visible.
* **Engine Logging:** Native tracking for `DuelStart` and `DuelEnd` events, including player names and duel results, written directly to `games.log`.

**Impact:** Significantly reduces snapshot size and prevents "warping" in servers with high player counts by filtering data to only what is relevant to the combatants.

## 3. "Absolute" Fragment & Timing Overhaul
**Functions:** `SV_SendMessageToClient` & `SV_SendClientMessages` (sv_client.cpp / sv_snapshot.cpp)

* **Fragment Priority:** Replaces stock loops with a "Blast" mechanic. It aggressively clears existing fragments before building new snapshots to ensure the pipe is empty.
* **Active Timing:** For clients in `CS_ACTIVE` state, the server skips calculated delays, treating every frame as a delivery opportunity once fragments are cleared.

**Impact:** Provides "Non-Blocking" performance. One laggy player can no longer cause "micro-stutters" for the rest of the server.

## 4. Physics Heartbeat & UserMove Smoothing
**Function:** `SV_UserMove` (sv_client.cpp)

* **Smoothing Cap:** Calculates `msec` between commands and caps it to the server's native rhythm (e.g., 25ms at 40fps).
* **Jitter Absorption:** If packets arrive in a "bunch" due to latency spikes, the server processes them as smooth steps rather than one giant teleport.

**Impact:** Eliminates the "pull-back" effect. High-ping players (up to 260ms+) appear to move smoothly on the server rather than snapping in jagged bursts.

## 5. Snapshot Overflow Recovery
**Function:** `SV_SendClientSnapshot` (sv_snapshot.cpp)

* **Data Reduction:** If a message overflows, the server triggers an emergency pass. It re-initializes the message, prioritizes reliable commands, and forces a delta compression pass to fit essential data.

**Impact:** Maintains synchronization even during extreme combat conditions, preventing the server from dropping critical movement frames.

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
