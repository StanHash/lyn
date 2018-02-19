# `lyn`

Aims to bring the functionalities of a proper linker to EA.

(My goal will be to progressively port different EA-outputting tools to object-outputting tools, as well as allowing `lyn` to output directly to binary, so that using EA will eventually become a non-requirement.)

What `lyn` does is it takes a variable number of [ELF object files](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format) plus a bunch of options, and outputs corresponding labeled EA code to stdout (I'll probably make it able to output to file at some point)

See the [FEU Thread](http://feuniverse.us/t/ea-asm-tool-lyn-elf2ea-if-you-will/2986?u=stanh) for a detailed walkthrough and other goodies.

## Usage

```
lyn [-[no]link] [-[no]longcalls] [-[no]temp] [-[no]hook] [-raw] <elf...>
```

(parameters, including elf file references, can be arranged in any order)

- `-[no]link` specifies whether anticipated linking is to occur (elves linking between themselves) (default is on)
- `-[no]longcalls` specifies whether lyn should transform any relative jump/reference to an absolute one (useful when linking code using `bl`s to symbols out of range) (default is off)
- `-[no]temp` specifies whether lyn should keep & print *all* local symbols (when off, only the ones needed for relocation are kept) (default is off)
- `-[no]hook` specifies whether automatic routine replacement hooks should be inserted (this happens when an object-relative symbol and an absolute symbol in two different elves have the same name, then lyn will output a "hook" to where the absolute symbol points to that will jump to the object-relative location) (default is on)

## Planned features

- Bugfixes (always)
- A Complete rewrite that would imply:
  - Decoupling the object interface with the event-gerenating part (and then put the object bit into a reusable library (to make writing other object-generating tools))
  - Decoupling the object interface with the elf-reading bit, allowing me to define other ways of reading objects.
- Support for more relocations (such as `ARM` relocations (`lyn` currently only supports a handful of `thumb` & `data` ones))
- Allowing handling not only only read-only allocated sections (`.text`, `.rodata`, etc) but also writable allocated sections (`.data`, `.bss`, etc). Idk how that'd work with EA tho...
- Allowing other output formats:
  - Object files (in which case `lyn`'s role would be to merge/simplify objects rather than "translating" them)
  - I'm thinking [`json-bpatch`](https://github.com/zahlman/json_bpatch) since zahl himself mentionned being interested.
  - Direct output to target binary, and allowing some kind of control over how to do that.

**As always, any question or bug report can also be done by posting in the [FEU Thread](http://feuniverse.us/t/ea-asm-tool-lyn-elf2ea-if-you-will/2986?u=stanh) (or on the [FEU Discord](http://feuniverse.us/t/feu-discord-server/1480?u=stanh) ~~whenever I'll decide to come back over there~~).**
