# `lyn`

`lyn` is being rewritten. The rewritten lyn will be lyn 3.x.

`lyn` translates relocatable ARM ELF objects into a sequence of Event Assembler commands, allowing it to work as a basic but proper linker. `lyn` output is currently tailored for ColorzCore.

The name of this tool is "lyn" and is not intended to be capitalized.

## Planned features for 3.x

### Returning

- basic section outputting (symbols, data, relocations).
- auto-generating hooks for jumping to replacement functions.
- the ability to transform relative calls (BLs) into long calls via generated veneers.

### New

- output arbitrary sections to fixed addresses using directives encoded as section name suffixes
- explicit replacement sections and auto_replace sections.
- basic RAM section support, both fixed address and floating (enabled by some EA trickery devised by Mokha). Such section can only contain local (=static) labels because of EA limitations.
- emission of PROTECT and ASSERT in the output event to enable stronger error cathing.

### Deprecated

- auto-replacement being enabled by default. Redefinition of functions symbols with the same name as ones from the reference outside of auto_replace sections will still work but lyn will raise a warning. If this behavior is desired, a new commandline option `--always-allow-replace` can be used to opt-in (doing so will be required in next 4.x+).
- automatic detection of the reference object. One should pass it explicitement via the `--reference` option.

### Removed

- most old commandline options and their associated features.
  - `-[no]hook` and `-[no]longcall` are kept as backward compatible aliases, but will have lyn raise warnings.
- `lyn diff`

## Planned interface

```
lyn [OPTIONS] ELVES...
Options:
  -o OUTPUT,--output OUTPUT   Specifies the output file. Defaults to outputting to stdout.
  -r ELF,--reference ELF      Specifies the reference ELF.
  --always-allow-replace      Treat functions that have the same name as one from the reference as being part of a auto_replace section.
  --extend-calls              Transform every BL relocation to undefined symbols into a call to a absolute jump.
  --to-stdout                 Enables inctext-compatibility mode, which disables some features.
  --strict                    Disables deprecated features.
  -h,--help                   Display basic help.
  -hook                       Deprecated, alias of --always-allow-replace
  -longcalls                  Deprecated, alias of --extend-calls
  -nohook,-nolongcalls        Deprecated, ignored
```
