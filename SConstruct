#!/usr/bin/env python
import glob

env = SConscript("godot-cpp/SConstruct")

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

outpath = "bin"


def add_source_files(self, sources, files, allow_gen=False):
    # Convert string to list of absolute paths (including expanding wildcard)
    if isinstance(files, (str, bytes)):
        # Keep SCons project-absolute path as they are (no wildcard support)
        if files.startswith("#"):
            if "*" in files:
                print("ERROR: Wildcards can't be expanded in SCons project-absolute path: '{}'".format(files))
                return
            files = [files]
        else:
            # Exclude .gen.cpp files from globbing, to avoid including obsolete ones.
            # They should instead be added manually.
            skip_gen_cpp = "*" in files
            dir_path = self.Dir(".").abspath
            files = sorted(glob.glob(dir_path + "/" + files))
            if skip_gen_cpp and not allow_gen:
                files = [f for f in files if not f.endswith(".gen.cpp")]

    # Add each path as compiled SharedObject following environment (self) configuration
    for path in files:
        obj = self.SharedObject(path)
        if obj in sources:
            print('WARNING: SharedObject "{}" already included in environment sources.'.format(obj))
            continue
        sources.append(obj)


def _register_library(name, path):
    env.Append(CPPPATH=["#{}/".format(path)])
    env.library_sources = []

    SConscript("{}/SCsub".format(path))

    if env["platform"] == "macos":
        library = env.SharedLibrary(
            "{}/{}.{}.{}.framework/{}.{}.{}".format(
                outpath, name, env["platform"], env["target"], name, env["platform"], env["target"]
            ),
            source=env.library_sources,
        )
    else:
        library = env.SharedLibrary(
            "{}/{}{}{}".format(outpath, name, env["suffix"], env["SHLIBSUFFIX"]),
            source=env.library_sources,
        )

    Default(library)
    return library

env.__class__.add_source_files = add_source_files
Export("env")

library_gdsion = _register_library("libgdsion", "src")

# Copy the build results into the example project.
install_artifacts = env.Install("example", "bin")
Default(install_artifacts)

env.Depends(install_artifacts, library_gdsion)
