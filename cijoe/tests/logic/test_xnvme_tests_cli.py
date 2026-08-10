from ..conftest import xnvme_parametrize


def test_optional_none(cijoe):
    err, _ = cijoe.run("xnvme_tests_cli optional")
    assert not err


@xnvme_parametrize(labels=[], opts=["be", "mem", "sync", "async", "admin"])
def test_optional_all(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(
        f"xnvme_tests_cli optional "
        f"--be {be_opts['be']} "
        f"--mem {be_opts['mem']} "
        f"--sync {be_opts['sync']} "
        f"--async {be_opts['async']} "
        f"--admin {be_opts['admin']} "
    )
    assert not err


def test_cpuids_none(cijoe):
    err, state = cijoe.run("xnvme_tests_cli cpu-ids")
    assert not err

    assert "cpumask: 'NULL', given: 0" in state.output()
    assert "cpulist: 'NULL', given: 0" in state.output()
    assert "ncpus: 0" in state.output()


def test_cpuids_cpumask_0x(cijoe):
    err, state = cijoe.run("xnvme_tests_cli cpu-ids --cpumask 0x3")
    assert not err

    assert "cpulist: 'NULL', given: 0" in state.output()
    assert "ncpus: 2" in state.output()
    for i, cpu in enumerate([0, 1]):
        assert f"cpus[{i}]: {cpu}" in state.output()


def test_cpuids_cpumask(cijoe):
    err, state = cijoe.run("xnvme_tests_cli cpu-ids --cpumask 3")
    assert not err

    assert "cpulist: 'NULL', given: 0" in state.output()
    assert "ncpus: 2" in state.output()
    for i, cpu in enumerate([0, 1]):
        assert f"cpus[{i}]: {cpu}" in state.output()


def test_cpuids_cpulist(cijoe):
    err, state = cijoe.run("xnvme_tests_cli cpu-ids --cpulist 2-4,6")
    assert not err

    assert "cpumask: 'NULL', given: 0" in state.output()
    assert "ncpus: 4" in state.output()
    for i, cpu in enumerate([2, 3, 4, 6]):
        assert f"cpus[{i}]: {cpu}" in state.output()


def test_cpuids_invalid(cijoe):
    cases = [
        "--cpumask 0x1 --cpulist 1",  # mutually exclusive
        "--cpumask 0x0",  # zero mask
        "--cpumask zz",  # not hex
        "--cpulist ''",  # empty list
        "--cpulist 2-1",  # reversed range
        "--cpulist 1,",  # trailing comma
        "--cpulist 1024",  # id out of range
        "--cpulist 0,0",  # repeated id
        "--cpulist 0-4,1",  # repeated id inside range
        "--cpulist 0-4,3-6",  # repeated id inside ranges
        "--cpulist 0-1023,0-1023",  # repeated id, would overflow cpus[]
    ]

    # Collect instead of asserting per case, to see every offender in one run
    unexpected = []
    for args in cases:
        err, _ = cijoe.run(f"xnvme_tests_cli cpu-ids {args}")
        if not err:
            unexpected.append(args)

    assert not unexpected, f"We expected these to fail: {unexpected}"
