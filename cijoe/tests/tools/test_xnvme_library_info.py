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
