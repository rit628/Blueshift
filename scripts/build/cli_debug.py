import env
from util import *
import socket
import os
import atexit
from subprocess import DEVNULL
from time import sleep
from pathlib import Path

def get_remote_path(build_path):
    return read_build_info(Path(build_path, ".info"))[0]

def debug(args):
    debug_target_path = "relwithdebinfo" if args.release else "debug"
    build_path = Path(env.BUILD_OUTPUT_DIRECTORY, env.PLATFORM_TAG, debug_target_path)
    executable = Path(build_path, env.RUNTIME_OUTPUT_DIRECTORY, args.binary)
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
        # update executable to use platform specific binary
        build_path = Path(env.BUILD_OUTPUT_DIRECTORY, get_host_os(), debug_target_path)
        executable = Path(build_path, env.RUNTIME_OUTPUT_DIRECTORY, args.binary)
        remote_path = get_remote_path(build_path)

        if allow_visual: # debug locally with codelldb
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect(env.CODELLDB_ADDRESS)
                s.sendall(f"""
                {{
                    name: {debug_name},
                    token: blueshift,
                    terminal: console,
                    stopOnEntry: {stop_on_entry},
                    sourceMap: {{ {remote_path} : {os.getcwd()} }},
                    program: {executable},
                    args: {args.binary_args}
                }}
                """.encode())
        elif args.debugger == "lldb": # debug locally with lldb tui
            run_cmd(["lldb", "-o", f"settings set target.source-map {remote_path} {cwd}",
                             "--", executable, *args.binary_args])
        else: # debug locally with gdb tui
            run_cmd(["gdb", "-ex", f"set substitute-path {remote_path} {cwd}",
                            "--args", executable, *args.binary_args])
    else: # debug on container gdbserver
        if args.vnc: initialize_vnc_display()
        initialize_host()
        DEBUG_SERVER_PORT = get_free_port()
        cwd = os.getcwd()
        remote_path = get_remote_path(build_path)

        # Initialize debug server
        run_cmd(["docker", "compose", "--profile", "debug", "run", "-d", "--no-deps",
                        "-p", f"{DEBUG_SERVER_PORT}:{DEBUG_SERVER_PORT}", "--name", debug_name, "--rm",
                        "debugger", "debug", "--server", f"{DEBUG_SERVER_PORT}",
                        "--debugger", args.debugger, args.binary, *args.binary_args])
        wait_for_container_server(DEBUG_SERVER_PORT)
        atexit.register(run_cmd, ["docker", "container", "stop", "-t", "1", debug_name], exit_on_failure=False, stderr=DEVNULL)

        if allow_visual: # debug remotely with codelldb
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect(env.CODELLDB_ADDRESS) == 0
                s.sendall(f"""
                {{
                    name: {debug_name},
                    token: blueshift,
                    terminal: console,
                    request: attach,
                    stopOnEntry: {stop_on_entry},
                    sourceMap: {{ {remote_path} : {cwd} }},
                    targetCreateCommands: [target create {executable}],
                    processCreateCommands: [gdb-remote localhost:{DEBUG_SERVER_PORT}]
                }}
                """.encode())
                sleep(1) # externally extend gdb-remote connection timeout
            wait_for_container_server(DEBUG_SERVER_PORT, wait_for_exit=True)
        elif args.debugger == "lldb": # debug remotely with lldb tui
            run_cmd(["lldb", "-o", f"settings set target.source-map {remote_path} {cwd}",
                             "-o", f"gdb-remote localhost:{DEBUG_SERVER_PORT}", "--", executable, *args.binary_args])
        else: # debug remotely with gdb tui
            run_cmd(["gdb", "-ex", f"target remote localhost:{DEBUG_SERVER_PORT}",
                            "-ex", f"set substitute-path {remote_path} {cwd}",
                            "--args", executable, *args.binary_args])