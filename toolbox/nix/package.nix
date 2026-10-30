# The xNVMe package as nixpkgs would take it, at pkgs/by-name/xn/xnvme. CI
# builds it by default.nix with its own tree in place of 'src'. It names a
# commit on main, as nixpkgs takes unreleased versions, since v0.7.5 has neither
# the uPCIe subproject nor the options below; a release replaces it.
{
  lib,
  stdenv,
  fetchFromGitHub,
  meson,
  ninja,
  pkg-config,
  python3,
  libaio,
  liburing,
  isa-l,
}:

let
  # Header-only; xNVMe takes it as a meson subproject, which the build sandbox
  # cannot fetch
  upcie = fetchFromGitHub {
    owner = "safl";
    repo = "upcie";
    tag = "v0.10.4";
    hash = "sha256-8HCt0QYrSb/zrnx4B/sR3LauT/rTkQjmhWQetbyma20=";
  };
in
stdenv.mkDerivation {
  pname = "xnvme";
  version = "0.7.5-unstable-2026-10-30";

  src = fetchFromGitHub {
    owner = "xnvme";
    repo = "xnvme";
    rev = "1ef692f5c587668ad98c2505575aceaca94bc99b";
    hash = "sha256-OUvwDUA3oTQ/uPaL+KxJPQ/1sE12Lz/qduSpWtCzf3I=";
  };

  postUnpack = ''
    cp -r ${upcie} $sourceRoot/subprojects/upcie
    chmod -R u+w $sourceRoot/subprojects/upcie
  '';

  nativeBuildInputs = [
    meson
    ninja
    pkg-config
    python3
  ];

  buildInputs = [
    libaio
    liburing
    isa-l
  ];

  # The meson hook enables every 'auto' feature; xNVMe's own defaults are wanted
  mesonAutoFeatures = "auto";

  # Without SPDK and libvfn; the uPCIe backend drives NVMe from user space
  mesonFlags = [
    (lib.mesonEnable "with-spdk" false)
    (lib.mesonEnable "with-libvfn" false)
    (lib.mesonEnable "with-cuda" false)
    (lib.mesonEnable "with-hip" false)
    (lib.mesonEnable "with-libaio" true)
    (lib.mesonEnable "with-liburing" true)
    (lib.mesonEnable "with-isal" true)
    (lib.mesonBool "build_subprojects" false)
    (lib.mesonOption "systemd_unitdir" "${placeholder "out"}/lib/systemd/system")
  ];

  meta = {
    description = "Cross-platform libraries and tools for NVMe devices";
    homepage = "https://xnvme.io";
    license = lib.licenses.bsd3;
    platforms = lib.platforms.linux;
    mainProgram = "xnvme";
    maintainers = [ ];
  };
}
