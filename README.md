# LatticeCorrection

Feedback loop that corrects drift of the optical lattice position by stepping
the lattice Brewster-plate piezo motors. Written by Yang in 2012 (Windows,
MinGW GCC via Eclipse CDT).

## How it works

1. After each run, CameraGUI (`camera::camSaveHerald()`) measures the lattice
   position (x, y in pixels, z in microns, each with an uncertainty) and
   writes it to a herald file (`herald.txt`).
2. `LatticeCorrection` polls for that file once a second. A file that is
   still being written (no final newline yet) is left for the next poll. When
   it is complete, the program reads the new position and runs a PID-style
   update against the running average (gains and decay time in
   `src/constant.h`). A previous measurement more than a day old is ignored.
3. The correction is converted to piezo steps, clamped to a per-cycle limit
   and sent over the COM port, one channel per axis.
4. The herald file is deleted, the running average is saved (`LClog.txt`),
   and a line is appended to a drift log (`flog-YYYYMMDD-HHMMSS.txt`, named
   by the program's start time).

## Settings

Paths, the COM port and the setpoints can be set in a `parameters.txt` in the
folder the program is started from; see `parameters.example.txt`. Without
that file the defaults in `src/constant.h` are used (paths on the original lab
PC under `C:/Users/Yang/...`). The settings in use are printed at start-up.

The z channel is only used when the reported z is within 2 um of `zS`.
CameraGUI reports z as `deltaZ + Objective Position`, so `zS` has to match
the GUI's Objective Position (0 by default). When they don't match, the
program says so every cycle and corrects x and y only.

## Console commands

Type a command while the program runs and press Enter. Herald files keep
being processed while you type.

```
m <axis> <steps>   move a plate by hand, e.g. "m x -1500" (x, y or z, at most 5000 steps)
s                  show the step counters and the running average
z <axis|all>       zero step counter(s), e.g. once the plates are back near the middle
h                  help
q                  quit
```

The step counters (`LCsteps.txt`) add up every automatic and manual move and
are kept across restarts. When an axis passes `maxsteps` from zero, the
program warns that the plate may be near the rail. To recover a plate that
has hit the rail, move it back with `m`; that axis's running average then
starts afresh from the next measurement, so the old position doesn't feed
the integral term. Manual moves are also recorded in the drift log as lines
starting with `#`.

## Layout

```
src/LatticeCorrection.cpp   main loop and console commands
src/function.h              COM port, PID, log/herald/parameter file handling
src/constant.h              controller commands, gains, limits, default paths
parameters.example.txt      template for parameters.txt
```
