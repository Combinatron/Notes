# Combinatron

A definition, virtual implementation, and hardware implementation of an
experimental combinator reduction machine. Inspired in part by the Reduceron

## Definition (Stable)

See (semantics2)[semantics2] for the current definition and operational
semantics of the Combinatron. A more accessible version is needed, but this
suffices for now.

## Virtual Implementation (WIP)

See (emu.hs)[emu.hs] for a virtual implementation of the machine in Haskell.
Ignore emu.c for now, it was mostly useful as an exploratory exercise and for
figuring out memory layouts.

The goal for right now is to have a high-level implementation in Haskell to
prove out the semantics. If necessary, a low-level implementation will be
written in C to explore the low-level details.

## Hardware Implementation (Not Started)

Eventually I'm going to work on a hardware implementation of the machine in an
FPGA chip. Nothing to report here yet.

## Software

This is more than just a hardware project. I'm very excited to get the hardware
working and then start on some comprehensive pieces of software.

### Compiler (Not Started)

Eventually a compiler to the machine language used in the Combinatron will need
to be written.

### OS (Not Started)

I'm very interested in what an operating system might look like on this
hardware. Part of the allure of graph reduction is that it is inherently
parallel. I'm excited to see how I can take advantage of that in an OS. One
potential idea is to have the OS distribute and manage it's running across many
Combinatron cores.
