pstool
======

For a given signal, pstool calculates a one-sided power spectrum, i.e. the portion of
a signal's power falling within given frequency bins. This is done by employing a
one-dimensional real-to-complex FFT routine on an MPI cluster.

This was written more than 10 years ago some work done in a UIC physics lab,
for https://phys.uic.edu/physics/research/research-overview/applied-laser,
although it's unclear if it ever got used by anybody.

See historical documentation in documentation.tex.

Now builds on Ubuntu Quantal and possibly something newer!
You will need:
- fftw-dev
- libopenmpi-dev or mpich2-dev, depending on what you're doing

A