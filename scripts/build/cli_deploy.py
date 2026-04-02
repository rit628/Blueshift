import env
from util import *
import socket
import atexit
import os
from pathlib import Path
from threading import Thread

def attach_to_process(pid, process_name, debug_binary, server_port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect(env.CODELLDB_ADDRESS)
        s.sendall(f"""
        {{
            name: {process_name},
            token: blueshift,
            terminal: console,
            request: attach,
            pid: {pid},
            sourceMap: {{ {env.CONTAINER_MOUNT_DIRECTORY} : {os.getcwd()} }},
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

def deploy(args):
    # container never issues deploy command
    initialize_host()

    profiles = []
    if args.debug:
        profiles += ["--profile", "debug"]
    if args.vnc:
        profiles += ["--profile", "vnc"]
        Thread(target=open_vnc_display, daemon=True).start()
        
    base_cmd = ["docker", "compose", *profiles]

    # ensure containers are properly destroyed
    atexit.register(run_cmd, [*base_cmd, "stop", "-t", "1"])
    atexit.register(run_cmd, [*base_cmd, "down", "-t", "1"])
    
    command = [*base_cmd, "up"]
    os.environ["DEPLOY_SRC"] = str(Path("samples", "src", args.filename))

    default_target_path = get_target_dir("linux64")
    master_binary = Path(default_target_path, "master")
    client_binary = Path(default_target_path, "client")

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
        os.environ["DEBUG_SERVER_PORT_MAX"] = str(int(env.DEBUG_SERVER_PORT_MIN) + num_clients + 1) # +1 for master

        debug_target_path = Path(env.BUILD_OUTPUT_DIRECTORY, env.CONTAINER_PLATFORM, os.getenv("BUILD_TYPE").lower(), env.RUNTIME_OUTPUT_DIRECTORY)
        master_binary = Path(debug_target_path, "master")
        client_binary = Path(debug_target_path, "client")
        # start watchers to attach to each target process when spawned in container
        Thread(target=attach_debugger, args=("master", master_binary, DEBUG_SERVER_PORT), daemon=True).start()
        for i in range(num_clients): 
            Thread(target=attach_debugger, args=(f"blueshift-client-{i + 1}", client_binary, DEBUG_SERVER_PORT), daemon=True).start()
    
    os.environ["MASTER_PROGRAM_NAME"] = str(master_binary)
    os.environ["CLIENT_PROGRAM_NAME"] = str(client_binary)

    run_cmd(command)