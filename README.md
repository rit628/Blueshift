# Development Environment Setup

## Required Software
- `docker 25+`
- `python 3.9+`

## Recommended Software
- `cmake 3.30+` - Local Builds
- `llvm 19+` - Local Builds
- `clang 19+` - Local Builds
- `lld 19+` - Local Builds
- `lldb 19+` - Debugging
- `clangd 19+` - Language Server

## File & Directory Structure
- `bls` - Primary build script for Blueshift. This is the main script you should use for building, running, and testing the project. For a list of commands and options run `./bls --help`. Further, for help with any of the commands and their respective subcommands/options, run `./bls [command] --help`.
- `blueshift.py` - This script contains the logic for the primary build script. Any edits or new additions to the build script should be made here as `bls` is meant to be a thin wrapper around this script.
- `CMakeLists.txt` - Contains all build information for the target module, library, or test suite.
- `compose.yaml` - Contains the image, container, and network configurations for Blueshift's containerized environment. This file can generally be ignored unless there are problems with the primary build script.
- `.docker` - Contains Dockerfiles for building and running Blueshift. These files will rarely need updating and can generally be ignored unless there are problems with the primary build script
- `build` - Contains completed builds for Blueshift if binaries are compiled locally. Compiled binaries, libraries, and archives will be located in the subdirectory specified in the `.env` file. This folder also contains `compile_commands.json`, so any issues with your language server could result from a misconfigured `build` directory.
- `src` - Contains all Blueshift source code. In order for best compatibility with our testing framework, source code should be organized with module folders at the top level, associated library folders within, and optionally a `main.cpp` file at the root of each module containing its execution engine. An example of a properly organized `src` folder is given below:
```
Project
└──src
   ├── CMakeLists.txt
   ├── client
   │   ├── include
   │   ├── libmessage
   │   │   ├── CMakeLists.txt
   │   │   ├── message.cpp
   │   │   ├── message.h
   │   │   └── message_types.hpp
   │   ├── CMakeLists.txt
   │   └── main.cpp
   └── lang
       ├── include
       │   └── TYPES.include
       ├── liblexer
       │   ├── CMakeLists.txt
       │   ├── token.cpp
       │   ├── lexer.cpp
       │   └── lexer.hpp
       ├── libparser
       │   ├── CMakeLists.txt
       │   ├── include
       │   │   └── DEPS.include
       │   ├── parser.cpp
       │   └── parser.hpp
       ├── CMakeLists.txt
       └── main.cpp
```
- `test` - Contains all Blueshift unit tests. Organized in the same way as `src` with tests written for individual library components denoted by `test_component.cpp`.
```
Project
└──test
   ├── CMakeLists.txt
   ├── client
   │   ├── CMakeLists.txt
   │   └── libmessage
   │       ├── CMakeLists.txt
   │       └── test_message.cpp
   └── lang
       ├── liblexer
       │   ├── CMakeLists.txt
       │   ├── test_token.cpp
       │   └── test_lexer.cpp
       └── libparser
           ├── CMakeLists.txt
           └── test_parser.hpp
```

## Workflow Example (client module with message library and boost dependency)
1. Create a `client` directory under `src` and a `libmessage` directory under it.
2. Add source files for the `client` module's engine (`main.cpp`) and for the implementation of `libmessage`.
3. Add any new boost dependencies to the `REQUIRED_BOOST_LIBS` variable within `CMakeLists.txt`.
4. Create a new build target in a new `CMakeLists.txt` within the library directory as follows:
```cmake
# If building a static library with source files
bls_add_library(message STATIC LINKS protocol)
# If building an interface (header only) library
bls_add_library(message INTERFACE LINKS protocol)
```
5. Add a new entry to the `client` directory's `CMakeLists.txt` file as follows:
```cmake
add_subdirectory(libmessage)
```
6. Build with `./bls build client`, run with `./bls run client`, and debug with `./bls debug client` (first build will take some time due to docker image builds).
7. If desired, test cases can be added to `CMakeLists.txt` as follows:
```cmake
bls_add_test(libmessage LINKS message)
```
8. Build created tests with `./bls build` and run with `./bls test`.

