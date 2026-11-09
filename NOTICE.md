# Sources and attribution

The RNG and boot reconstruction in `src/core` derives from
[LucasPicoli/god-hand-decomp](https://github.com/LucasPicoli/god-hand-decomp),
commit `eaae18d0dfab1f9a0df276ae5ca1581ad326faf3`, under the MIT license.
The upstream copyright notice and license are retained in
`src/core/LICENSE.upstream`. Native adaptations are identified in the source
and provenance notes.

`src/platform/disc.c` is an independent implementation informed by the verified
ISO9660/AFS/FST layout and that project's MIT-licensed format research.
`src/rooms/r207.c` reconstructs the small room module from the user's own input;
its evidence is generated and retained locally, separately from this repository.

The project follows the source-recovery and platform-porting approach documented
by [OpenGOAL](https://opengoal.dev/). No OpenGOAL source is included here.
Portable development dependencies are downloaded or installed separately;
their licenses remain with those distributions, which are not committed.

The project license covers project code, not the original game's assets or
trademarks. This repository contains no ISO, executable game binary, extracted
assets, BIOS, Sony SDK or keys. Running disc-dependent analysis requires the
user's own game data. The private local analysis directory is not published.
