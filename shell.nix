with import (builtins.fetchTarball {
  name = "24.05";
  url = "https://github.com/NixOS/nixpkgs/archive/refs/tags/24.05.tar.gz";
  sha256 = "sha256:1lr1h35prqkd1mkmzriwlpvxcb34kmhc9dnr48gkm8hh089hifmx";
}) {};
mkShell {
  buildInputs =
    let
      extensionName = "plmustache";
      supportedPgVersions = [
        postgresql_16
        postgresql_15
        postgresql_14
        postgresql_13
        postgresql_12
      ];
      mustach    = callPackage ./nix/mustach.nix {};
      pgWExtension = { postgresql }: postgresql.withPackages (p: [ (callPackage ./nix/pgExtension.nix { inherit postgresql extensionName mustach; }) ]);
      extAll = map (x: callPackage ./nix/pgScript.nix { postgresql = pgWExtension { postgresql = x;}; }) supportedPgVersions;
    in
    extAll;

  shellHook = ''
    export HISTFILE=.history
  '';
}
