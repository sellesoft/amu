#!/usr/bin/python
#_____________________________________________________________________________________________________
#                                            Usage
#_____________________________________________________________________________________________________
# build.py <command> [arguments...]
#
# Commands:
#   (empty)       Compile and link (default)
#   compile       Compile only
#   link          Link with previously generated .obj/.o
#   one <file>    Compile one file and link with previously generated .obj/.o
#
# Arguments:
#   --v    Echo build commands to the console
#   --d    Build with    debug info and without optimization (default)
#   --r    Build without debug info and with    optimization
#   --sa   Enable static analysis
#   --ba   Enable build analysis (currently only works with clang with ClangBuildAnalyzer installed)
#   TODO this isn't implementaed yet --pch  Generate a precompiled header from a selection of headers in the 'precompiled' array. If one of these files is found to be newer than its pch, it will be regenerated
#   --cc   Generate compile_commands.json for use with clangd (requires Build EAR and clang)
#
#   -platform <win32,mac,linux>           Build for specified OS: win32, mac, linux (default: builder's OS)
#   -compiler <cl,gcc,clang,clang-cl>     Build using the specified compiler (default: cl on Windows, gcc on Mac and Linux)
#   -linker <link,ld,lld,lld-link>        Build using the specified linker (default: link on Windows, ld on Mac and Linux)
#   -notcurses <path>                     Build with support for a notcurses interface, requires providing a path to notcurses source files
#   -tracy <path>                         Build with support for the Tracy profiler, providing a path to its source files

# TODO(sushi) clean this up it's such a huge mess

import os,sys,subprocess,platform,time
from datetime import datetime
from threading import Thread

app_name = "amu"

includes = (
    "-Isrc "
)

sources = {
    "app": "src/amu.cpp"
    # "app": "misc/co.cpp"
    # "app": "misc/fibers/fiber.cpp"
}

config = {
    "verbose": False, 
    "time": False,
    "buildmode": "debug",
    "profiling": "off", # "off", "on", "on and wait"
    "static_analysis": False,
    "build_analysis": False,
    "use_pch": False,
    "use_notcurses": False,
    "use_tracy": False,
    "gen_compcmd": False,

    "compiler": "unknown",
    "linker":   "unknown",
}

precompiled = [

]

match platform.system():
    case 'Windows': 
        config["platform"] = "win32"
        config["compiler"] = "cl"
        config["linker"] = "link"
    case 'Linux': 
        config["platform"] = "linux"
        config["compiler"] = "clang++"
        config["linker"] = "ld"
    case _:
        # TODO(sushi) if we ever setup cross compiling somehow, we need to remove this
        print(f"unsupported platform: {platform.system()}")

#
#  gather command line arguments
#______________________________________________________________________________________________________________________

i = 0
while i < len(sys.argv):
    match sys.argv[i]:
        case "--v":    config["verbose"] = True
        case "--time": config["time"] = True
        case "--d":    continue # dont do anything cause debug is the default
        case "--r":    config["buildmode"] = "release"
        case "--p":    config["profiling"] = "on"
        case "--pw":   config["profiling"] = "on and wait"
        case "--sa":   config["static_analysis"] = True
        case "--ba":   config["build_analysis"] = True
        case "--pch":  config["use_pch"] = True
        case "--nc":   config["use_notcurses"] = True
        case "--cc":   config["gen_compcmd"] = True
        
        case "-platform":
            if i != len(sys.argv) - 1:
                i += 1
                config["platform"] = sys.argv[i]
                if config["platform"] not in ("win32", "linux", "mac"):
                    print(f"unknown platform: {sys.argv[i]}, expected one of (win32, linux, mac).")
                    quit()
            else:
                print("expected a platform (win32, linux, max) after switch '-platform'")

        case "-compiler":
            if i != len(sys.argv) - 1:
                i += 1
                config["compiler"] = sys.argv[i]
                if config["compiler"] not in ("cl", "gcc", "clang", "clang-cl"):
                    print(f"unknown compiler: {sys.argv[i]}, expected one of (cl, gcc, clang, clang-cl).")
                    quit()
            else:
                print("expected a compiler (cl, gcc, clang, clang-cl) after switch '-compiler'")

        case "-linker":
            if i != len(sys.argv) - 1:
                i += 1
                config["linker"] = sys.argv[i]
                if config["linker"] not in ("link", "ld", "lld", "lld-link"):
                    print(f"unknown linker: {sys.argv[i]}, expected one of (link, ld, lld, lld-link).")
                    quit()
            else:
                print("expected a linker (cl, gcc, clang, clang-cl) after switch '-linker'")
        
        case "-notcurses":
            if i != len(sys.argv) - 1:
                i += 1
                includes += "-I" + sys.argv[i] + " "
                config["use_notcurses"] = True
            else:
                print("expected a path to notcurses source after -notcurses")
                quit()
        
        case "-tracy":
            if i == len(sys.argv) - 1:
                print("expected a path to tracy source after -tracy")
                quit()
            i += 1
            includes += "-I" + sys.argv[i] + " "
            config["use_tracy"] = True

        case _:
            if sys.argv[i].startswith("-"):
                print(f"unknown switch: {sys.argv[i]}")
                quit()
    i += 1
