#!/usr/bin/env python3
import sys
import os
import json
import subprocess
import shutil

STAGES = [
  {
    "index": 1,
    "slug": "oo8",
    "tester_log_prefix": "stage-1",
    "title": "Stage #1: Print a prompt"
  },
  {
    "index": 2,
    "slug": "cz2",
    "tester_log_prefix": "stage-2",
    "title": "Stage #2: Handle invalid commands"
  },
  {
    "index": 3,
    "slug": "ff0",
    "tester_log_prefix": "stage-3",
    "title": "Stage #3: REPL"
  },
  {
    "index": 4,
    "slug": "pn5",
    "tester_log_prefix": "stage-4",
    "title": "Stage #4: The exit builtin"
  },
  {
    "index": 5,
    "slug": "iz3",
    "tester_log_prefix": "stage-5",
    "title": "Stage #5: The echo builtin"
  },
  {
    "index": 6,
    "slug": "ez5",
    "tester_log_prefix": "stage-6",
    "title": "Stage #6: The type builtin: builtins"
  },
  {
    "index": 7,
    "slug": "mg5",
    "tester_log_prefix": "stage-7",
    "title": "Stage #7: The type builtin: executable files"
  },
  {
    "index": 8,
    "slug": "ip1",
    "tester_log_prefix": "stage-8",
    "title": "Stage #8: Run a program"
  },
  {
    "index": 9,
    "slug": "ei0",
    "tester_log_prefix": "stage-9",
    "title": "Stage #9: The pwd builtin"
  },
  {
    "index": 10,
    "slug": "ra6",
    "tester_log_prefix": "stage-10",
    "title": "Stage #10: The cd builtin: Absolute paths"
  },
  {
    "index": 11,
    "slug": "gq9",
    "tester_log_prefix": "stage-11",
    "title": "Stage #11: The cd builtin: Relative paths"
  },
  {
    "index": 12,
    "slug": "gp4",
    "tester_log_prefix": "stage-12",
    "title": "Stage #12: The cd builtin: Home directory"
  },
  {
    "index": 13,
    "slug": "ni6",
    "tester_log_prefix": "stage-13",
    "title": "Stage #13: Single quotes"
  },
  {
    "index": 14,
    "slug": "tg6",
    "tester_log_prefix": "stage-14",
    "title": "Stage #14: Double quotes"
  },
  {
    "index": 15,
    "slug": "yt5",
    "tester_log_prefix": "stage-15",
    "title": "Stage #15: Backslash outside quotes"
  },
  {
    "index": 16,
    "slug": "le5",
    "tester_log_prefix": "stage-16",
    "title": "Stage #16: Backslash within single quotes"
  },
  {
    "index": 17,
    "slug": "gu3",
    "tester_log_prefix": "stage-17",
    "title": "Stage #17: Backslash within double quotes"
  },
  {
    "index": 18,
    "slug": "qj0",
    "tester_log_prefix": "stage-18",
    "title": "Stage #18: Executing a quoted executable"
  },
  {
    "index": 19,
    "slug": "jv1",
    "tester_log_prefix": "stage-19",
    "title": "Stage #19: Redirect stdout"
  },
  {
    "index": 20,
    "slug": "vz4",
    "tester_log_prefix": "stage-20",
    "title": "Stage #20: Redirect stderr"
  },
  {
    "index": 21,
    "slug": "el9",
    "tester_log_prefix": "stage-21",
    "title": "Stage #21: Append stdout"
  },
  {
    "index": 22,
    "slug": "un3",
    "tester_log_prefix": "stage-22",
    "title": "Stage #22: Append stderr"
  },
  {
    "index": 23,
    "slug": "qp2",
    "tester_log_prefix": "stage-23",
    "title": "Stage #23: Builtin completion"
  },
  {
    "index": 24,
    "slug": "gm9",
    "tester_log_prefix": "stage-24",
    "title": "Stage #24: Completion with arguments"
  },
  {
    "index": 25,
    "slug": "qm8",
    "tester_log_prefix": "stage-25",
    "title": "Stage #25: Missing completions"
  },
  {
    "index": 26,
    "slug": "gy5",
    "tester_log_prefix": "stage-26",
    "title": "Stage #26: Executable completion"
  },
  {
    "index": 27,
    "slug": "wh6",
    "tester_log_prefix": "stage-27",
    "title": "Stage #27: Multiple completions"
  },
  {
    "index": 28,
    "slug": "wt6",
    "tester_log_prefix": "stage-28",
    "title": "Stage #28: Partial completions"
  },
  {
    "index": 29,
    "slug": "zv2",
    "tester_log_prefix": "stage-29",
    "title": "Stage #29: File completion"
  },
  {
    "index": 30,
    "slug": "ue6",
    "tester_log_prefix": "stage-30",
    "title": "Stage #30: Nested file completion"
  },
  {
    "index": 31,
    "slug": "lc6",
    "tester_log_prefix": "stage-31",
    "title": "Stage #31: Directory completion"
  },
  {
    "index": 32,
    "slug": "vs5",
    "tester_log_prefix": "stage-32",
    "title": "Stage #32: Missing entry completion"
  },
  {
    "index": 33,
    "slug": "no5",
    "tester_log_prefix": "stage-33",
    "title": "Stage #33: Multiple matches"
  },
  {
    "index": 34,
    "slug": "jp8",
    "tester_log_prefix": "stage-34",
    "title": "Stage #34: Partial filename completions"
  },
  {
    "index": 35,
    "slug": "bf8",
    "tester_log_prefix": "stage-35",
    "title": "Stage #35: Multi-argument completions"
  },
  {
    "index": 36,
    "slug": "ne7",
    "tester_log_prefix": "stage-36",
    "title": "Stage #36: Register complete builtin"
  },
  {
    "index": 37,
    "slug": "oi7",
    "tester_log_prefix": "stage-37",
    "title": "Stage #37: Printing missing specifications"
  },
  {
    "index": 38,
    "slug": "wl6",
    "tester_log_prefix": "stage-38",
    "title": "Stage #38: Displaying registered specifications"
  },
  {
    "index": 39,
    "slug": "pm5",
    "tester_log_prefix": "stage-39",
    "title": "Stage #39: Single completion"
  },
  {
    "index": 40,
    "slug": "qf1",
    "tester_log_prefix": "stage-40",
    "title": "Stage #40: Handling no completions"
  },
  {
    "index": 41,
    "slug": "zi0",
    "tester_log_prefix": "stage-41",
    "title": "Stage #41: Passing command-line arguments"
  },
  {
    "index": 42,
    "slug": "nr7",
    "tester_log_prefix": "stage-42",
    "title": "Stage #42: Passing environment variables"
  },
  {
    "index": 43,
    "slug": "ep2",
    "tester_log_prefix": "stage-43",
    "title": "Stage #43: Multiple completer candidates"
  },
  {
    "index": 44,
    "slug": "xz3",
    "tester_log_prefix": "stage-44",
    "title": "Stage #44: Longest common prefix"
  },
  {
    "index": 45,
    "slug": "tz2",
    "tester_log_prefix": "stage-45",
    "title": "Stage #45: Unregister a completion"
  },
  {
    "index": 46,
    "slug": "af3",
    "tester_log_prefix": "stage-46",
    "title": "Stage #46: The jobs builtin"
  },
  {
    "index": 47,
    "slug": "at7",
    "tester_log_prefix": "stage-47",
    "title": "Stage #47: Starting background job"
  },
  {
    "index": 48,
    "slug": "si2",
    "tester_log_prefix": "stage-48",
    "title": "Stage #48: Printing background job output"
  },
  {
    "index": 49,
    "slug": "jd6",
    "tester_log_prefix": "stage-49",
    "title": "Stage #49: List a single job"
  },
  {
    "index": 50,
    "slug": "dk5",
    "tester_log_prefix": "stage-50",
    "title": "Stage #50: List multiple jobs"
  },
  {
    "index": 51,
    "slug": "ma9",
    "tester_log_prefix": "stage-51",
    "title": "Stage #51: Reaping one job using jobs"
  },
  {
    "index": 52,
    "slug": "rq2",
    "tester_log_prefix": "stage-52",
    "title": "Stage #52: Reap multiple jobs using jobs"
  },
  {
    "index": 53,
    "slug": "bv8",
    "tester_log_prefix": "stage-53",
    "title": "Stage #53: Reap before the next prompt"
  },
  {
    "index": 54,
    "slug": "fy4",
    "tester_log_prefix": "stage-54",
    "title": "Stage #54: Job number reset"
  },
  {
    "index": 55,
    "slug": "br6",
    "tester_log_prefix": "stage-55",
    "title": "Stage #55: Dual-command pipeline"
  },
  {
    "index": 56,
    "slug": "ny9",
    "tester_log_prefix": "stage-56",
    "title": "Stage #56: Pipelines with built-ins"
  },
  {
    "index": 57,
    "slug": "xk3",
    "tester_log_prefix": "stage-57",
    "title": "Stage #57: Multi-command pipelines"
  },
  {
    "index": 58,
    "slug": "bq4",
    "tester_log_prefix": "stage-58",
    "title": "Stage #58: The history builtin"
  },
  {
    "index": 59,
    "slug": "yf5",
    "tester_log_prefix": "stage-59",
    "title": "Stage #59: Listing history"
  },
  {
    "index": 60,
    "slug": "ag6",
    "tester_log_prefix": "stage-60",
    "title": "Stage #60: Limiting history entries"
  },
  {
    "index": 61,
    "slug": "rh7",
    "tester_log_prefix": "stage-61",
    "title": "Stage #61: Up-arrow navigation"
  },
  {
    "index": 62,
    "slug": "vq0",
    "tester_log_prefix": "stage-62",
    "title": "Stage #62: Down-arrow navigation"
  },
  {
    "index": 63,
    "slug": "dm2",
    "tester_log_prefix": "stage-63",
    "title": "Stage #63: Executing commands from history"
  },
  {
    "index": 64,
    "slug": "za2",
    "tester_log_prefix": "stage-64",
    "title": "Stage #64: Read history from file"
  },
  {
    "index": 65,
    "slug": "in3",
    "tester_log_prefix": "stage-65",
    "title": "Stage #65: Write history to file"
  },
  {
    "index": 66,
    "slug": "sx3",
    "tester_log_prefix": "stage-66",
    "title": "Stage #66: Append history to file"
  },
  {
    "index": 67,
    "slug": "zp4",
    "tester_log_prefix": "stage-67",
    "title": "Stage #67: Read history on startup"
  },
  {
    "index": 68,
    "slug": "kz7",
    "tester_log_prefix": "stage-68",
    "title": "Stage #68: Write history on exit"
  },
  {
    "index": 69,
    "slug": "jv2",
    "tester_log_prefix": "stage-69",
    "title": "Stage #69: jv2"
  },
  {
    "index": 70,
    "slug": "ji0",
    "tester_log_prefix": "stage-70",
    "title": "Stage #70: The declare builtin"
  },
  {
    "index": 71,
    "slug": "oa2",
    "tester_log_prefix": "stage-71",
    "title": "Stage #71: Printing non-existing variables"
  },
  {
    "index": 72,
    "slug": "kv5",
    "tester_log_prefix": "stage-72",
    "title": "Stage #72: Storing shell variables"
  },
  {
    "index": 73,
    "slug": "db8",
    "tester_log_prefix": "stage-73",
    "title": "Stage #73: Validating shell variable names"
  },
  {
    "index": 74,
    "slug": "ge9",
    "tester_log_prefix": "stage-74",
    "title": "Stage #74: Expanding variables"
  },
  {
    "index": 75,
    "slug": "br2",
    "tester_log_prefix": "stage-75",
    "title": "Stage #75: Expansion with braces"
  },
  {
    "index": 76,
    "slug": "my0",
    "tester_log_prefix": "stage-76",
    "title": "Stage #76: Expansion when a name is unset"
  }
]


