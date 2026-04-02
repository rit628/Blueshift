import env
import atexit
import os
import platform
import shutil
import socket
import subprocess
import sys
import webbrowser
from functools import wraps
from time import sleep
from pathlib import Path

# restrict wildcard imports
__all__ = [
      "run_cmd"
    , "get_output"
    , "run_as_src_user"
    , "initialize_host"
    , "copy"
    , "get_host_os"
    , "read_build_info"
    , "get_target_dir"
    , "wait_for_container_server"
    , "get_free_port"
    , "open_vnc_display"
    , "initialize_vnc_display"
]

def run_cmd(cmd, exit_on_failure=True, **kwargs) -> subprocess.CompletedProcess | None:
    cmd = list(filter(None, cmd))
    try:
        process_result = subprocess.run(cmd, **kwargs)
    except KeyboardInterrupt:
        sys.exit(0)
    except FileNotFoundError as e:
        if exit_on_failure:
            raise e
        else: # ignore error
            return None
    if process_result.returncode != 0 and exit_on_failure:
        sys.exit(process_result.returncode)
    return process_result

def get_output(cmd, **kwargs) -> str:
    kwargs.update(text=True, capture_output=True)
    return run_cmd(cmd, False, **kwargs).stdout.strip()

def run_as_src_user(f):
    wraps(f)
    def wrapper(*args, **kwargs):
        if os.name == "nt": # unix perms dont apply to windows
            return f(*args, **kwargs)
        
        uid, gid = os.getuid(), os.getgid()
        src_perms = os.stat(Path("src"))
        os.setegid(src_perms.st_gid)
        os.seteuid(src_perms.st_uid)
        result = f(*args, **kwargs)
        os.setegid(gid)
        os.seteuid(uid)

        return result
    
    return wrapper

@run_as_src_user
def initialize_host():
    run_cmd(["docker", "network", "create", env.NETWORK_NAME, "--label", "com.docker.compose.network=default", "--label", f"com.docker.compose.project={env.PROJECT_NAME}"],
            exit_on_failure=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    
    context = get_output(["docker", "context", "show"])
    if context == "rootless" or context == "desktop-linux":
        os.environ["CONTAINER_UID"] = str(0)
        os.environ["CONTAINER_GID"] = str(0)
    else:
        os.environ["CONTAINER_UID"] = str(os.geteuid())
        os.environ["CONTAINER_GID"] = str(os.getegid())
    
    if platform.system() == "Linux" and os.getenv("DISPLAY_NAME") == "nullDisplay": # allow x11 forwarding through docker
        run_cmd(["xhost", "+local:docker"], exit_on_failure=False, stdout=subprocess.DEVNULL)
        os.environ["DISPLAY_NAME"] = os.getenv("DISPLAY", "")
        os.environ["DISPLAY_MOUNT"] = "/tmp/.X11-unix"
        atexit.register(run_cmd, ["xhost", "-local:docker"], exit_on_failure=False, stdout=subprocess.DEVNULL)

def copy(source : Path, *destinations : Path):
    for destination in destinations:
        shutil.copy2(source, destination)

def get_host_os():
    system = platform.system()
    if system == "Linux":
        return "linux64"
    if system == "Windows":
        return "win64"
    if system == "Darwin":
        return "osx64"
    return system

def read_build_info(info_path : Path):
    if not info_path.exists():
        print("Build target not found.")
        sys.exit(1)
    with info_path.open() as info:
        return info.readline().split()

def get_target_dir(PLATFORM=""):
    if not PLATFORM:
        PLATFORM = get_host_os()

    target_info = Path(env.BUILD_OUTPUT_DIRECTORY, PLATFORM, ".info")
    ARTIFACT_TYPE = read_build_info(target_info)[2]
    return Path(env.BUILD_OUTPUT_DIRECTORY, PLATFORM, ARTIFACT_TYPE)

def wait_for_container_server(port, wait_for_exit=False):
    while(True):
        container_running = len(get_output(["docker", "ps", "--filter", f"publish={port}", "--format", "{{.Ports}}"])) > 0
        stop = container_running != wait_for_exit # container_running XOR wait_for_exit
        if stop:
            return
        sleep(.25)

def get_free_port():
    sock = socket.socket()
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('', 0))
    return sock.getsockname()[1]

def open_vnc_display():
    os.environ["DISPLAY_NAME"] = f"{env.VNC_CONTAINER_NAME}:0.0"
    wait_for_container_server(env.VNC_PORT)
    sleep(1.5) # wait for all X subsystems to be ready
    print("opening vnc display in browser...")
    webbrowser.open(f"http://127.0.0.1:{env.VNC_PORT}/vnc.html")

def initialize_vnc_display():
    print("intializing vnc display...")
    run_cmd(["docker", "compose", "up", "-d", env.VNC_CONTAINER_NAME])
    atexit.register(run_cmd, ["docker", "compose", "down", "-t", "1", env.VNC_CONTAINER_NAME], exit_on_failure=False, stderr=subprocess.DEVNULL)
    open_vnc_display()