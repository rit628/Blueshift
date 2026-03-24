from util import *
import os

def test(args):
    os.environ["GTEST_COLOR"] = "1"
    ctest_args = [args.verbose, "-R", args.tests_regex]
    if args.local: # TODO: fix local test execution in non container environments (map container src dir to host src dir in ctest?)
        fail_output = ["--output-on-failure"] if not args.no_output_on_failure else []
        run_cmd(["ctest", *fail_output, *ctest_args], cwd=f"./{get_target_dir()}")
    else: # test in remote container
        initialize_host()
        run_cmd(["docker", "compose", "run", "--rm", "builder", "test", "-l", *ctest_args])