# CamSim

## Overview

This is a camera simulation library developed at the 
[Computer Graphics Group, University of Siegen](http://www.cg.informatik.uni-siegen.de).

Supported cameras types:

- RGB cameras, optionally physically plausible, with OpenGL-style materials and
  lighting

- Amplitude-modulated continuous-wave Time-of-Flight cameras (AMCW ToF, e.g. PMD cameras),
  with 4 phase images, physically plausible, reusing OpenGL-style materials and lighting
  as far as it makes sense

Features:

- Simulation:
  - light-transport based information (RGB, PMD phases and result)
  - geometry information (positions, normals, depth and range, object/shape/
    triangle indices)
  - temporal flow information (2D and 3D flow)

- Scene:
  - Arbitrary number of light sources (point lights, spot lights, and
    directional lights)
  - Light sources with measured intensity distribution and structured-light
    projecting light sources
  - Materials with
    - ambient, diffuse, specular, gloss, emission and lightness maps
    - transparency (but not translucency)
    - bump or normal maps
    - BRDF-based lighting (in the simplest case using the modified Phong model)
  - Builtin functions to create scenes using simple geometries (quads, cubes,
    cylinders, cones, spheres, tori, teapots)

- Animation:
  - Simple linear animations of position, orientation, and scale
  - For camera, light sources, and objects

- Oversampling:
  - Spatial, with custom weights (especially useful for PMD simulation)
  - Temporal (for correct motion artefacts in light-transport based
    simulations)

- Noise and Effects:
  - Gaussian white noise
  - Lens distortion compatible to OpenCV (radial and tangential)
  - Thins lens vignetting effect

- Import and Export:
  - Import of arbitrary models via ASSIMP
  - Export to RAW, CSV, PNM, PNG, PFS, GTA, MAT, HDF5

- Other:
  - Multi-GPU support
  - Multicore CPU support for export (data export is typically the bottleneck)


## Requirements

Only [Qt](https://www.qt.io/) is required, nothing else. You can *optionally*
use external libraries to extent functionality:
- [libassimp](http://www.assimp.org/): import arbitrary 3D models
- [libmatio](https://sourceforge.net/projects/matio/): export to Matlab .mat files
- [libhdf5](https://support.hdfgroup.org/HDF5/): export to .h5 files
- [libgta](https://marlam.de/gta/): export to .gta files

First build *and install* libcamsim. Then compile and link your application
against the *installed* version of libcamsim (not against the files in its
build directories).


## Usage

Libcamsim requires an OpenGL 4.5 core context. You should typically create
such a context using an offscreen surface (see the examples).

There is Doxygen-style documentation (build with `CAMSIM_BUILD_DOCUMENTATION=ON`
to generate HTML), but the best starting point is probably to look at the
example programs.

If you want to display results in an interactive application, create another
context that shares OpenGL objects with the libcamsim context, so that you can
render the textures that libcamsim produces. If you do this, you might need to
call glFinish() on the libcamsim context after calling simulate() to make sure
the result textures are finished before reusing them in another OpenGL
context, since there is no implicit synchronization between contexts.

If you want to use multiple GPUs, create offscreen contexts for each of them
(this step is system dependent). Contexts on different GPUs cannot share
objects, so each of these contexts must get its own scene and simulator
instances. For example, if you import a model file, you must add its contents
to all scene instances.

Note that the bottleneck is almost always the export, not the simulation
itself! Export data without compression, and use a write-efficient file
format! For example, simulating and exporting 125 floating point RGB frames of
size 800x600 took the following time in seconds for different file formats,
without compression: 1.5 (raw), 1.5 (gta), 4.9 (ppm), 7.0 (pfs), 7.0 (mat),
7.2 (png), 7.2 (h5), 118 (csv). Most of the time, it is a good idea to export
.raw or .gta and postprocess / convert these results afterwards.


## Relevant Papers

The following papers are relevant for CamSim. If you use this software, please
cite the appropriate paper, depending on the features you are using:

- General simulation model, including AMCW ToF illumination and sensor:
  M. Lambers, S. Hoberg, A. Kolb:
  [Simulation of Time-of-Flight Sensors for Evaluation of Chip Layout Variants](http://ieeexplore.ieee.org/xpl/articleDetails.jsp?reload=true&arnumber=7054461).
  In IEEE Sensors Journal, 15(7), 2015, pages 4019-4026.

- Extension of the illumination model to single-bounce indirect illumination via
  reflective shadow maps:
  D. Bulczak, M. Lambers, A. Kolb:
  [Quantified, Interactive Simulation of AMCW ToF Camera Including Multipath Effects](http://doi.org/10.3390/s18010013).
  In MDPI Sensors, 18(1), 2018, pages 1424-8220.

- Lens distortion simulation:
  M. Lambers, H. Sommerhoff, A. Kolb: [Realistic Lens Distortion Rendering](http://wscg.zcu.cz/wscg2018/2018-WSCG-Papers-Separated.html).
  In Proc. Int. Conf. in Central Europe on Computer Graphics, Visualization and
  Computer Vision (WSCG), 2018.
