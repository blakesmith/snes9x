# Snes9x
*Snes9x - Portable Super Nintendo Entertainment System (TM) emulator*

This is the official source code repository for the Snes9x project.

Please check the [Wiki](https://github.com/snes9xgit/snes9x/wiki) for additional information.

# RG350 build

You'll need to setup your RG350
buildroot. Checkout
[this GitHub repository](https://github.com/tonyjih/RG350_buildroot)
and follow the setup instructions to build a local toolchain. This
will build the necessary MIPS compiler toolchain, as well as compile
the shared libraries that are present on the system for linking (in
rombp's case, we just link against uclibc and SDL2).

Once you setup the toolchain, you should be able to build:

```
export RG350_TOOLCHAIN=/some/path/to/toolchain
mkdir build
cmake -B build/ -DTOOLCHAIN_PREFIX=$RG350_TOOLCHAIN/output/host/usr -DMODEL=gcw0
cd build/
make
```

Once that is done, you can build an OPK that will run on the device:

```
cd opk
./make_opk.sh
```
