#!/usr/bin/env python3
"""I19 Phase-0b: byte-locked semantics and typed numeric extracts. CI ONLY.

Typed outputs are lossless with respect to *selected numerical values*, NOT
lossless reconstructions of source DLY/JSON/ZIP bytes. UCI heldout is hashed
but never decoded or inspected for structure/values in this phase.
"""
from __future__ import annotations

import argparse
from decimal import Decimal, InvalidOperation
import hashlib
import json
from pathlib import Path
import struct

NOAA_STATION = "USW00003947"
MAX_TYPED_VALUES = 2_000_000
NOAA_ELEMENTS = ("TMAX", "TMIN")


def require(value: bool, message: str) -> None:
    if not value:
        raise ValueError(message)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 16), b""):
            h.update(chunk)
    return h.hexdigest()


def scaled_int32(value: object, scale: int = 1000) -> int:
    """Exact fixed-point conversion, with no float arithmetic/rounding."""
    try:
        d = Decimal(str(value))
    except (InvalidOperation, ValueError) as exc:
        raise ValueError("non-numeric value") from exc
    require(d.is_finite(), "non-finite numeric value")
    n = d * scale
    require(n == n.to_integral_value(), "numeric precision exceeds fixed-point scale")
    number = int(n)
    require(-(1 << 31) <= number < (1 << 31), "fixed-point overflow")
    return number


def emit_ints(values: list[int], width: int) -> bytes:
    require(0 < len(values) <= MAX_TYPED_VALUES, "invalid numerical row count")
    if width == 2:
        require(all(-32768 <= x <= 32767 for x in values), "int16 overflow")
        return struct.pack("<" + str(len(values)) + "h", *values)
    require(width == 4, "invalid packed width")
    return struct.pack("<" + str(len(values)) + "i", *values)


def decode_noaa(raw: bytes) -> dict[str, tuple[bytes, dict]]:
    require(len(raw) > 100 and b"\0" not in raw, "invalid station source")
    try:
        lines = raw.decode("ascii").splitlines()
    except UnicodeDecodeError as exc:
        raise ValueError("invalid non-ASCII NOAA station data") from exc
    require(len(lines) > 24, "NOAA source too short")
    arrays = {name: [] for name in NOAA_ELEMENTS}
    counts = {name: 0 for name in NOAA_ELEMENTS}
    years = []
    for i, line in enumerate(lines):
        require(len(line) == 269, f"NOAA line {i} length not 269")
        require(line[:11] == NOAA_STATION, "NOAA station identity mismatch")
        year, month, element = line[11:15], line[15:17], line[17:21]
        require(year.isdecimal() and month.isdecimal() and 1 <= int(month) <= 12,
                f"NOAA invalid record date at {i}")
        years.append(int(year))
        if element not in arrays:
            continue
        record = []
        for day in range(31):
            field = line[21 + 8 * day:26 + 8 * day]
            try:
                value = int(field)
            except ValueError as exc:
                raise ValueError(f"NOAA non-integral field line={i} day={day}") from exc
            require(-32768 <= value <= 32767, "NOAA value exceeds int16")
            record.append(value)
        arrays[element].extend(record)
        counts[element] += 1
    require(min(years) <= 2000 and max(years) >= 2010, "NOAA historical range insufficient")
    result = {}
    for elem, values in arrays.items():
        require(counts[elem] >= 12 and len(values) >= 372, f"missing NOAA {elem}")
        binary = emit_ints(values, 2)
        result[elem.lower()] = (binary, {
            "dtype": "int16le", "scale": 1, "unit": "0.1 degrees Celsius",
            "element": elem, "source_records": counts[elem], "samples": len(values),
            "missing_sentinel": -9999, "missing_count": values.count(-9999),
            "mapping": "station .dly rows matching element, each 31 daily slots in file order",
            "original_source_reconstructible": False,
        })
    return result


def decode_usgs(raw: bytes) -> dict[str, tuple[bytes, dict]]:
    try:
        doc = json.loads(raw, parse_float=Decimal)
    except (ValueError, UnicodeDecodeError) as exc:
        raise ValueError("USGS invalid JSON") from exc
    require(isinstance(doc, dict) and isinstance(doc.get("value"), dict),
            "USGS missing WaterML root")
    series = doc["value"].get("timeSeries")
    require(isinstance(series, list) and series, "USGS empty timeSeries")
    matched = []
    for item in series:
        if not isinstance(item, dict):
            continue
        codes = item.get("variable", {}).get("variableCode", [])
        if any(isinstance(c, dict) and str(c.get("value")) == "00060" for c in codes):
            matched.append(item)
    require(len(matched) == 1, "USGS expected exactly one discharge series")
    station = matched[0].get("sourceInfo", {}).get("siteCode", [])
    require(any(isinstance(s, dict) and s.get("value") == "01646500" for s in station),
            "USGS wrong monitoring station")
    arrays = matched[0].get("values", [])
    require(isinstance(arrays, list) and arrays, "USGS values missing")
    values, dates = [], []
    for part in arrays:
        require(isinstance(part, dict), "USGS unexpected values shape")
        measurements = part.get("value", [])
        require(isinstance(measurements, list), "USGS invalid observations")
        for item in measurements:
            require(isinstance(item, dict) and "value" in item
                    and isinstance(item.get("dateTime"), str), "USGS invalid observation")
            values.append(scaled_int32(item["value"]))
            dates.append(item["dateTime"])
    require(len(values) >= 365, "USGS unexpectedly few observations")
    require(dates == sorted(dates) and len(set(dates)) == len(dates),
            "USGS unsorted or duplicate times")
    binary = emit_ints(values, 4)
    return {"discharge": (binary, {
        "dtype": "int32le", "scale": 1000, "parameter": "00060",
        "site": "01646500", "samples": len(values), "first_date": dates[0],
        "last_date": dates[-1], "mapping": "WaterML JSON timeSeries[parameter 00060] values in source order",
        "original_source_reconstructible": False,
    })}