# end of cli arg collection

# determines what compilers and linkers are compatible with each platform
# so that extending this is easy
compatibility = {
    "win32": {
        "compiler": ["cl", "clang-cl"],
        "linker": ["link"]
    },
    "linux":{
        "compiler": ["clang", "gcc", "clang++"],
        "linker": ["ld", "lld", "lld-link", "clang++", "clang"]
    }
}

# setup folders
# this assumes that the build script is in a misc folder that is in the root of the repo
folders = {}
folders["misc"] = os.path.dirname(__file__)
folders["root"] = os.path.abspath(f"{folders['misc']}/..")
folders["build"] = f"{folders['root']}/build/{config['buildmode']}"
folders["temp"] = f"{folders['root']}/temp"

os.chdir(folders["root"])

#
#  data
#______________________________________________________________________________________________________________________

parts = {
    "link":{ # various things handed to the linker
        "win32": { 
            "always": ["gdi32", "shell32", "ws2_32", "winmm"],
            "vulkan": ["vulkan-1", "shaderc_combined"],
            "opengl": ["opengl32"],
            "none": [],
            "paths": []
        },
        "linux": {
            "always": ["stdc++fs"],
            "none": [],
            "paths": []
        },
        "flags":{
            **dict.fromkeys(["link", "lld-link"], ( # NOTE(sushi) this is just assigning this value to both link and lld-link, so it doesnt appear twice
                "-nologo "         # prevents microsoft copyright banner
                "-opt:ref "        # doesn't link functions and data that are never used 
                "-incremental:no " # relink everything
            )),
            **dict.fromkeys(["ld", "lld"], "")
        },
        "prefix":{
            **dict.fromkeys(["link", "lld-link"], {
                "path": "-libpath:",
                "file": "",
            }),
            **dict.fromkeys(["ld", "lld"], {
                "path": "-L",
                "file": "-l",
            }),
        }
    },

    "defines":{ 
        "buildmode": {
            "release": "-DBUILD_INTERNAL=0 -DBUILD_SLOW=0 -DBUILD_RELEASE=1 ",
            "debug": "-DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -DBUILD_RELEASE=0 ",
        },
        "always": "",
    },

    "compiler_flags":{

        "cl": {
            "always":( # flags always applied if this compiler is chosen
                "-diagnostics:column " # prints diagnostics on one line, giving column number
                "-EHsc "               # enables C++ exception handling and defaults 'extern "C"' to nothrow
                "-nologo "             # prevents initial banner from displaying
                "-MD "                 # create a multithreaded dll
                "-Oi "                 # generates intrinsic functions
                "-GR "                 # enables runtime type information
                "-std:c++20 "          # use C++17
                "-utf-8 "              # read source as utf8
                "-MP "                 # builds multiple source files concurrently
                "-W1"                  # warning level 1
                "-wd4100"              # unused function parameter
                "-wd4189"              # unused local variables
                "-wd4201"              # nameless unions and structs
                "-wd4311"              # pointer truncation
                "-wd4706"              # assignment within conditional expression
            ),
            "release": (
                "-O2 " # maximizes speed (O1 minimizes size)
            ),
            "debug": (
                "-Z7 " # generates C 7.0-compatible debugging information
                "-Od " # disables optimization complete
            ),
            "analyze": {
                True: "--analyze ",
                False: ""
            }
        },

        "clang-cl": {
            "always":( # flags always applied if this compiler is chosen
                "-diagnostics:column "  # prints diagnostics on one line, giving column number
                "-EHsc "                # enables C++ exception handling and defaults 'extern "C"' to nothrow
                "-nologo "              # prevents initial banner from displaying
                "-MD "                  # create a multithreaded dll
                "-Oi "                  # generates intrinsic functions
                "-GR "                  # enables runtime type information
                "-std:c++20 "           # use C++17
                "-utf-8 "               # read source as utf8
                "-msse3 "               # enables SSE
                "-Wno-unused-value "                # 
                "-Wno-writable-strings "            # conversion from string literals to other things
                "-Wno-implicitly-unsigned-literal " # 
                "-Wno-nonportable-include-path "    # 
                "-Wno-unused-function "             #
                "-Wno-unused-variable "             #
                "-Wno-undefined-inline "            #
            ),
            "release": (
                "-O2 " # maximizes speed (O1 minimizes size)
            ),
            "debug": (
                "-Z7 " # generates C 7.0-compatible debugging information
                "-Od " # disables optimization completely
            ),
            "analyze": "--analyze "
        },

        "gcc": {
            "not implemented yet"
        },

        "clang++":{
            "always": ( # flags always applied if this compiler is chosen
                "-fno-caret-diagnostics " # dont show source with diagnostics
                "-std=c++20 "         # use the c++20 standard
                "-fexceptions "       # enable exception handling
                "-fcxx-exceptions "   # enable c++ exceptions
                "-finline-functions " # inlines suitable functions
                "-pipe "              # use pipes between commands when possible
                "-msse3 "             # enables sse
                "-pthread "
                "-Wno-switch "
                "-Wno-unused-value "  
                "-Wno-implicitly-unsigned-literal "
                "-Wno-nonportable-include-path "
                "-Wno-writable-strings "
                "-Wno-unused-function "
                "-Wno-unused-variable "
                "-Wno-undefined-inline "
                "-Wno-return-type-c-linkage "
                "-Wno-reorder-init-list "
            ),
            "release":(
                "-O2 " # maximizes speed (O1 minimizes size)
            ),
            "debug":(
                "-ggdb3 " # output debug information for gdb
                "-O0 " # disable optimization completely
            ),
            "analyze": {
                True: "--analyze ",
                False: ""
            }
        }
    }
}