def find_tester_binary():
    custom_path = os.environ.get("CODECRAFTERS_TESTER_BIN")
    if custom_path and os.path.isfile(custom_path) and os.access(custom_path, os.X_OK):
        return custom_path
    
    local_bin = os.path.expanduser("~/.local/bin/codecrafters-shell-tester")
    if os.path.isfile(local_bin) and os.access(local_bin, os.X_OK):
        return local_bin
    
    which_bin = shutil.which("codecrafters-shell-tester")
    if which_bin:
        return which_bin
        
    tmp_bin = "/tmp/shell-tester/tester"
    if os.path.isfile(tmp_bin) and os.access(tmp_bin, os.X_OK):
        return tmp_bin
        
    return None

def print_help():
    print("CodeCrafters Shell Challenge - Local Stage Tester")
    print("==================================================")
    print("Usage:")
    print("  ./test_local.sh <stage_number_or_slug>      Run a single stage")
    print("  ./test_local.sh --up-to <stage_number>      Run all stages up to the given stage")
    print("  ./test_local.sh --all                       Run all challenge stages")
    print("  ./test_local.sh --list                      List all stages and slugs")
    print("  ./test_local.sh --help                      Show this help message")
    print("")
    print("Examples:")
    print("  ./test_local.sh 1                           # Test stage 1")
    print("  ./test_local.sh --up-to 8                   # Test base stages 1 through 8")
    print("  ./test_local.sh ip1                         # Test stage by slug")

