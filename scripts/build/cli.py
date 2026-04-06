import env
from cli_run import run
from cli_debug import debug
from cli_deploy import deploy
from cli_build import build
from cli_test import test
from cli_reset import reset
import argparse
import os
import sys

parser = argparse.ArgumentParser(description=f"{env.PROJECT_NAME} development environment build script", formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument("-e", "--env",
                    help="set environment variable",
                    action="append",
                    default=[])

subparsers = parser.add_subparsers(title="commands")

run_parser = subparsers.add_parser("run", help=f"run selected {env.PROJECT_NAME} binaries")
run_parser.add_argument("binary",
                        help="binary to execute")
run_parser.add_argument("binary_args",
                        help="program arguments",
                        nargs="*",
                        default=None)
run_parser.add_argument("-l", "--local",
                        help="run selected binary on local host instead of in containerized environment",
                        action="store_true")
run_parser.add_argument("-p", "--packet-forward",
                        help="forward all network packets from container LAN to host LAN",
                        action="store_true")
run_parser.add_argument("-u", "--udp-broadcast-forward",
                        help="""start a listener that received udp broadcasts on 127.0.0.1 and forwards them to host LAN
                                (necessary if running with -p on OSX)""",
                        action="store_true")
run_parser.add_argument("--driver",
                        help="""specify a program to drive execution for a particular target platform
                                when running locally""",
                        metavar="PROGRAM:TARGET_PLATFORM",
                        default=None)
run_parser.add_argument("--vnc",
                        help="start a novnc display server to forward container GUI applications",
                        action="store_true")
run_parser.set_defaults(fn=run)

debug_parser = subparsers.add_parser("debug", help=f"debug selected {env.PROJECT_NAME} binaries")
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
debug_parser.add_argument("--vnc",
                        help="start a novnc display server to forward container GUI applications",
                        action="store_true")
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
                        default=env.DEBUG_SERVER_PORT_MIN)
debug_parser.add_argument("--max-server-port",
                        help="specify maximum port for spawned gdbserver instances in listen mode",
                        metavar="PORT",
                        default=env.DEBUG_SERVER_PORT_MAX)
debug_parser.add_argument("--stop-on-entry",
                        help="halt execution on debugger attach",
                        action="store_true")
debug_parser.set_defaults(fn=debug)

deploy_parser = subparsers.add_parser("deploy", help=f"run {env.PROJECT_NAME} deployment environment (send SIGINT [ctrl+c] to stop gracefully)")
deploy_parser.add_argument("filename",
                          help="source file to execute",
                          nargs="?",
                          default="main.blu")
deploy_parser.add_argument("-n", "--num-clients",
                           help="number of clients to create (only used in deployment simulation)",
                           default=env.NUM_CLIENTS)
deploy_parser.add_argument("-b", "--build",
                           help="rebuild container images before running",
                           action="store_true")
deploy_parser.add_argument("-d", "--debug",
                           help="attach debugger to deployment configuration",
                           action="store_true")
deploy_parser.add_argument("-r", "--release-debug",
                           help="use release build with limited debug symbols in debug mode",
                           action="store_true")
deploy_parser.add_argument("--vnc",
                            help="start a novnc display server to forward container GUI applications",
                            action="store_true")
deploy_parser.set_defaults(fn=deploy)

build_parser = subparsers.add_parser("build", help=f"build {env.PROJECT_NAME} binaries or deployment images")
build_parser.add_argument("make",
                          help="makefile arguments",
                          nargs="*",
                          default=["all"])
build_parser.add_argument("-i", "--image-build",
                          help="build container images prior to running",
                          action="store_const",
                          const="--build")
build_parser.add_argument("-j", "--parallel",
                          help="set number of logical cores to use for compilation",
                          metavar="JOBS",
                          type=int,
                          default=env.NUM_CORES)
build_parser.add_argument("-b", "--build-type",
                          help="set cmake build type",
                          choices=["Debug", "Release", "MinSizeRel", "RelWithDebInfo"],
                          default="Debug")
build_parser.add_argument("-t", "--target",
                          help="set target platform",
                          choices=["linux64", "rpi64", "win64", "osx64", "wasm32-web", "wasm32-node"],
                          default="linux64")
build_parser.add_argument("-v", "--verbose",
                          help="get verbose output from cmake",
                          action="store_const",
                          const="--verbose")
build_parser.add_argument("-l", "--local",
                          help="build for local host only; container builds are not updated",
                          action="store_true")
build_parser.add_argument("--clean",
                          help="clean build directory before building",
                          action="store_true")
build_parser.set_defaults(fn=build)

test_parser = subparsers.add_parser("test", help=f"test {env.PROJECT_NAME} binary and library builds")
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

if __name__ == "__main__":
    args = parser.parse_args(args=None if len(sys.argv) > 1 else ["--help"])
    for env in args.env:
        var, val = env.split("=")
        os.environ[var] = val
    args.fn(args)