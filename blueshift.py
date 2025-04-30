import argparse
import functools
import os
import sys
import subprocess
import socket
import re
import atexit
import time
import multiprocessing as mp
from dotenv import load_dotenv
from pathlib import Path
from threading import Thread
from shutil import rmtree

load_dotenv()
PROJECT_NAME = os.getenv("PROJECT_NAME")
PROJECT_PREFIX = os.getenv("PROJECT_PREFIX")
NETWORK_NAME = os.getenv("NETWORK_NAME")
NUM_CLIENTS = os.getenv("NUM_CLIENTS")
CONTAINER_MOUNT_DIRECTORY = os.getenv("CONTAINER_MOUNT_DIRECTORY")
BUILD_OUTPUT_DIRECTORY = os.getenv("BUILD_OUTPUT_DIRECTORY")
RUNTIME_OUTPUT_DIRECTORY = os.getenv("RUNTIME_OUTPUT_DIRECTORY")
REMOTE_OUTPUT_DIRECTORY = os.getenv("REMOTE_OUTPUT_DIRECTORY")
TARGET_OUTPUT_DIRECTORY = os.getenv("TARGET_OUTPUT_DIRECTORY")
DEBUG_SERVER_PORT_MIN = os.getenv("DEBUG_SERVER_PORT_MIN")
DEBUG_SERVER_PORT_MAX = os.getenv("DEBUG_SERVER_PORT_MAX")
NUM_CORES = mp.cpu_count()
CODELLDB_ADDRESS = ("127.0.0.1", 7349)

def run_cmd(cmd, exit_on_failure=True, **kwargs) -> subprocess.CompletedProcess:
    try:
        process_result = subprocess.run(cmd, **kwargs)
    except KeyboardInterrupt:
        sys.exit(0)
    if process_result.returncode != 0 and exit_on_failure:
        sys.exit(process_result.returncode)
    return process_result

def get_output(cmd, **kwargs) -> str:
    kwargs.update(text=True, capture_output=True)
    return run_cmd(cmd, False, **kwargs).stdout.strip()

def run_as_src_user(f):
    @functools.wraps(f)
    def wrapper(*args, **kwargs):
        uid, gid = os.getuid(), os.getgid()
        src_perms = os.stat(Path("src"))
        if uid == 0 and gid == 0:
            os.setegid(src_perms.st_gid)
            os.seteuid(src_perms.st_uid)
            f(*args, **kwargs)
            os.setegid(gid)
            os.seteuid(uid)
        else:
            f(*args, **kwargs)
    return wrapper

@run_as_src_user
def initialize_host():
    Path(REMOTE_OUTPUT_DIRECTORY, "debug").mkdir(parents=True, exist_ok=True)
    Path(REMOTE_OUTPUT_DIRECTORY, "release").mkdir(parents=True, exist_ok=True)
    Path(REMOTE_OUTPUT_DIRECTORY, "minsizerel").mkdir(parents=True, exist_ok=True)
    Path(REMOTE_OUTPUT_DIRECTORY, "relwithdebinfo").mkdir(parents=True, exist_ok=True)
    context = get_output(["docker", "context", "show"])
    if context == "rootless" or context == "desktop-linux":
        os.environ["CONTAINER_UID"] = str(0)
        os.environ["CONTAINER_GID"] = str(0)
    else:
        os.environ["CONTAINER_UID"] = str(os.geteuid())
        os.environ["CONTAINER_GID"] = str(os.getegid())

def wait_for_lldb_server(port):
    pattern = re.compile(f"{port}/tcp")
    while(True):
        out = get_output(["docker",  "ps"])
        if pattern.search(out):
            return
        time.sleep(.25)

def get_free_port():
    sock = socket.socket()
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('', 0))
    return sock.getsockname()[1]

