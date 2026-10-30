# Builds package.nix from the tree it sits in rather than a release, for CI and
# for toolbox/pkgs/nixos-latest.sh, which realises its build inputs. Only the
# toolbox is there when the deps image is built, and that suffices: the inputs
# do not depend on the tree. Subprojects a meson download left behind are kept
# out, as the package puts uPCIe there itself, and so are the build directory
# and the archives CI downloads into the tree.
{
  # Pinned, so the image the build runs in holds the very inputs the build
  # takes; moving it rebuilds that image, as distro-deps watches toolbox/nix
  pkgs ? import (fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/1c89083e451e891152365b799595cf7790a43325.tar.gz";
    sha256 = "0gnw7gkrh9axnknb67627mf9sbdd7sbgw3148g5lh1d8hiks4l33";
  }) { },
  src ? ../..,
}:

(pkgs.callPackage ./package.nix { }).overrideAttrs {
  version = "git";
  src = pkgs.lib.cleanSourceWith {
    src = pkgs.lib.cleanSource src;
    filter =
      path: type:
      let
        rel = pkgs.lib.removePrefix (toString src + "/") (toString path);
      in
      !(pkgs.lib.hasPrefix "builddir" rel)
      && !(pkgs.lib.hasPrefix "artifacts" rel)
      && !(type == "directory" && builtins.elem rel [
        "subprojects/upcie"
        "subprojects/spdk"
      ]);
  };
}
