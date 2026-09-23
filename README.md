# PedsEnEx

> This is a GTA:SA mod that adds ambient peds walking in and out of buildings.
<img width="1365" height="767" alt="image" src="https://github.com/user-attachments/assets/1b68b36c-cef0-44cb-bd27-7ac71b56a153" />
<img width="1365" height="767" alt="image" src="https://github.com/user-attachments/assets/1d619ec6-a883-4316-8da0-8f36beb18c13" />
***

## General Info

* This is an extended, much better, `ASI` version of my previous `CLEO` version of this mod.
* ~~Still requires manual mapping.~~
* **NO LONGER REQUIRES MANUAL MAPPING!** - The mod now comes with almost **100** locations configured by default!.

-# This was inspired by the mod `Doorway Life` by `LKBoss`.

## Why?

> I loved the idea of `LKBoss`' mod but the customizability/configuration was minimal in their version.

> Plus, I have added many features that were missing in both their mod and my previous `CLEO` version of this mod.

* Filtering by in-game time
* Filtering by model
* Ability to add/remove locations
* Cops entering interiors (if enabled) when wanted

## Note on AI

> This was one of my first, if not the very first C++ project. I had to use Gemini to learn some concepts (such as vectors) and learn to do some things (such as reading files), and also to map out certain functions from plugin-sdk.

* AI was also used to spot bugs i didn't spot.
* The core logic was developed by me, with the knowledge I'd attained from my previous CLEO attempt of this mod.

## CONFIGURATION - IMPORTANT!

> Error catching is not yet implemented so if a crash occurs, please check if all your configs are proper before reporting!
> All configuration files skip lines starting with "//", "#" or ";"

### `locations.txt`
This is the very first and most important file.
It defines all the locations where peds are to spawn. 
It has 17 fields as follows:
```
x(0)		y(1)		z(2)		r(3)	m(4)	int(5)	prob(6)	walk(7)	wt(8)	iw(9)	pid(10)	trid(11)	c(12)	cp(13)	co(14)	lp(15)	lfb(16)
```
`x`, `y`, and `z` are `floats` -> They define the coordinates of the location.
`r` is also a `float` -> This defines the radius.
> The code only spawns peds if the player is within this radius and only peds within this radius will enter the door.

`m` is an `integer` -> This is the model of the door.
`int` is an `integer` (`milliseconds`) -> This is the interval at which the ped spawning logic will be fired.
`prob` is a `float` -> This defines the probability that a ped will spawn when a spawn request is fired.
`walk` is a `float` -> This defines the distance the ped will walk to exit the door. The ped will be given the wandering AI after it moves a distance of this value towards the door.
`wt` is an `integer` (`milliseconds`) -> This is the maximum time a ped can spend walking. If it takes longer than that to walk (out or in) it will be teleported to the destination.
> NOTE: this is the **maximum** time. not a forced time. Therefore, if the ped takes only `10000 milliseconds` out of the allowed `60000 milliseconds`, it would not wait for `50000 milliseconds`. Therefore, it is recommended to give this a high value.

`iw` is a `boolean` (`1`/`0`) -> This inverts the walk destination. Set this to `1` if the ped walks towards the wall instead of walking towards the door. Otherwise, leave this at `0` (Useful when adding interior locations)
`pid` is an `integer` -> This stands for the identifier field in `pedModels.txt`. i.e. if this is set to `3`, only peds defined as 
```
3 = ...
```
in `pedModels.txt` will be spawning at this location.
`trid` is an `integer` -> This behaves similarly to `pid`, but this time with `timeRanges.txt`.
`c` is an `integer` -> This acts as a bool and a identifier field (like `pid`) enables/disables (if `0`) cops spawning when the player is wanted.
`cp` is a `float` -> This is the probability that a cop will spawn if the player is wanted. **This value is multiplied by the number of stars**.
`co` is a `boolean` (`1`/`0`) -> Set this to `1` if the location will only have cops spawning
`lp` is a `float` -> This is the probability that a ped will enter (if from outside) or leave (if from inside).
`lfb` is a `boolean` (`1`/`0`) -> This stands for leavefallback, if a ped is not found in the radius to enter/exit, if this is `0` nothing will happen but if this is `1` a ped will walk out (if from outside) or walk in (if from inside).

### `pedModels.txt`
This file defines the peds (model ids) for each `pid` in `locations.txt`
 Format:
 ```
 id = int,int,int
 ```
example:
```
1 = 120,121,122
```
and in the `pid` field of `locations.txt`, it should be `1`
ID `-1` is the fallback field. If an entry with the ID defined in `locations.txt` isn't found, this will be used.
### `copModels.txt`
This file defines the cops (model ids) for each `cop` in `locations.txt`
 Format:
 ```
 id = int,int,int
 ```
example:
```
1 = 120,121,122
```
and in the `cop` field of `locations.txt`, it should be `1`

ID `-1` is the fallback field. If an entry with the `cop` ID defined in `locations.txt` isn't found, this will be used.
> Do **NOT** use id `0`. (Setting the `c` field to `0` in `locations.txt` is the hardcoded switch to completely disable cops for that location).
### `timeRanges.txt`
This file defines the time ranges for each `trid` defined in `locations.txt`
ID `0` is the time range for cops
This uses the **24 hour time format**.
Format:
```
id = timeStart-timeEnd,timeStart-timeEnd
```
example:
```
1 = 08:00-09:00
```
and in the `trid` field of `locations.txt`, it should be `1`

> `timeStart` **MUST** be a value lower than `timeEnd` for it to work.
---
> Those are all the files that you need to know about to configure the mod. The mod comes by default with 3 locations. (See SS)
---
## CREDITS
- `LKBoss` - inspired by the mod `Doorway Life` by them - Nothing from their script was used in this mod. I made this from the ground up. I just got the idea from this mod.
- `rill_` - for the door unlocking opcode (https://libertycity.net/files/gta-san-andreas/221665-doorway-life.html)
- `mirandmc` - initial help regarding opcodes
- `sebicu` - offering help
- `black.greyed_61525` - offering help
- `caio_scorpion23` - better peds config
- `plugin-sdk` by `DK22Pac` - Used to write and compile the script (https://github.com/DK22Pac/plugin-sdk/)
- `Sanny Builder Library` - Used to find opcodes (https://library.sannybuilder.com/#/sa/script) (see below)
- `Google Gemini` - Spotting bugs and some help with the code (see above), Location Editor file (check source code).
- `vecondite` - Me. Wrote the code :D

> Please let me know if I missed any credits.
---
## SUPPORT
If you need any help with the mod, you can contact me as follows.
`Discord` -> `vecondite`
Bug reports, contributions, etc are very much appreciated and welcome.
## Worth mentioning
The mod still uses some `CLEO` opcodes for some actions (such as defining task sequences for peds walking and door unlocking with `0905`). If you know the `plugin-sdk` alternatives for those actions, please let me know.

---
### The code for this mod is fully open-source under the MIT License. Feel free to edit and redistribute, provided you include the original license and copyright notice.

Have fun! ❤️
