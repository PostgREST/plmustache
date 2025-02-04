with import (builtins.fetchTarball {
  name = "25.05-pre";
  url = "https://github.com/NixOS/nixpkgs/archive/refs/tags/25.05-pre.tar.gz";
  sha256 = "sha256:0b96wqvk3hs98dhfrmdhqmx9ibac4kjpanpd1pig19jaglanqnxr";
}) {};
mkShell
{
  buildInputs =
  [
    (callPackage ./nix/mustach.nix {})
  ] ++
  (callPackage ./nix/nxpg.nix {});

  shellHook = ''
    export HISTFILE=.history
  '';
}
