#!/usr/bin/env python3
"""Low-poll-rate Actions run watcher for several runs at once.

usage: tmp_watch_run.py <run_id> [<run_id> ...] [interval_s]
Keeps the shared GitHub API rate limit sane.
"""
import json
import subprocess
import sys
import time

args = sys.argv[1:]
interval = 240
if args and args[-1].isdigit() and len(args) > 1:
    interval = int(args[-1])
    args = args[:-1]
run_ids = args
deadline = time.time() + 21600

last = {}
while time.time() < deadline:
    for run_id in run_ids:
        try:
            p = subprocess.run(
                ["gh", "run", "view", run_id, "--repo", "thelabcorner/anvil",
                 "--json", "status,conclusion,jobs,headSha,url,workflowName"],
                capture_output=True, text=True, timeout=120,
            )
            d = json.loads(p.stdout)
            jobs = {j.get("name"): (j.get("conclusion") or j.get("status"))
                    for j in (d.get("jobs") or [])}
            sig = (d.get("status"), d.get("conclusion"), json.dumps(jobs, sort_keys=True))
            if last.get(run_id) != sig:
                print(f"[{time.strftime('%H:%M:%S')}] run={run_id} status={sig[0]} "
                      f"conclusion={sig[1]} jobs={jobs}", flush=True)
                last[run_id] = sig
            if d.get("status") == "completed":
                print(f"DONE run={run_id} url={d.get('url')} headSha={d.get('headSha')} "
                      f"workflow={d.get('workflowName')}", flush=True)
        except Exception as exc:  # noqa: BLE001
            print(f"[{time.strftime('%H:%M:%S')}] run={run_id} watch error: {exc}", flush=True)
    time.sleep(interval)
print("TIMEOUT", flush=True)
