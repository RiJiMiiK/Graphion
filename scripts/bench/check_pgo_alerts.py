#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import sys


def load_report(path: pathlib.Path) -> dict[str, object]:
    return json.loads(path.read_text(encoding="utf-8"))


def summarize(report: dict[str, object]) -> tuple[list[str], list[str]]:
    rows = report["report_rows"]
    hard_failures: list[str] = []
    advisories: list[str] = []
    below_minimum = 0

    for row in rows:
        benchmark = str(row["benchmark"])
        speedup = float(row["speedup_x"])
        status = str(row["threshold_status"])
        minimum = float(row["minimum_speedup_x"])
        family = str(row["threshold_family"])

        if status == "below":
            below_minimum += 1
            advisories.append(
                f"{benchmark}: below minimum threshold ({speedup:.3f}x < {minimum:.3f}x) in family {family}"
            )

        if benchmark == "vm_dispatch" and speedup < minimum:
            hard_failures.append(
                f"{benchmark}: release-candidate alert triggered because dispatch fell below minimum ({speedup:.3f}x < {minimum:.3f}x)"
            )

        if speedup < 0.95:
            hard_failures.append(
                f"{benchmark}: severe regression detected ({speedup:.3f}x < 0.950x)"
            )

    if below_minimum >= 3:
        hard_failures.append(
            f"release-candidate alert triggered because {below_minimum} benchmark families fell below minimum thresholds"
        )

    return hard_failures, advisories


def main() -> int:
    parser = argparse.ArgumentParser(description="Evaluate PGO/non-PGO release-candidate alert policy.")
    parser.add_argument("--report-json", required=True, help="Optimization report JSON produced by generate_optimization_report.py")
    parser.add_argument("--mode", choices=["advisory", "release-candidate"], default="release-candidate",
                        help="Alert mode; release-candidate returns non-zero on hard failures")
    args = parser.parse_args()

    report = load_report(pathlib.Path(args.report_json))
    hard_failures, advisories = summarize(report)

    print("pgo alert policy summary:")
    print(f"  advisories: {len(advisories)}")
    print(f"  hard_failures: {len(hard_failures)}")

    if advisories:
        print("advisories:")
        for item in advisories:
            print(f"  - {item}")

    if hard_failures:
        print("hard failures:")
        for item in hard_failures:
            print(f"  - {item}")

    if args.mode == "release-candidate" and hard_failures:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
