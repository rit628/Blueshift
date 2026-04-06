import env
from util import *
import os
from pathlib import Path
from shutil import rmtree

def build(args):
    ARTIFACT_TYPE = args.build_type.lower()
    PLATFORM = args.target
    PLATFORM_TAG = PLATFORM.split("-")[0]
    ARTIFACT_DIR = Path(env.BUILD_OUTPUT_DIRECTORY, PLATFORM, ARTIFACT_TYPE)
    COMPILE_DB_PATH = Path(env.BUILD_OUTPUT_DIRECTORY, "compile_commands.json")
    remote_args = [f"--build-type", args.build_type,
                  "--parallel", str(args.parallel),
                  "--target", args.target]
    if args.clean: remote_args.append("--clean")

    if args.local:
        if args.clean: rmtree(ARTIFACT_DIR, ignore_errors=True)

        cmake_args = [f"-DCMAKE_BUILD_TYPE={args.build_type}", "-Wno-dev"]
        if PLATFORM != get_host_os(): # specify toolchain file for cross compilation
            cmake_args.append(f"-DCMAKE_TOOLCHAIN_FILE={Path(os.getcwd(), ".cmake", f"{PLATFORM}.cmake")}")

        run_cmd(["cmake", *cmake_args, "-S", ".", "-B", ARTIFACT_DIR])
        # used for clangd and debugger source maps
        copy(Path(ARTIFACT_DIR, "compile_commands.json"), COMPILE_DB_PATH)
        with Path(ARTIFACT_DIR, ".info").open("w") as build_info:
            build_info.write(f"{os.getcwd()} {PLATFORM_TAG} {ARTIFACT_TYPE}")
        # copy info to target arch artifact dir root as substitute for symlinking previous artifact dir
        copy(Path(ARTIFACT_DIR, ".info"), ARTIFACT_DIR.parent, Path(env.BUILD_OUTPUT_DIRECTORY, ".info"))

        # 1 <= j <= n-1 (keep 1 core free for background tasks)
        core_count = str(max(1, min(args.parallel, env.NUM_CORES - 1)))
        build_args = [args.verbose, "-j", core_count]
        run_cmd(["cmake", "--build", ".", *build_args, "-t", *args.make],
                       cwd=f"./{ARTIFACT_DIR}")
    else: # build binaries in remote container
        initialize_host()
        if args.image_build: # ensure builder base image is up to date
            os.environ["PLATFORM_TAG"] = "base"
            run_cmd(["docker", "compose", "build", "builder"])
        os.environ["PLATFORM_TAG"] = PLATFORM_TAG # use correct platform for building
        run_cmd(["docker", "compose", "run", args.image_build, "--rm", "builder",
                        "build", "-l", *remote_args, *args.make])