with import (builtins.fetchTarball {
  name = "26.05";
  url = "https://github.com/NixOS/nixpkgs/archive/refs/tags/26.05.tar.gz";
  sha256 = "sha256:0am8xx09fx5yf2p0wb001v0jx1g5hrfb76h4r37xph378jgk7pcr";
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
