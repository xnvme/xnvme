import pytest


def grab_sysinfo(cijoe):
    info = {}

    _, state = cijoe.run("uname")
    info["os"] = state.output().lower()

    _, state = cijoe.run("cat /etc/os-release")
    info["rel"] = state.output().lower()

    return info


def test_library_info_has_spdk(cijoe):
    info = grab_sysinfo(cijoe)

    is_macos = "darwin" in info["os"]
    is_windows = "msys_nt" in info["os"]

    if not (is_macos or is_windows):
        if "alpine" in info["rel"]:
            pytest.skip("SPDK does not support Alpine Linux; skipping")
            return

        err, state = cijoe.run("xnvme library-info")
        assert not err

        assert "XNVME_BE_SPDK_ENABLED" in state.output()
    else:
        pytest.skip("Not Linux or FreeBSD, so skipping SPDK check")


def test_library_info_has_liburing(cijoe):
    info = grab_sysinfo(cijoe)

    if "linux" in info["os"]:
        err, state = cijoe.run("xnvme library-info")
        assert not err

        assert "XNVME_BE_LINUX_LIBURING_ENABLED" in state.output()
    else:
        pytest.skip("Not Linux, so skipping liburing check")


def test_library_info_has_libaio(cijoe):
    info = grab_sysinfo(cijoe)

    if "linux" in info["os"]:
        err, state = cijoe.run("xnvme library-info")
        assert not err

        assert "XNVME_BE_LINUX_LIBAIO_ENABLED" in state.output()
    else:
        pytest.skip("Not Linux, so skipping libaio check")


def test_library_info_has_libvfn(cijoe):
    info = grab_sysinfo(cijoe)

    if "linux" in info["os"]:
        err, state = cijoe.run("xnvme library-info")
        assert not err

        assert "XNVME_BE_VFIO_ENABLED" in state.output()
    else:
        pytest.skip("Not Linux, so skipping libvfn/vfio check")


def test_library_info_has_all_combos(cijoe):
    """Verify every combo in get_backend_configurations() exists in library-info"""
    from ..xnvme_be_combinations import get_backend_configurations

    info = grab_sysinfo(cijoe)

    if "linux" in info["os"]:
        os_key = "linux"
    elif "freebsd" in info["os"]:
        os_key = "freebsd"
    elif "darwin" in info["os"]:
        os_key = "macos"
    elif "msys_nt" in info["os"]:
        os_key = "windows"
    else:
        pytest.skip(f"Unknown OS: {info['os']}")
        return

    err, state = cijoe.run("xnvme library-info")
    assert not err

    output = state.output()

    # Optional backends that depend on compile-time flags
    optional = {
        "spdk": "XNVME_BE_SPDK_ENABLED",
        "libvfn": "XNVME_BE_VFIO_ENABLED",
        "io_uring": "XNVME_BE_LINUX_LIBURING_ENABLED",
        "io_uring_cmd": "XNVME_BE_LINUX_LIBURING_ENABLED",
        "io_uring_bdev": "XNVME_BE_LINUX_LIBURING_ENABLED",
        "io_uring_file": "XNVME_BE_LINUX_LIBURING_ENABLED",
        "libaio": "XNVME_BE_LINUX_LIBAIO_ENABLED",
        "libaio_bdev": "XNVME_BE_LINUX_LIBAIO_ENABLED",
        "libaio_file": "XNVME_BE_LINUX_LIBAIO_ENABLED",
        "upcie": "XNVME_BE_UPCIE_ENABLED",
        "upcie-cuda": "XNVME_BE_UPCIE_CUDA_ENABLED",
        "dmamem": "XNVME_BE_DMAMEM_ENABLED",
    }

    combos = get_backend_configurations()
    missing = []
    for combo in combos.get(os_key, []):
        be_name = combo["be"][0]
        # Skip optional backends not compiled in
        if be_name in optional and optional[be_name] not in output:
            continue
        if f"name: '{be_name}'" not in output:
            missing.append(be_name)

    assert not missing, f"Backend configs missing from library-info: {missing}"
