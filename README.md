# `lyn`

`lyn` translates relocatable ARM ELF objects into a sequence of Event Assembler commands. `lyn` thus allows it (Event Assembler) to work as a basic but proper linker, enabling the use of standard cross-compilers in EA-based workflows.

The name of this tool is "lyn" and is not intended to be capitalized.

See the [FEU Thread](http://feuniverse.us/t/ea-asm-tool-lyn-elf2ea-if-you-will/2986?u=stanh) for a detailed walkthrough and other goodies.

**If you have any questions or encounter any issues, feel free to open an [Issue] or start a [Discussion].**

[Issue]: https://github.com/StanHash/lyn/issues
[Discussion]: https://github.com/StanHash/lyn/discussions

## Usage

```
lyn [-nohook] <elf...>
```

(parameters, including elf file references, can be arranged in any order)

- `-nohook` specifies whether automatic routine replacement hook insertion should be disabled (this happens when an object-relative symbol and an absolute symbol in two different elves have the same name, then lyn will output a "hook" to where the absolute symbol points to that will jump to the object-relative location)

Other parameters are available but they exist for historical reasons and are probably not really useful to users. (see older versions of this README if you're curious).

## Building

You need [CMake](https://cmake.org/) and a working C++20 compiler.

```sh
mkdir build
cd build
cmake ..
cmake --build .
```