def decode_nasa(raw: bytes) -> dict[str, tuple[bytes, dict]]:
    try:
        doc = json.loads(raw, parse_float=Decimal)
    except (ValueError, UnicodeDecodeError) as exc:
        raise ValueError("NASA invalid JSON") from exc
    require(isinstance(doc, dict) and isinstance(doc.get("properties"), dict),
            "NASA missing properties")
    series = doc["properties"].get("parameter", {}).get("T2M")
    require(isinstance(series, dict) and len(series) >= 365, "NASA T2M absent/short")
    dates = sorted(series)
    require(all(len(day) == 8 and day.isdecimal() for day in dates),
            "NASA invalid time-key format")
    require(dates[0] >= "20000101" and dates[-1] <= "20101231",
            "NASA date range unexpectedly changed")
    values = [scaled_int32(series[day]) for day in dates]
    binary = emit_ints(values, 4)
    return {"t2m": (binary, {
        "dtype": "int32le", "scale": 1000, "parameter": "T2M",
        "samples": len(values), "first_date": dates[0], "last_date": dates[-1],
        "mapping": "properties.parameter.T2M values sorted by YYYYMMDD key",
        "original_source_reconstructible": False,
    })}


PARSERS = {
    "noaa_kmci_daily": decode_noaa,
    "usgs_potomac_daily": decode_usgs,
    "nasa_power_t2m_daily": decode_nasa,
}


def validate_and_extract(lock: dict, artifacts: Path, output: Path) -> dict:
    require(lock.get("schema") == "anvil.i19.source-byte-lock.v1"
            and lock.get("efficacy_authorized") is False
            and lock.get("heldout_unsealed") is True, "invalid science gates")
    require(lock.get("status") == "SOURCE_BYTES_FROZEN_EFFICACY_NOT_AUTHORIZED",
            "unrecognized source byte-lock state")
    require(not output.exists() or not any(output.iterdir()), "output must be empty")
    candidate = json.loads((artifacts / "snapshot/candidate-manifest.UNFROZEN.json").read_text("utf-8"))
    require(candidate["frozen"] is False, "prematurely admitted candidate manifest")
    by_id = {s["id"]: s for s in candidate["sources"]}
    require(len(by_id) == len(lock["sources"]) == 4, "source roster mismatch")
    output.mkdir(parents=True, exist_ok=True)
    report = {"schema": "anvil.i19.typed-preparation.v1",
              "origin_run_id": lock["origin_run_id"],
              "origin_commit": lock["origin_commit"],
              "status": "SEMANTIC_VALIDATION_IN_PROGRESS",
              "codec_efficacy_measured": False, "heldout_unsealed": True,
              "promotion_authorized": False, "outputs": []}
    try:
        for source in lock["sources"]:
            identifier = source["id"]
            require(identifier in by_id and source["split"] == by_id[identifier]["split"]
                    and source["sha256"] == by_id[identifier]["sha256"]
                    and source["source_bytes"] == by_id[identifier]["source_bytes"],
                    f"byte-lock mismatch: {identifier}")
            path = artifacts / "snapshot" / "raw" / (identifier + ".source")
            require(path.is_file() and path.stat().st_size == source["source_bytes"]
                    and sha256(path) == source["sha256"],
                    f"snapshot digest mismatch: {identifier}")
            if source["split"] == "heldout":
                require(identifier == "uci_household_electricity",
                        "unexpected heldout origin")
                report["outputs"].append({
                    "source_id": identifier, "split": "heldout",
                    "status": "OPAQUE_UNPARSED_SHA_ONLY",
                    "source_sha256": source["sha256"],
                    "source_bytes": source["source_bytes"]})
                continue
            require(identifier in PARSERS, f"missing semantic parser: {identifier}")
            parsed = PARSERS[identifier](path.read_bytes())
            for name, (binary, details) in parsed.items():
                require(details["original_source_reconstructible"] is False,
                        "typed output cannot claim reconstructible source bytes")
                outfile = output / "typed" / (identifier + "__" + name + ".bin")
                outfile.parent.mkdir(parents=True, exist_ok=True)
                require(not outfile.exists(), "duplicate typed output")
                outfile.write_bytes(binary)
                report["outputs"].append({
                    "source_id": identifier, "split": source["split"],
                    "status": "TYPED_SELECTION_ONLY",
                    "source_sha256": source["sha256"],
                    "typed_path": "typed/" + outfile.name,
                    "typed_sha256": sha256(outfile), "typed_bytes": len(binary),
                    **details})
        report["status"] = "THREE_ORIGIN_TYPED_SELECTIONS_READY_UNADMITTED"
    except Exception as exc:
        report["status"] = "BLOCKED_SEMANTIC_VALIDATION"
        report["error"] = f"{type(exc).__name__}: {exc}"
        raise
    finally:
        (output / "semantic-report.json").write_text(
            json.dumps(report, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    return report


def main() -> None:
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("--lock", type=Path, required=True)
    cli.add_argument("--artifact", type=Path, required=True)
    cli.add_argument("--out", type=Path, required=True)
    args = cli.parse_args()
    lock = json.loads(args.lock.read_text("utf-8"))
    report = validate_and_extract(lock, args.artifact, args.out)
    print(json.dumps({"status": report["status"], "count": len(report["outputs"]),
                      "heldout_unsealed": True, "efficacy": False}))


if __name__ == "__main__":
    main()