# make this more general later idk man 
if config["use_notcurses"]:
    parts["link"]["linux"]["always"].append("notcurses-core")
    parts["link"]["linux"]["always"].append("notcurses")

if config["use_tracy"]:
    parts["defines"]["always"] += "-DAMU_USE_TRACY -DTRACY_ENABLE -DTRACY_NO_EXIT=1 "

# make sure that all the chosen options are compatible
if config["compiler"] not in compatibility[config["platform"]]["compiler"]:
    print(f"compiler {config['compiler']} is not compatible with the platform {config['platform']}")
    quit()
if config["linker"] not in compatibility[config["platform"]]["linker"]: 
    print(f"linker {config['linker']} is not compatible with the platform {config['platform']}")
    quit()

if config["build_analysis"]:
    if config["compiler"] != "clang++":
        print("Build analysis (--ba) is only available with clang.")
        quit()
    import shutil
    if shutil.which("ClangBuildAnalyzer") is None:
        print("Build analysis (--ba) is enabled, but ClangBuildAnalyzer is not installed.")
        quit()
    parts["compiler_flags"]["clang++"]["always"] += "-ftime-trace"

#
#  construct compiler commands
#______________________________________________________________________________________________________________________

header = f'{datetime.now().strftime("%a, %h %d %Y, %H:%M:%S")} ({config["compiler"]}/{config["buildmode"]}) [{app_name}]'
print(header)
print('-'*len(header))

link = {
    "flags": "",
    "libs": "",
    "paths": ""
}

defines = (
    parts["defines"]["buildmode"][config["buildmode"]] +
    parts["defines"]["always"]
)

if config["use_notcurses"]:
    defines += "-DAMU_USE_NOTCURSES -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600"

link_names = (
    parts["link"][config["platform"]]["always"]
)

nameprefix = parts['link']['prefix'][config['linker']]['file']
for ln in link_names:
    link["libs"] += f"{nameprefix}{ln} "

link_paths = parts["link"][config["platform"]]["paths"]
pathprefix = parts['link']['prefix'][config['linker']]['path']
for lp in link_paths:
    link["paths"] += f"{pathprefix}{lp} "

shared = (
    f'{defines} '
    f'{includes} '
    f'{parts["compiler_flags"][config["compiler"]]["always"]} '
    f'{parts["compiler_flags"][config["compiler"]][config["buildmode"]]} '
    f'{parts["compiler_flags"][config["compiler"]]["analyze"][config["static_analysis"]]} '
)

# TODO(sushi) look more into precompiling
# cmd = f"clang -c -xc++-header test.h -o test.h.pch {shared}"
# print(cmd)
# subprocess.Popen(cmd.split(' ')).wait()

