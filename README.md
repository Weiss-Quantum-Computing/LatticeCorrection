# LatticeCorrection

Feedback loop that corrects drift of the optical lattice position by stepping
the lattice piezo motors. Written by Yang in 2012 (Windows, MinGW GCC via
Eclipse CDT).

## How it works

1. The camera software measures the lattice position (x, y in pixels, z in
   microns, each with an uncertainty) and writes it to a herald file
   (`herald.txt`).
2. `LatticeCorrection` polls for that file once a second. When it appears, it
   reads the new position and runs a PID-style update against the previous
   measurement (gains and decay time in `src/constant.h`). A previous
   measurement more than a day old is ignored.
3. The correction is converted to piezo steps and sent over COM6, one
   channel per axis. A correction that is too large is skipped, and a warning
   is printed if the total steps on an axis exceed `maxsteps`.
4. The herald file is deleted, the measurement is saved as the new reference
   (`LClog.txt`), and a line is appended to a timestamped drift log
   (`flog-YYYYMMDD-HHMMSS.txt`).

## Layout

```
src/LatticeCorrection.cpp   main loop
src/function.h              COM port setup, PID, log/herald file handling
src/constant.h              controller commands, gains, limits, file paths
```

## Before running

File paths (`herald`, `logF`, `para`, `syslog` in `src/constant.h`, and the
drift-log path in `LatticeCorrection.cpp`) and the COM port (`COM6`) are
hard-coded for the original lab PC under `C:/Users/Yang/...`. Update them for
the current machine before building.
