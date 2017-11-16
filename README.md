# lyn
Aims to bring the functionalities of a proper linker to EA.

What `lyn` does is it takes a variable number of [ELF object files](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format) plus a bunch of options, and outputs corresponding labeled EA code to stdout (I'll probably make it able to output to file at some point)

See the [FEU Thread](http://feuniverse.us/t/ea-asm-tool-lyn-elf2ea-if-you-will/2986?u=stanh) for a detailed walkthrough and other goodies.

## Usage
```
lyn [-nolink/-linkabs/-linkall] [-longcalls] [-raw] [-printtemp] [-autohook/-nohook] <elf...>
```

(parameters, including elf file references, can be arranged in any order)

- `-nolink`/`-linkabs`/`-linkall` defines linking policy. Behavior may conflict when other options are set (other options usually have priority).
    - `-nolink` prevents any anticipated linking to occur (all relocations will be exposed to EA)
    - `-linkabs` will only link absolute symbols (ie symbols that cannot simply be represented as labels using EA)
    - `-linkall` will link everything it can (absolutes, relatives, locals...) (set by default)
- `-longcalls` will transform any direct BL call (and maybe later other calls/jumps) to a call to a "trampoline", where the target symbol will be represented as absolute.
- `-raw` will prevent any anticipated computation to occur, exposing the elves as is.
- `-printtemp` will expose temporary symbols (for now this only includes labels associated with trampolines)
- `-autohook` will allow automatic hooking into vanilla routines (see above for details). On by default (disable it with `-nohook` or `-raw`)

## Planned features

- Bugfixes (always)
- Code cleanup (kinda want to have my mini `elf` library to be a bit more clean and such)
- Support for more relocations (such as `ARM` relocations (`lyn` currently only supports a handful of `thumb` & `data` ones))
- Maybe support for output formats? I'm thinking [`json-bpatch`](https://github.com/zahlman/json_bpatch) since zahl himself mentionned being interested, or maybe also the ability to write directly to target binary? (Some would say heresy, but maybe then we'll finally have a proper linker that is fit for our purposes?)
- Allowing handling not only only read-only allocated sections (`.text`, `.rodata`, etc) but also writable allocated sections (`.data`, `.bss`, etc). Idk how that'd work with EA tho.

**As always, any question or bug report can also be done by contacting me through the [FEU Thread](http://feuniverse.us/t/ea-asm-tool-lyn-elf2ea-if-you-will/2986?u=stanh) or on the [FEU Discord](http://feuniverse.us/t/feu-discord-server/1480?u=stanh).**
