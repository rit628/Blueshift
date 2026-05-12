from multiprocessing import cpu_count
from os import getenv
from dotenv import load_dotenv

load_dotenv()
# project cfg
PROJECT_NAME = getenv("PROJECT_NAME")
PROJECT_PREFIX = getenv("PROJECT_PREFIX")

# script cfg
VENV_DIRECTORY = getenv("VENV_DIRECTORY")

# cmake cfg
BUILD_OUTPUT_DIRECTORY = getenv("BUILD_OUTPUT_DIRECTORY")
GENERATED_OUTPUT_DIRECTORY = getenv("GENERATED_OUTPUT_DIRECTORY")
RUNTIME_OUTPUT_DIRECTORY = getenv("RUNTIME_OUTPUT_DIRECTORY")
DEPENDENCY_DIRECTORY = getenv("DEPENDENCY_DIRECTORY")

# common container cfg
CONTAINER_MOUNT_DIRECTORY = getenv("CONTAINER_MOUNT_DIRECTORY")
NETWORK_NAME = getenv("NETWORK_NAME")

# display container cfg
VNC_CONTAINER_NAME = getenv("VNC_CONTAINER_NAME")
VNC_PORT = getenv("VNC_PORT")

# build container cfg
CONTAINER_PLATFORM = getenv("PLATFORM_TAG")

# debug container cfg
DEBUG_SERVER_PORT_MIN = getenv("DEBUG_SERVER_PORT_MIN")
DEBUG_SERVER_PORT_MAX = getenv("DEBUG_SERVER_PORT_MAX")

# client container cfg
BROADCAST_PORT = getenv("BROADCAST_PORT")
NUM_CLIENTS = getenv("NUM_CLIENTS")

# util variables
NUM_CORES = cpu_count()
CODELLDB_ADDRESS = ("127.0.0.1", 7349)