if config["use_pch"]:
    f = open("src/pch.h", "r")
    buff = f.read()
    f.close()

    pchlasttime = 0
    if os.path.exists(f"{folders['build']}/pch.h.pch"):
        pchlasttime = os.path.getmtime(f"{folders['build']}/pch.h.pch")

    buff = buff.split("*/")[1]
    includes = list(filter(len, [a.strip().strip('"') for a in buff.split("#include")]))
    # TODO(sushi) this doesn't detect changes to files that may not exist within the 
    #             project's directory like the stdlib, so this should really 
    #             be checking for changes to the list of includes we create 
    def regen_pch():
        for include in includes:
            if os.path.getmtime(f"{include}") > pchlasttime:
                return True
        return False
    if os.path.getmtime("src/pch.h") > pchlasttime or regen_pch():
        subprocess.Popen(f"clang -c -xc++-header -I. src/pch.h -o {folders['build']}/pch.h.pch {shared}".split(' ')).wait()

    shared += f"-include-pch {folders['build']}/pch.h.pch "

full_app = (
    f'{config["compiler"]} -c '
    f'{sources["app"]} ' + shared +
    f'-o {folders["build"]}/{app_name}.o'
)

if config["gen_compcmd"]:
    import re
    shared_args = []
    shared_args.append(config["compiler"])
    for s in shared.split(' '):
        if s and not s.isspace():
            shared_args.append(s)
    out = "["
    cfiles = []
    hfiles = []
    with open(sources["app"], "r") as f:
        includes = re.findall(r'#include\s*?["|<](.*?)["|>]', f.read())
        for include in includes:
            if include.endswith('.cpp'):
                cfiles.append(include)
            else:
                hfiles.append(include)
    includes = ""
    for h in hfiles:
        includes += '"-include' + h + '",\n'
    for c in cfiles:
        out += '{\n"arguments": [\n'
        out += "".join(['"' + arg + '",\n' for arg in shared_args])
        out += includes
        out += '"-c",'
        out += '"' + os.path.abspath(c[0]) + '/' + c[1] + '"\n'
        out += '],'
        out += f'"file": "{os.path.abspath(c[0])}/{c[1]}",'
        out += f'"directory": "{folders["root"]}"}},'
    out = out[0:-1]
    out += "]"
    with open('compile_commands.json', "w") as f:
        f.write(out)

full_link = (
    f'{config["compiler"]} ' # NOTE(sushi) this should not be the compiler, but it is for now cause that is how it is on linux for me so fix later please
    f'{folders["build"]}/{app_name}.o '
    f'{folders["root"]}/misc/fibers/context.o '
    f'{link["flags"]} '
    f'{link["libs"]} '
    f'{link["paths"]} '
    f'-o {folders["build"]}/{app_name} '
)

def format_time(time):
    one_second = 1
    one_minute = 60*one_second
    if not time: return "0 seconds"
    if time > one_minute:
        n_minutes = time//one_minute
        return f"{n_minutes} minute{'s' if n_minutes != 1 else ''} {format_time(time-n_minutes*one_minute)}"
    return f"{time} second{'s' if time != 1 else ''}"

def run_proc(name, cmd):
    if config["verbose"]: print(cmd)
    start = time.time()
    dproc = subprocess.Popen(cmd.split(' '))
    dproc.communicate()
    taken = format_time(round(time.time()-start, 3))
    if not dproc.returncode:
        print(f'  \033[32m{name}\033[0m - {taken}')
    else:
        print(f'  \033[31m{name} failed to build\033[0m - {taken}')

start = time.time()
aproc = Thread(target=run_proc, args=(app_name, full_app))

baproc = None
if config["build_analysis"]:
    subprocess.Popen(f"ClangBuildAnalyzer --start {folders['build']}/".split(' '), stdout=subprocess.PIPE).wait()

aproc.start()
aproc.join()

if config["build_analysis"]:
    subprocess.Popen(f"ClangBuildAnalyzer --stop {folders['build']}/ {folders['build']}/out".split(' '), stdout=subprocess.PIPE).wait()
    proc = subprocess.Popen(f"ClangBuildAnalyzer --analyze {folders['build']}/out".split(' '), stdout=subprocess.PIPE)
    analysis = proc.communicate()[0].decode()
    file = open(f"{folders['build']}/ctimeanalysis", "w")
    file.write(analysis)
    file.close()

lproc = Thread(target=run_proc, args=("exe", full_link))
lproc.start()
lproc.join()

print(f"time: {format_time(round(time.time()-start, 3))}")

