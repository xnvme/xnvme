import importlib.util
from pathlib import Path

SCRIPT = Path(__file__).parents[2] / "scripts" / "benchmark_threshold.py"
spec = importlib.util.spec_from_file_location("benchmark_threshold", SCRIPT)
threshold = importlib.util.module_from_spec(spec)
spec.loader.exec_module(threshold)

POINTS = [(4096, depth) for depth in (1, 2, 4, 8, 16)] + [(512, 1)]


def run(iops, groups=None, repetitions=1):
    """Normalized results: 'iops' maps a label to its IOPS at each of POINTS,
    a value or a function of the point and the repetition"""

    groups = groups or {}
    return [
        (
            f"{label}{size}{depth}{rep}",
            {
                "ctx": {
                    "label": label,
                    "group": groups.get(label, "io_uring"),
                    "iosize": size,
                    "iodepth": depth,
                },
                "iops": value(size, depth, rep) if callable(value) else value,
            },
        )
        for label, value in iops.items()
        for size, depth in POINTS
        for rep in range(repetitions)
    ]


FLOORS = {"xnvme_io_uring/io_uring": 0.95, "xnvme_null/null": 0.95}


def test_engines_on_par_pass():
    assert threshold.check(run({"io_uring": 1000, "xnvme_io_uring": 980}), FLOORS) == []


def test_an_engine_a_twentieth_slower_fails():
    problems = threshold.check(run({"io_uring": 1000, "xnvme_io_uring": 940}), FLOORS)
    assert problems == ["xnvme_io_uring/io_uring: 0.940 < 0.95"]


def test_one_slow_point_of_several_passes():
    def slow(size, depth, rep):
        return 800 if depth == 2 else 1000

    assert (
        threshold.check(run({"io_uring": 1000, "xnvme_io_uring": slow}), FLOORS) == []
    )


def test_a_pair_without_a_floor_fails():
    problems = threshold.check(run({"libaio": 1000, "xnvme_libaio": 1000}), FLOORS)
    assert problems == ["xnvme_libaio/libaio: 1.000, and no floor"]


def test_an_engine_without_iops_fails():
    problems = threshold.check(run({"io_uring": 0, "xnvme_io_uring": 1000}), FLOORS)
    assert problems == ["io_uring: no IOPS; its benchmark failed"]


def test_an_unpaired_engine_without_iops_fails():
    def lost(size, depth, rep):
        return 0 if size == 512 else 1000

    assert threshold.check(run({"xnvme_null": lost}), FLOORS) == [
        "xnvme_null: no IOPS; its benchmark failed",
        "xnvme_null: no native peer; name one in its floor",
    ]


def test_an_engine_without_a_native_peer_fails():
    results = run({"cmd-bdev_xnvme": 1000}, groups={"cmd-bdev_xnvme": "cmd"})
    assert threshold.check(results, FLOORS) == [
        "cmd-bdev_xnvme: no native peer; name one in its floor"
    ]


def test_a_floor_names_a_peer_in_another_group():
    groups = {"cmd-bdev_xnvme": "cmd", "uring-bdev_uring": "uring"}
    results = run({"cmd-bdev_xnvme": 1500, "uring-bdev_uring": 1000}, groups=groups)
    floors = {"cmd-bdev_xnvme/uring-bdev_uring": 1.6}
    assert threshold.check(results, floors) == [
        "cmd-bdev_xnvme/uring-bdev_uring: 1.500 < 1.6"
    ]


def test_repetitions_at_a_point_are_averaged():
    def alternating(size, depth, rep):
        return 900 if rep == 0 else 1000

    results = run({"io_uring": 1000, "xnvme_io_uring": alternating}, repetitions=2)
    assert threshold.check(results, FLOORS) == [], "0.95 on average, not 0.9 or 1.0"


def test_a_pair_with_no_common_point_fails():
    def only_512(size, depth, rep):
        return 1000 if size == 512 else None

    def only_4096(size, depth, rep):
        return 1000 if size == 4096 else None

    groups = {"cmd-bdev_xnvme": "cmd", "uring-bdev_uring": "uring"}
    results = [
        r
        for r in run(
            {"cmd-bdev_xnvme": only_512, "uring-bdev_uring": only_4096}, groups=groups
        )
        if r[1]["iops"] is not None
    ]
    floors = {"cmd-bdev_xnvme/uring-bdev_uring": 1.0}
    assert threshold.check(results, floors) == [
        "cmd-bdev_xnvme/uring-bdev_uring: no point where both report IOPS"
    ]
