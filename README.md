mitk-docker
===========

This project provides utilities to run Docker commands from within MITK and Docker based processing views.

Features
--------

- MitkDocker Module
  - mitkDockerHelper: utility class to run docker commands
- Plugin views
  - TotalSegmentator [[https://github.com/wasserth/TotalSegmentator]]


How it works
------------

1. `Clone https://github.com/MITK/MITK` and checkout the latest release tag or at least the stable master branch
3. `Clone https://github.com/m2aia/mitk-docker`
4. Configure the MITK superbuild and set the CMake cache variable `MITK_EXTENSION_DIRS` to your working copy of the project template
5. Generate and build the MITK superbuild

Supported platforms and other requirements
------------------------------------------

- Currently only unix systems are supported
- Docker installation (https://www.docker.com/get-started/)
- User access to the 'docker' command

License
-------
See the LICENSE file for more details.