## Troubleshooting
- If you can't run the build script, make sure it has execute permissions by running: `chmod +x blueshift bls`.
- If something is wrong with your build within the containerized environment, run `./bls reset` to clear the Blueshift container environment before reattempting your previous task.
- If `./bls reset` doesn't fix anything, try `./bls reset --rm-build-cache`.
- If `./bls reset --rm-build-cache` fails to resolve the issue as well, use the `--local` argument in your target command to perform the operation locally while a solution is found to the issue.
- If you are on Windows and are having issues with building and/or running Blueshift, try cloning the repo to a directory within WSL instead of a directory on the host's filesystem.

## Recommendations
It is highly recommended to use `clangd 19+` as the language server for development, as it will automatically detect dependency locations from the `compile_commands.json` file generated from the `cmake` build. Configuration details will differ depending on your IDE; the instructions for VSCode are given below:
- Install the official `clangd` extension.
- If the Microsoft C/C++ is installed, disable Intellisense (there will be a pop up upon opening the project).
- Run `./bls build` to do an initial build and generate `compile_commands.json`.
- Run the `clangd: Restart language server` command within VSCode to update the server with the new dependencies.

Both `lldb` and `gdb` will integrate with the build script, however, use of the VSCode visual debugger will require the `CodeLLDB` extension, which ships with its own version of `lldb`. The instructions for setup with the VSCode visual debugger are given below:
- Install the `CodeLLDB` extension.
- Add the following line to the settings map your workspace configuration: `"lldb.rpcServer": { "host": "127.0.0.1", "port": 7349, "token": "blueshift" }`.
- Additionally, optionally add the following line to your settings map if you would like to manually control disassembly view `"lldb.showDisassembly": "never"`.
- If using a single root workspace, the aforementioned line(s) will need to be added to the settings configuration within `.vscode/settings.json`.
- If using a multi-root workspace, the aforementioned line(s) will need to be added to the settings configuration within `[workspace-name].code-workspace` under the `settings` key in the root map.
- Otherwise, if neither of the above work, simply add it to the global `settings.json` configuration for your user profile.

# Guidelines & Best Practices

## Commit Structure
In general, all commits should have the form: `component: Short change explanation (in the
imperative)`

For example: `readme: Add documentation for commit structure`

For larger commits (such as pr commits), if the extent of what has been changed cannot be
encompassed in a single line, use the following format:
```
pr-name: Short explanation of what the pr encompasses (in the imperative)
* component-1: Short change explanation (in the imperative)
* component-2: Short change explanation (in the imperative)
.
.
.
* component-n: Short change explanation (in the imperative)
```

## Pull Requests
All pull requests should assign at least one reviewer to look over them. This list should include
any developer who may have relevant knowledge on the components being worked on (ie.
people who have previously contributed to those components).

For all current and future PRs DO NOT:
- Merge into main without verbal, written, or GitHub approval of the PR from one of the
listed reviewers
- Commit a merge commit into main (pr commits should always be squashed to ensure
linear history)

When a PR is approved, run the following command in your local branch before clicking merge
on GitHub to ensure you do not create any merge conflicts in main and adhere to the commit
structure:

- `git rebase -i $(git merge-base HEAD origin/main)`

After running the above command, your git merge editor will ask you what you would like to do
with all of your commits on your local branch. Choose `reword` for the oldest commit and
`fixup` for all others. You will then be prompted to resolve all merge conflicts with main. In most
cases, these will be trivial conflicts so long as all developers adhere to the best practices listed in
the following section. For nontrivial merge conflicts, consult the author of the conflicting code to
discuss how to best resolve the conflict without breaking their changes. Once all conflicts are
resolved, you will be asked to reword the commit message for the new single commit all
previous changes will be registered under. Simply follow the guidelines on commit structure and
complete the commit. Once done, force push the new commit onto your branch and the PR
should automatically update to reflect the rebase. Feel free to merge the commit into main via the
GitHub GUI at this point.

## Best Practices
- Use short-lived branches based on assigned Trello tickets to work on new changes.
- Always create your feature branch off of `origin/main` to avoid history problems when
rebasing for a PR.
- Do not continue to work on the same branch post PR merge. Instead, create a new short-lived branch for the next PR. This helps avoid nasty merge conflicts in the future due to
outdated history.