def attach_to_process(pid, process_name, debug_binary, server_port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect(CODELLDB_ADDRESS)
        s.sendall(f"""
        {{
            name: {process_name},
            token: blueshift,
            terminal: console,
            request: attach,
            pid: {pid},
            sourceMap: {{ {CONTAINER_MOUNT_DIRECTORY} : {os.getcwd()} }},
            stopOnEntry: true,
            initCommands: [
                platform select remote-linux,
                platform connect connect://localhost:{server_port},
                settings set target.inherit-env false
            ],
            targetCreateCommands: [target create {debug_binary}],
            postRunCommands: [platform shell kill -SIGCONT {pid}]
        }}
        """.encode())

def attach_debugger(process_name, debug_binary, server_port):
    while True:
        pid = get_output(["docker", "exec", "debugger", "pgrep", "-f", process_name])
        if pid.isdigit():
            attach_to_process(pid, process_name, debug_binary, server_port)
            return

def symlink(source : Path | str, target : Path | str, is_directory = False):
    source = Path(source)
    if source.exists() or source.is_symlink():
        os.unlink(source)
    source.symlink_to(target, target_is_directory=is_directory)
    return source

def run(args):
    if args.local:
        executable = Path(".", TARGET_OUTPUT_DIRECTORY, RUNTIME_OUTPUT_DIRECTORY, args.binary)
        run_cmd([executable, *args.binary_args])
    else:
        initialize_host()
        run_cmd(["docker", "compose", "run", "--rm", "builder", "run", "-l", args.binary, *args.binary_args])

def debug(args):
    debug_target_path = "relwithdebinfo" if args.release else "debug"
    executable = Path(BUILD_OUTPUT_DIRECTORY, debug_target_path, RUNTIME_OUTPUT_DIRECTORY, args.binary)
    allow_visual = not args.terminal and args.debugger == "lldb"
    stop_on_entry = "true" if args.stop_on_entry else "false"
    # use controller name as debug name for readability if debugging client
    debug_name = args.binary_args[0] if args.binary == "client" and len(args.binary_args) > 0 else args.binary

    if args.server:
        if args.debugger == "lldb":
            run_cmd(["lldb-server", "gdbserver", f"*:{args.server}", "--", executable, *args.binary_args])
        else:
            run_cmd(["gdbserver", f"*:{args.server}", executable, *args.binary_args])
    elif args.listen:
        run_cmd(["lldb-server", "platform", "--listen", f"*:{args.listen}", "--server",
                 "--min-gdbserver-port", args.min_server_port, "--max-gdbserver-port", args.max_server_port])
    elif args.local:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            if allow_visual and s.connect_ex(CODELLDB_ADDRESS) == 0: # debug locally with codelldb
                s.sendall(f"""
                {{
                    name: {debug_name},
                    token: blueshift,
                    terminal: console,
                    stopOnEntry: {stop_on_entry},
                    program: {executable},
                    args: {args.binary_args}
                }}
                """.encode())
            else: # debug locally in terminal
                args_command = "--" if args.debugger == "lldb" else "--args"
                run_cmd([args.debugger, args_command, executable, *args.binary_args])
    else: # debug on container gdbserver
        initialize_host()
        DEBUG_SERVER_PORT = get_free_port()
        remote_binary = Path(REMOTE_OUTPUT_DIRECTORY, debug_target_path, RUNTIME_OUTPUT_DIRECTORY, args.binary)
        cwd = os.getcwd()
        # Initialize debug server
        run_cmd(["docker", "container", "stop", "-t", "1", f"{debug_name}"], exit_on_failure=False, stderr=subprocess.DEVNULL)
        run_cmd(["docker", "compose", "--profile", "debug", "run", "-d", "--no-deps",
                        "-p", f"{DEBUG_SERVER_PORT}:{DEBUG_SERVER_PORT}", "--name", debug_name, "--rm",
                        "debugger", "debug", "--server", f"{DEBUG_SERVER_PORT}",
                        "--debugger", args.debugger, args.binary, *args.binary_args])
        wait_for_lldb_server(DEBUG_SERVER_PORT)

        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            time.sleep(.25) # externally extend gdb-remote connection timeout
            if allow_visual and s.connect_ex(CODELLDB_ADDRESS) == 0: # debug remotely with codelldb
                s.sendall(f"""
                {{
                    name: {debug_name},
                    token: blueshift,
                    terminal: console,
                    request: attach,
                    stopOnEntry: {stop_on_entry},
                    sourceMap: {{ {CONTAINER_MOUNT_DIRECTORY} : {cwd} }},
                    targetCreateCommands: [target create {remote_binary}],
                    processCreateCommands: [gdb-remote localhost:{DEBUG_SERVER_PORT}]
                }}
                """.encode())
            elif args.debugger == "lldb": # debug remotely with lldb tui
                run_cmd(["lldb", "-o", f"settings set target.source-map {CONTAINER_MOUNT_DIRECTORY} {cwd}",
                                "-o", f"gdb-remote localhost:{DEBUG_SERVER_PORT}", "--", remote_binary, *args.binary_args])
            else: # debug remotely with gdb tui
                run_cmd(["gdb", "-ex", f"target remote localhost:{DEBUG_SERVER_PORT}",
                                "-ex", f"set substitute-path {CONTAINER_MOUNT_DIRECTORY} {cwd}",
                                "--args", remote_binary, *args.binary_args])

def deploy(args):
    # container never issues deploy command
    initialize_host()
    profiles = ["--profile", "debug"] if args.debug else []
    base_cmd = ["docker", "compose", *profiles]

    # ensure containers are properly destroyed
    atexit.register(run_cmd, [*base_cmd, "stop", "-t", "1"])
    atexit.register(run_cmd, [*base_cmd, "down", "-t", "1"])
    
    command = [*base_cmd, "up"]
    os.environ["DEPLOY_SRC"] = str(Path("samples", "src", args.filename))

    if args.num_clients:
        os.environ["NUM_CLIENTS"] = args.num_clients
    if args.build:
        command.append("--build")
    if args.debug:
        DEBUG_SERVER_PORT = get_free_port()
        num_clients = int(args.num_clients)
        os.environ["BUILD_TYPE"] = "RelWithDebInfo" if args.release_debug else "Debug"
        os.environ["DEPLOY_NAMESPACE"] = "service:debugger"
        os.environ["DEBUG_PORT"] = str(DEBUG_SERVER_PORT)
        os.environ["DEBUG_SERVER_PORT_MAX"] = str(int(DEBUG_SERVER_PORT_MIN) + num_clients + 1) # +1 for master

        debug_target_path = Path(REMOTE_OUTPUT_DIRECTORY, os.getenv("BUILD_TYPE").lower(), RUNTIME_OUTPUT_DIRECTORY)
        master_binary = Path(debug_target_path, "master")
        client_binary = Path(debug_target_path, "client")
        # start watchers to attach to each target process when spawned in container
        Thread(target=attach_debugger, args=("master", master_binary, DEBUG_SERVER_PORT), daemon=True).start()
        for i in range(num_clients): 
            Thread(target=attach_debugger, args=(f"blueshift-client-{i + 1}", client_binary, DEBUG_SERVER_PORT), daemon=True).start()

    run_cmd(command)

def build(args):
    ARTIFACT_TYPE = args.build_type.lower()
    ARTIFACT_DIR = Path(BUILD_OUTPUT_DIRECTORY, ARTIFACT_TYPE)
    COMPILE_DB_PATH = Path(BUILD_OUTPUT_DIRECTORY, "compile_commands.json")
    build_args = [f"--build-type", args.build_type,
                  "--compiler", args.compiler,
                  "--parallel", str(args.parallel)]
    if args.image_build:
        run_cmd(["docker", "compose", "build"])
    if args.clean:
        build_args.append("--clean")
        rmtree(ARTIFACT_DIR, ignore_errors=True)

    if args.local:
        cpp_compiler = f"-DCMAKE_CXX_COMPILER={args.compiler}"
        c_compiler = "-DCMAKE_C_COMPILER="
        linker = "-DCMAKE_LINKER="
        build_type = f"-DCMAKE_BUILD_TYPE={args.build_type}"
        if args.compiler == 'clang++':
            c_compiler += "clang"
            linker += "lld"
        else:
            c_compiler += "gcc"
            linker += "ld"
        cmake_args = [c_compiler, cpp_compiler, linker, build_type, "-Wno-dev"]
        run_cmd(["cmake", *cmake_args, "-S", ".", "-B", ARTIFACT_DIR])
        symlink(COMPILE_DB_PATH, Path(".", ARTIFACT_TYPE, "compile_commands.json"))

        # 1 <= j <= n-1 (keep 1 core free for background tasks)
        core_count = str(max(1, min(args.parallel, NUM_CORES - 1)))
        run_cmd(["cmake", "--build", ".", "-j", core_count, "-t", *args.make],
                       cwd=f"./{ARTIFACT_DIR}")
        symlink(TARGET_OUTPUT_DIRECTORY, Path(".", ARTIFACT_TYPE), True)
    else: # build binaries in remote container
        initialize_host()
        run_cmd(["docker", "compose", "run", "--rm", "builder",
                        "build", "-l", *build_args, *args.make])
        @run_as_src_user
        def link_compile_commands():
            symlink(TARGET_OUTPUT_DIRECTORY, Path(".", "remote", ARTIFACT_TYPE))
            curr_db_path = Path(REMOTE_OUTPUT_DIRECTORY, ARTIFACT_TYPE, "compile_commands.json")
            with (curr_db_path.open("r") as db, COMPILE_DB_PATH.open("w") as out):
                cwd = os.getcwd()
                build_dir = str(Path(CONTAINER_MOUNT_DIRECTORY, ARTIFACT_DIR))
                replacement_dir = str(Path(cwd, TARGET_OUTPUT_DIRECTORY))
                for line in db:
                    line = line.replace(build_dir, replacement_dir)  # replace build dir
                    line = line.replace(CONTAINER_MOUNT_DIRECTORY, cwd)  # replace source dir
                    out.write(line)
        link_compile_commands()

def test(args):
    os.environ["GTEST_COLOR"] = "1"
    ctest_args = [args.verbose, "-R", args.tests_regex]
    ctest_args = list(filter(None, ctest_args))
    if args.local:
        fail_output = ["--output-on-failure"] if not args.no_output_on_failure else []
        run_cmd(["ctest", *fail_output, *ctest_args], cwd=f"./{TARGET_OUTPUT_DIRECTORY}")
    else: # test in remote container
        initialize_host()
        run_cmd(["docker", "compose", "run", "--rm", "builder", "test", "-l", *ctest_args])

def reset(args):
    rmtree(BUILD_OUTPUT_DIRECTORY, ignore_errors=True)
    rmtree(".venv", ignore_errors=True)
    
    if args.preserve_images: return
    
    run_cmd(["docker", "compose", "down", "--rmi", "all", "-v", "--remove-orphans"])

    network_id = get_output(f"docker network ls | grep {NETWORK_NAME} | awk '{{ print $1 }}'", shell=True)
    container_ids = get_output(f"docker container ls | grep {PROJECT_PREFIX} | awk '{{ print $1 }}'", shell=True)
    image_ids = get_output(f"docker image ls | grep {PROJECT_PREFIX} | awk '{{ print $3 }}'", shell=True)
    volumes = get_output(f"docker volume ls | grep {PROJECT_NAME} | awk '{{ print $2 }}'", shell=True)
        
    if any((network_id, container_ids, image_ids, volumes)):
        run_cmd(["docker", "network", "rm", network_id])
        run_cmd(["docker", "container", "stop"] + container_ids.split('\n'))
        run_cmd(["docker", "container", "rm", "-f"] + container_ids.split('\n'))
        run_cmd(["docker", "image", "prune", "-f"])
        run_cmd(["docker", "image", "rm", "-f"] + image_ids.split('\n'))
        run_cmd(["docker", "volume", "rm", "-f"] + volumes.split('\n'))

    if args.system_prune:
        run_cmd(["docker", "system", "prune", "--volumes", "-af"])

    if args.rm_build_cache:
        run_cmd(["docker", "buildx", "prune", "-f"])

parser = argparse.ArgumentParser(description=f"{PROJECT_NAME} development environment build script", formatter_class=argparse.ArgumentDefaultsHelpFormatter)
subparsers = parser.add_subparsers(title="commands")

run_parser = subparsers.add_parser("run", help=f"run selected {PROJECT_NAME} binaries")
run_parser.add_argument("binary",
                        help="binary to execute")
run_parser.add_argument("binary_args",
                        help="program arguments",
                        nargs="*",
                        default=None)
run_parser.add_argument("-l", "--local",
                        help="run selected binary on local host instead of in containerized environment",
                        action="store_true")
run_parser.set_defaults(fn=run)

debug_parser = subparsers.add_parser("debug", help=f"debug selected {PROJECT_NAME} binaries")
debug_parser.add_argument("binary",
                        help="binary to debug")
debug_parser.add_argument("binary_args",
                        help="program arguments",
                        nargs="*",
                        default=None)
debug_parser.add_argument("-l", "--local",
                        help="debug selected binary on local host instead of in containerized environment",
                        action="store_true")
debug_parser.add_argument("-t", "--terminal",
                        help="force debugging through terminal alone",
                        action="store_true")
debug_parser.add_argument("-r", "--release",
                        help="debug release build with Og optimizations and limited symbols",
                        action="store_true")
debug_parser.add_argument("-d", "--debugger",
                        help="choose a specific debugger (note that the VSCode visual debugger is only supported with lldb)",
                        choices=["lldb", "gdb"],
                        default="lldb")
debug_parser.add_argument("--server",
                        help="create a gdbserver instance to attach to remotely at the specified port",
                        metavar="PORT",
                        default=None)
debug_parser.add_argument("--listen",
                        help="create a lldb-server instance listening on the specified port to spawn multiple gdbserver instances",
                        metavar="PORT",
                        default=None)
debug_parser.add_argument("--min-server-port",
                        help="specify minimum port for spawned gdbserver instances in listen mode",
                        metavar="PORT",
                        default=DEBUG_SERVER_PORT_MIN)
debug_parser.add_argument("--max-server-port",
                        help="specify maximum port for spawned gdbserver instances in listen mode",
                        metavar="PORT",
                        default=DEBUG_SERVER_PORT_MAX)
debug_parser.add_argument("--stop-on-entry",
                        help="halt execution on debugger attach",
                        action="store_true")
debug_parser.set_defaults(fn=debug)

deploy_parser = subparsers.add_parser("deploy", help=f"run {PROJECT_NAME} deployment environment (send SIGINT [ctrl+c] to stop gracefully)")
deploy_parser.add_argument("filename",
                          help="source file to execute",
                          nargs="?",
                          default="main.blu")
deploy_parser.add_argument("-n", "--num-clients",
                           help="number of clients to create (only used in deployment simulation)",
                           default=NUM_CLIENTS)
deploy_parser.add_argument("-b", "--build",
                           help="rebuild container images before running",
                           action="store_true")
deploy_parser.add_argument("-d", "--debug",
                           help="attach debugger to deployment configuration",
                           action="store_true")
deploy_parser.add_argument("-r", "--release-debug",
                           help="use release build with limited debug symbols in debug mode",
                           action="store_true")
deploy_parser.set_defaults(fn=deploy)

build_parser = subparsers.add_parser("build", help=f"build {PROJECT_NAME} binaries or deployment images")
build_parser.add_argument("make",
                          help="makefile arguments",
                          nargs="*",
                          default=["all"])
build_parser.add_argument("-i", "--image-build",
                          help="build container images",
                          action="store_true")
build_parser.add_argument("-c", "--compiler",
                          help="set compiler used for building",
                          choices=["clang++", "g++"],
                          default="clang++")
build_parser.add_argument("-j", "--parallel",
                          help="set number of logical cores to use for compilation",
                          metavar="JOBS",
                          type=int,
                          default=NUM_CORES)
build_parser.add_argument("-t", "--build-type",
                          help="set cmake build type",
                          choices=["Debug", "Release", "MinSizeRel", "RelWithDebInfo"],
                          default="Debug")
build_parser.add_argument("-l", "--local",
                          help="build for local host only; container builds are not updated",
                          action="store_true")
build_parser.add_argument("--clean",
                          help="clean build directory before building",
                          action="store_true")
build_parser.set_defaults(fn=build)

test_parser = subparsers.add_parser("test", help=f"test {PROJECT_NAME} binary and library builds")
test_parser.add_argument("-v", "--verbose",
                         help="show verbose output",
                         action="store_const",
                         const="--verbose")
test_parser.add_argument("--no-output-on-failure",
                         help="show condensed output regardless of failed tests",
                         action="store_true")
test_parser.add_argument("-l", "--local",
                         help="test on local host only; container builds are not tested",
                         action="store_true")
test_parser.add_argument("-R", "--tests-regex",
                         help="run subset of tests matching regular expression",
                         metavar="REGEX",
                         default=".*")
test_parser.set_defaults(fn=test)

reset_parser = subparsers.add_parser("reset", help="resets images and volumes")
reset_parser.add_argument("--rm-build-cache",
                          help="clears dangling docker build cache",
                          action="store_true")
reset_parser.add_argument("--preserve-images",
                          help="prevent resetting of container images",
                          action="store_true")
reset_parser.add_argument("--system-prune",
                          help="prune all system images and volumes (even those not associated with blueshift)",
                          action="store_true")
reset_parser.set_defaults(fn=reset)

args = parser.parse_args(args=None if len(sys.argv) > 1 else ["--help"])
args.fn(args)