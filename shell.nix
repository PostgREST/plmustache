with import (builtins.fetchTarball {
  name = "25.05-pre";
  url = "https://github.com/NixOS/nixpkgs/archive/refs/tags/25.05-pre.tar.gz";
  sha256 = "sha256:0b96wqvk3hs98dhfrmdhqmx9ibac4kjpanpd1pig19jaglanqnxr";
}) {};
let
  style =
    writeShellScriptBin "plmustache-style" ''
      ${clang-tools}/bin/clang-format -i src/*
    '';
  styleCheck =
    writeShellScriptBin "plmustache-style-check" ''
      ${clang-tools}/bin/clang-format -i src/*
      ${git}/bin/git diff-index --exit-code HEAD -- '*.c'
    '';
in
mkShell
{
  buildInputs = [
    (callPackage ./nix/mustach.nix {})
    (callPackage ./nix/xpg.nix {})
    styleCheck
    style
  ];

  shellHook = ''
    export HISTFILE=.history
  '';
}
