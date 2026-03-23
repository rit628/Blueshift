import env
from util import *
from shutil import rmtree

def reset(args):
    print("Removing build directory, dependency directory, and venv...")
    rmtree(env.BUILD_OUTPUT_DIRECTORY, ignore_errors=True)
    rmtree(env.DEPENDENCY_DIRECTORY, ignore_errors=True)
    rmtree(env.VENV_DIRECTORY, ignore_errors=True)
    
    if args.preserve_images: return
    
    print("Shutting down all containers...")
    run_cmd(["docker", "compose", "down", "--rmi", "all", "-v", "--remove-orphans"])

    network_id = get_output(["docker", "network", "ls", "-f", f"name={env.NETWORK_NAME}", "--format", "{{.ID}}"])
    container_ids = get_output(["docker", "container", "ls", "-af", f"name=({env.PROJECT_PREFIX}|{env.PROJECT_NAME})-*", "--format", "{{.ID}}"])
    image_ids = get_output(["docker", "image", "ls", "-af", f"reference=*/{env.PROJECT_PREFIX}-*", "--format", "{{.ID}}"])
    volumes = get_output(["docker", "volume", "ls", "-f", f"name={env.PROJECT_PREFIX}_*", "--format", "{{.Nam}}}"])
    
    if network_id:
        print("Removing dangling networks...")
        run_cmd(["docker", "network", "rm", network_id])
    if container_ids:
        print("Removing dangling containers...")
        run_cmd(["docker", "container", "stop", "-t", "5"] + container_ids.split('\n'), exit_on_failure=False)
        run_cmd(["docker", "container", "rm", "-f"] + container_ids.split('\n'))
    if image_ids:
        print("Removing dangling images...")
        run_cmd(["docker", "image", "rm", "-f"] + image_ids.split('\n'))
    if volumes:
        print("Removing dangling volumes...")
        run_cmd(["docker", "volume", "rm", "-f"] + volumes.split('\n'))

    if args.system_prune:
        print("Pruning everything in current docker context...")
        run_cmd(["docker", "system", "prune", "--volumes", "-af"])

    if args.rm_build_cache:
        print("Removing dangling image build cache for current docker context...")
        run_cmd(["docker", "buildx", "prune", "-f"])