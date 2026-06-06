#!/usr/bin/env python3
"""
Image filter via subprocess bisect:
  Each subprocess runs PIL (with warnings-as-errors) + cv2 on a batch of images.
  Any image producing a warning or error (Python-level or C-level stderr) is removed.

Usage:
    python3 catch_non_silent.py --img-list <file.txt> --out-list <file_cleaned.txt>
                                [--batch 2000] [--workers 16]

--img-list   : input file, one absolute image path per line
--out-list   : output cleaned list (all bad/warn images removed)

Side-effect output files (written next to --out-list):
    bad_clevel.txt   : images with errors or non-harmless warnings
    warn_clevel.txt  : images with harmless warnings only (e.g. libpng iCCP)
"""

import os
import sys
import subprocess
import threading
import time
import argparse
import tempfile
from concurrent.futures import ThreadPoolExecutor, as_completed

# ── subprocess reader + checker ────────────────────────────────────────────────

_READER_CODE = """\
import sys, warnings, cv2
from PIL import Image, ImageOps
from io import BytesIO

for path in sys.argv[1:]:
    try:
        with open(path, 'rb') as f:
            data = f.read()
    except Exception as e:
        print(f"read_error:{e}", file=sys.stderr)
        sys.exit(1)

    try:
        with warnings.catch_warnings():
            warnings.simplefilter('error')
            img = Image.open(BytesIO(data))
            img.verify()
    except Exception as e:
        print(f"pil_error:{e}", file=sys.stderr)
        sys.exit(1)

    try:
        with warnings.catch_warnings():
            warnings.simplefilter('error')
            img = Image.open(BytesIO(data))
            img.load()
            ImageOps.exif_transpose(img)
    except Exception as e:
        print(f"pil_error:{e}", file=sys.stderr)
        sys.exit(1)

    try:
        with warnings.catch_warnings():
            warnings.simplefilter('error')
            arr = cv2.imread(path, cv2.IMREAD_UNCHANGED)
    except Warning as e:
        print(f"cv2_warning:{e}", file=sys.stderr)
        sys.exit(1)
    if arr is None:
        print(f"cv2_none:{path}", file=sys.stderr)
        sys.exit(1)

sys.exit(0)
"""

_HARMLESS_PATTERNS = (
    "libpng warning: iCCP:",
    "libpng warning: iCCP ",
)


def _filter_stderr(stderr):
    lines = [l for l in stderr.splitlines() if l.strip()]
    return "\n".join(lines).strip()


def _is_harmless_stderr(stderr):
    lines = [l for l in stderr.splitlines() if l.strip()]
    if not lines:
        return True
    return all(any(l.startswith(p) for p in _HARMLESS_PATTERNS) for l in lines)


def _is_harmless_reason(reason):
    if not reason.startswith("c_warning_or_error:"):
        return False
    return _is_harmless_stderr(reason[len("c_warning_or_error:"):])


def _run_batch(reader_script, paths, timeout=60):
    result = subprocess.run(
        [sys.executable, reader_script] + paths,
        capture_output=True,
        timeout=timeout,
    )
    stderr = _filter_stderr(result.stderr.decode(errors="replace"))
    rc = result.returncode
    ok = (rc == 0 and not stderr)
    return ok, rc, stderr


def _bisect_bad(reader_script, paths, found):
    if not paths:
        return

    if len(paths) == 1:
        ext = os.path.splitext(paths[0])[1].lower()
        n_tries = 1 if ext == ".png" else 3
        worst_ok, worst_rc, worst_stderr = True, 0, ""
        for _ in range(n_tries):
            try:
                ok, rc, stderr = _run_batch(reader_script, paths, timeout=15)
            except subprocess.TimeoutExpired:
                found.append((paths[0], "crash:timeout"))
                return
            if not ok:
                worst_ok, worst_rc, worst_stderr = ok, rc, stderr
                break
        if not worst_ok:
            if worst_rc < 0:
                import signal as _sig
                try:
                    name = _sig.Signals(-worst_rc).name
                except Exception:
                    name = str(-worst_rc)
                found.append((paths[0], f"crash:signal {name}({-worst_rc})"))
            elif worst_stderr:
                found.append((paths[0], f"c_warning_or_error:{worst_stderr[:300]}"))
            else:
                found.append((paths[0], f"error:exit {worst_rc}"))
        return

    ok = False
    for _ in range(2):
        try:
            ok, rc, stderr = _run_batch(reader_script, paths,
                                        timeout=max(60, len(paths) * 0.05))
        except subprocess.TimeoutExpired:
            ok = False
            break
        if not ok:
            break

    if ok:
        return

    mid = len(paths) // 2
    _bisect_bad(reader_script, paths[:mid], found)
    _bisect_bad(reader_script, paths[mid:], found)


