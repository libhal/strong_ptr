# libhal device library architecture

Version: 1

This document goes over the layout and architecture of a libhal device library.
libhal device libraries implement device drivers by utilizing libhal
interfaces. For example, a typical accelerometer (acceleration sensor),
communicates with a host CPU via the protocol I2C. In order for that driver to
be able to communicate over i2c it must be provided with an i2c driver
controlling the port the sensor is connected to. This is done like so:

```C++
class some_accelerometer {
  some_accelerometer(hal::i2c& p_i2c, hal::byte p_address);
};
```

## Making a new Device Driver

To make your own libhal library:

1. Press the green "Use this Template" button then press the "Create a new
   repository".
2. Name it `libhal-<insert_device_name>` and replace `<insert_device_name>` with
   the name of the device's family. For exmaple, if you want to make a library
   for the MPU series of IMUs then call it `libhal-mpu`.
3. Choose where to put the repo under,
4. Go to `settings` > `Pages` > `Build and deployment` > `Source` and set the
   source to `Github Actions`.
5. Go to `Pull Requests` and merge the library rename pull request.
6. Done!

## Layout of the Directory

### `.github/workflows/`

This directory contains GitHub Actions workflow files for continuous integration
(CI) and other automated tasks. The workflows currently included are:

- `deploy.yml`: This workflow is used for deploy released versions of the code
  to the JFrog artifactory. See the section:
- `ci.yml`: This workflow runs the CI pipeline, which includes building the
  project, running tests, and deploying the library to the `libhal-trunk`
  package repository.
- `take.yml`: This workflow is responsible for the "take" action, which assigns
  commits to
- `update_name.yml`: This workflow updates the name of the repository when it's
  used as a template for a new repository.

### `conanfile.py`

This is a [Conan](https://conan.io/) recipe file. Conan is a package manager for
C and C++ that helps manage dependencies in your project. This file defines how
Conan should build your project and its dependencies.

This conan file utilizes the
[`libhal-bootstrap`](https://github.com/libhal/libhal-bootstrap) extension
library to reduce the amount of replicated code for libhal libraries. This conanfile extends the `libhal-bootstrap.library` class. That class implements:

- `def validate(self)`
- `def layout(self)`
- `def generate(self)`
- `def build(self)`
- `def package(self)`
- `def build_requirements(self)`: which adds the tool packages for `cmake`,
  `libhal-cmake-util`, `libhal-mock`, `boost-ext-ut`

The class methods can be extended by implementing your own. The base class
implementations simply get called first. The derived class can then extend
these even further, but in general, most of these should be left untouched.

### `CMakeList.txt`

The root CMake build script for the library. It contains a call to the
[`libhal-cmake-util`](https://github.com/libhal/libhal-cmake-util) function
[`libhal_test_and_make_library()`](https://github.com/libhal/libhal-cmake-util?tab=readme-ov-file#libhal_test_and_make_library).

### `datasheets/`

This directory is intended for storing data sheets related to the device that
the library is being built for. By default this will contain a placeholder file,
`put_datasheets_here.md`. This directory is meant to be a reference for library
developers and potentially users of the library, to gain information about how
the device behaves.

Many data sheets are subject to copyright and that must be considered when
adding the datasheet to a libhal repo. If the datasheet cannot be redistributed
on the repo for copyright and/or license reasons, then a markdown file with a
link to the datasheet (and potentially mirrors of it) is an acceptable
alternative.

### `demos/`

This directory contains demonstration applications showing how to use the device
library. It includes:

- `resource_list.hpp`: A header file defining the resource list required for
  the demo applications.
- `main.cpp`: The main entry point for the demo applications.
- `platforms/lpc4074.cpp` and `platforms/lpc4078.cpp`: Platform-specific
  implementations for the demo applications.
- `CMakeLists.txt`: Build file using the
  [`libhal_build_demos`](https://github.com/libhal/libhal-cmake-util?tab=readme-ov-file#libhal_test_and_make_library)
  function from
  [`libhal-cmake-util`](https://github.com/libhal/libhal-cmake-util).

The `demos/conanfile.py` utilizes the
[`libhal-bootstrap`](https://github.com/libhal/libhal-bootstrap) extension
library and extends the `libhal-bootstrap.demos` class. This class provides the
basic building blocks for a demo. The requirements from the base class are not
transitive so:

```python
bootstrap = self.python_requires["libhal-bootstrap"]
bootstrap.module.add_demo_requirements(self)
```

Must be invoked in order to add the appropriate platform libraries to the
`ConanFile` class. These platform libraries are usually `libhal-arm-mcu`,
`libhal-linux` and `libhal-micromod`. Note that bootstrap must be updated to
support additional platforms. If you attempt to use a profile with a platform
name outside of what is supported by `libhal-bootstrap` then this API does
nothing except include `libhal-util`.

Additional requirements and dependencies can be added after calling
`add_demo_requirements`.

### `include/libhal-__device__/`

This directory contains the header files for the device library. This contains
the public APIs. Try and keep the public APIs as minimal as possible as
removing or changing something from this area will result in either an API or
ABI break.

### `src/`

This directory contains the source files for the device library. Implementation
details for the device library and any other private support libraries are
written here.

### `test_package/`

This directory contains a test package for the Conan recipe. This tests that
the Conan recipe is working correctly. The test package doesn't have to do
anything fancy. It just exists to ensure that the device library can be a
dependency of an application and successfully build. Make sure to at least
include one file from the public includes of this repo in order to determine
that your headers work. If possible, create an object or run a function in the
code to ensure that your APIs and types can be used in the package.

### `tests/`

This directory contains tests for the device library. It will always contain a `main.test.cpp` which is the entry point for the tests.

## How to Deploy Binaries for a Release

1. On Github, click on the "Releases" button
2. Press "Draft a new release"
3. For the tag drop down, provide a version. This version should be in the
   [SEMVER](https://semver.org/) format like so "1.0.0". No "v1.0.0" or "1.0.
   0v" or anything like that.
4. Press "Create New Tag" on the bottom of the drop down to create a tag for
   this release.
5. Press "Generate Notes" to add release notes to the release. Feel free to add
   additional notes you believe are useful to developers reading over these
   changes.
6. Press "Public Release"
7. The release should now exist in the "releases" section of the repo.
8. Go to Actions and on the left hand bar, press "🚀 Deploy"
9. Press "Run Workflow"
10. For the "Use workflow from" drop down, press it and select "tag" then the
    "tag" version of your release.
11. Finally press "Run Workflow" and the `deploy.yml` action will build your
    device driver for all of the available libhal platforms and deploy to the JFrog Artifactory binary repository.
