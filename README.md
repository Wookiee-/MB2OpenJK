### 1. `sv_snapshot.cpp` Comprehensive Optimizations

These changes stabilize high-latency connections (200ms+) by implementing deeper delta searches, proactive fragment management, and smarter overflow recovery.

* **Deep Delta Recovery Search (Function: `SV_WriteSnapshotToClient`, Line 144)**
    * **Stock:** Uses a rigid check that triggers a "Full Snapshot" if the client hasn't acknowledged a packet in ~800ms.
    * **Optimized:** Implements a search loop `for ( i = 1 ; i < PACKET_BACKUP ; i++ )` that scans the backup buffer for any valid acknowledged frame.
    * **Result:** Allows the server to find a valid delta base up to 32 frames back, preventing the massive "Full Gamestate" stutters (1-second freezes) common on high-ping connections.

* **Reliable Command Throttling (Function: `SV_UpdateServerCommandsToClient`, Line 240)**
    * **Stock:** Blindly writes all unacknowledged reliable commands until the buffer overflows.
    * **Optimized:** Added a safety check: `if ( msg->cursize > (MAX_MSGLEN - 2048) )`.
    * **Result:** It stops writing reliable commands if the packet is nearing the 16KB limit, preventing a hard overflow and ensuring the essential snapshot data can still fit.

* **Smart Overflow Management & Recovery (Function: `SV_SendClientSnapshot`, Line 661)**
    * **Stock:** Simply prints a warning and clears the message, leading to dropped frames or "Connection Interrupted" stutters.
    * **Optimized:** If a message overflows, the server clears the buffer, sets `client->rateDelayed = qtrue`, and immediately re-attempts a "stripped down" snapshot.
    * **Result:** Actively manages data bursts during heavy combat to keep the client in sync instead of failing the transmission.

* **Proactive Fragment Pumping (Function: `SV_SendClientMessages`, Line 707)**
    * **Stock:** Pending fragments must wait for the `nextSnapshotTime` timer to expire before they are sent.
    * **Optimized:** Moved the fragment check to the top of the loop: `if ( c->netchan.unsentFragments ) { SV_SendMessageToClient( NULL, c ); continue; }`.
    * **Result:** Prioritizes pending fragments, pushing them out as fast as the client's rate allows. This significantly reduces the "999" lag icon by bypassing snapshot timing gates.

* **Precision Rate-Delay Calculation (Function: `SV_RateMsec`, Line 576)**
    * **Stock:** Clamps message size to a fixed 1500 bytes for rate calculations.
    * **Optimized:** Clamps to `MAX_MSGLEN` (16384 bytes) to reflect modern snapshot sizes.
    * **Result:** Provides smoother packet intervals and prevents the server from overwhelming the client's bandwidth setting.

* **Thread-Safe Fragment Handling (Function: `SV_SendMessageToClient`, Line 603)**
    * **Stock:** Uses a `while` loop that could potentially hang the server thread if fragments are backed up.
    * **Optimized:** Replaced with an `if` block that sends a single fragment and exits, updating `nextSnapshotTime` based on `SV_RateMsec`.
    * **Result:** Ensures one lagging player cannot hang the entire server thread while processing fragments.

* **Duel Culling Support (Lines 319, 513, and 553)**
    * **Stock:** No logic to cull entities based on duel status.
    * **Optimized:** Integrated `DuelCull` checks into `SV_AddEntitiesVisibleFromPoint` and `SV_BuildClientSnapshot`.
    * **Result:** Reduces network traffic for players in duels by not sending entity data for players they cannot see or interact with.

* **Dynamic Cull Distance (Line 274)**
    * **Stock:** Uses a hardcoded or default distance for entity culling.
    * **Optimized:** Initialized `g_svCullDist = 4096.0f;`.
    * **Result:** Provides a standard, high-performance base for entity visibility checks.

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