def list_stages():
    print("{:<5} | {:<6} | {}".format("Stage", "Slug", "Title"))
    print("-" * 65)
    for s in STAGES:
        print("{:<5} | {:<6} | {}".format(s["index"], s["slug"], s["title"]))

def main():
    if len(sys.argv) < 2 or sys.argv[1] in ("-h", "--help"):
        print_help()
        sys.exit(0)

    if sys.argv[1] in ("-l", "--list"):
        list_stages()
        sys.exit(0)

    tester_bin = find_tester_binary()
    if not tester_bin:
        print("Error: codecrafters-shell-tester binary not found!", file=sys.stderr)
        print("Please build it first or place it in ~/.local/bin/codecrafters-shell-tester", file=sys.stderr)
        sys.exit(1)

    repo_dir = os.path.abspath(os.path.dirname(__file__))

    selected_stages = []
    
    if sys.argv[1] in ("-a", "--all"):
        selected_stages = STAGES
    elif sys.argv[1] in ("-u", "--up-to", "-p", "--previous"):
        if len(sys.argv) < 3:
            print("Error: --up-to requires a stage number or slug", file=sys.stderr)
            sys.exit(1)
        target = sys.argv[2]
        target_idx = None
        if target.isdigit():
            target_idx = int(target)
        else:
            for s in STAGES:
                if s["slug"] == target:
                    target_idx = s["index"]
                    break
        if not target_idx or target_idx < 1 or target_idx > len(STAGES):
            print(f"Error: Invalid stage '{target}'. Available stages: 1 to {len(STAGES)}", file=sys.stderr)
            sys.exit(1)
        selected_stages = STAGES[:target_idx]
    else:
        target = sys.argv[1]
        matched = None
        if target.isdigit():
            target_num = int(target)
            if 1 <= target_num <= len(STAGES):
                matched = STAGES[target_num - 1]
        else:
            for s in STAGES:
                if s["slug"] == target:
                    matched = s
                    break
        if not matched:
            print(f"Error: Stage '{target}' not recognized. Run with --list to see stages.", file=sys.stderr)
            sys.exit(1)
        selected_stages = [matched]

    stage_names = [f"#{s['index']} ({s['slug']})" for s in selected_stages]
    summary_str = ", ".join(stage_names[:5]) + (" ..." if len(stage_names) > 5 else "")
    print(f">> Running tests for {len(selected_stages)} stage(s): {summary_str}")

    test_cases_payload = [
        {
            "slug": s["slug"],
            "tester_log_prefix": s["tester_log_prefix"],
            "title": s["title"]
        }
        for s in selected_stages
    ]

    env = os.environ.copy()
    env["CODECRAFTERS_REPOSITORY_DIR"] = repo_dir
    env["CODECRAFTERS_TEST_CASES_JSON"] = json.dumps(test_cases_payload)

    tester_data_dir = os.environ.get("TESTER_DIR")
    if not tester_data_dir:
        local_share = os.path.expanduser("~/.local/share/codecrafters-shell-tester")
        if os.path.isdir(os.path.join(local_share, "built_executables")):
            tester_data_dir = local_share
        elif os.path.isdir("/tmp/shell-tester/built_executables"):
            tester_data_dir = "/tmp/shell-tester"

    if tester_data_dir:
        env["TESTER_DIR"] = tester_data_dir

    proc = subprocess.run([tester_bin], env=env)
    sys.exit(proc.returncode)

if __name__ == "__main__":
    main()