def pass2(paths, out_dir, batch_size, n_workers):
    print(f"[catch_non_silent] scanning {len(paths)} images | batch={batch_size} workers={n_workers}", flush=True)

    reader_fd, reader_script = tempfile.mkstemp(suffix="_reader.py", prefix="cnsilent_")
    try:
        with os.fdopen(reader_fd, "w") as f:
            f.write(_READER_CODE)

        bad_clevel  = []
        warn_clevel = []
        lock        = threading.Lock()
        batches     = [paths[i:i + batch_size] for i in range(0, len(paths), batch_size)]
        n_batches   = len(batches)
        done_count  = [0]
        t0          = time.time()

        def _process_batch(batch):
            try:
                ok, rc, stderr = _run_batch(reader_script, batch,
                                            timeout=max(60, len(batch) * 0.05))
            except subprocess.TimeoutExpired:
                ok, rc, stderr = False, -1, "timeout"

            bad_results  = []
            warn_results = []
            if not ok:
                found = []
                _bisect_bad(reader_script, batch, found)
                for p, reason in found:
                    if _is_harmless_reason(reason):
                        warn_results.append(f"{p}\t{reason}")
                        print(f"  warn: {os.path.basename(p)}  {reason}", flush=True)
                    else:
                        bad_results.append(f"{p}\t{reason}")
                        print(f"  bad:  {os.path.basename(p)}  {reason}", flush=True)
            return bad_results, warn_results

        with ThreadPoolExecutor(max_workers=n_workers) as ex:
            futures = {ex.submit(_process_batch, b): b for b in batches}
            for fut in as_completed(futures):
                bad, warn = fut.result()
                with lock:
                    bad_clevel.extend(bad)
                    warn_clevel.extend(warn)
                    done_count[0] += 1
                    if done_count[0] % 50 == 0:
                        elapsed = (time.time() - t0) / 60
                        print(f"  {done_count[0]}/{n_batches} batches | "
                              f"{elapsed:.1f}min | bad={len(bad_clevel)} warn={len(warn_clevel)}",
                              flush=True)
    finally:
        try:
            os.unlink(reader_script)
        except OSError:
            pass

    elapsed = (time.time() - t0) / 60
    print(f"[catch_non_silent] done in {elapsed:.1f}min | "
          f"bad={len(bad_clevel)} warn={len(warn_clevel)}", flush=True)
    return bad_clevel, warn_clevel


# ── main ───────────────────────────────────────────────────────────────────────

def scan(image_list, out_dir, batch_size=2000, n_workers=16):
    os.makedirs(out_dir, exist_ok=True)

    with open(image_list) as f:
        paths = [l.strip() for l in f if l.strip()]
    total = len(paths)
    print(f"[catch_non_silent] {total} images | batch={batch_size} workers={n_workers}",
          flush=True)

    bad_clevel, warn_clevel = pass2(paths, out_dir, batch_size, n_workers)

    for name, entries in [("bad_clevel.txt", bad_clevel), ("warn_clevel.txt", warn_clevel)]:
        p = os.path.join(out_dir, name)
        with open(p, "w") as f:
            f.write("\n".join(entries))
        print(f"  → {p}  ({len(entries)} entries)", flush=True)

    drop_set = {line.split("\t")[0] for line in bad_clevel + warn_clevel}
    return paths, drop_set, bad_clevel, warn_clevel


# ── CLI ────────────────────────────────────────────────────────────────────────

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--img-list",  required=True, help="input: one image path per line")
    ap.add_argument("--out-list",  required=True, help="output: cleaned image list")
    ap.add_argument("--batch",   type=int, default=2000, help="batch size (default 2000)")
    ap.add_argument("--workers", type=int, default=16,   help="parallel workers (default 16)")
    args = ap.parse_args()

    if not os.path.isfile(args.img_list):
        print(f"[error] file not found: {args.img_list}", file=sys.stderr)
        sys.exit(1)

    out_dir = os.path.dirname(os.path.abspath(args.out_list))
    paths, drop_set, bad_clevel, warn_clevel = scan(
        args.img_list, out_dir, batch_size=args.batch, n_workers=args.workers
    )

    cleaned = [p for p in paths if p not in drop_set]
    with open(args.out_list, "w") as f:
        f.write("\n".join(cleaned))
    n_drop = len(paths) - len(cleaned)
    print(f"  → {args.out_list}  ({len(cleaned)}/{len(paths)} kept, {n_drop} removed: "
          f"bad={len(bad_clevel)} warn={len(warn_clevel)})")


if __name__ == "__main__":
    main()
