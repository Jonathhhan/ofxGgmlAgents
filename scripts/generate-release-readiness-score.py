#!/usr/bin/env python3
"""Thin wrapper: delegates to the PowerShell release readiness generator."""
import argparse
import os
import shutil
import subprocess
import sys


def find_powershell():
    for candidate in ("pwsh", "powershell"):
        path = shutil.which(candidate)
        if path:
            return path
    return None


parser = argparse.ArgumentParser(description="Generate the ofxGgmlAgents release readiness score.")
parser.add_argument("--text", action="store_true", help="Print the human-readable score instead of JSON.")
parser.add_argument("--json", action="store_true", help="Print JSON output. This is the default.")
parser.add_argument("--summary-only", action="store_true", help="Pass through to the PowerShell scorer.")
parser.add_argument("--markdown", action="store_true", help="Write docs/release-readiness-score.md.")
parser.add_argument("--output-path", default="", help="Optional output path for JSON or Markdown.")
args = parser.parse_args()

script_dir = os.path.dirname(os.path.abspath(__file__))
ps1 = os.path.join(script_dir, "generate-release-readiness-score.ps1")
ps_exe = find_powershell()
if not ps_exe:
    print("PowerShell 7+ or Windows PowerShell is required.", file=sys.stderr)
    sys.exit(1)

ps_args = [ps_exe, "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", ps1]
if not args.text:
    ps_args.append("-Json")
if args.summary_only:
    ps_args.append("-SummaryOnly")
if args.markdown:
    output_path = args.output_path or os.path.join(script_dir, "..", "docs", "release-readiness-score.md")
    ps_args.extend(["-OutputPath", output_path])
elif args.output_path:
    ps_args.extend(["-OutputPath", args.output_path])

result = subprocess.run(ps_args, capture_output=True, text=True, cwd=script_dir)
if result.stdout:
    print(result.stdout, end="")
if result.stderr:
    print(result.stderr, file=sys.stderr)
sys.exit(result.returncode)
