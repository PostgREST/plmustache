with import (builtins.fetchTarball {
  name = "2023-09-16";
  url = "https://github.com/NixOS/nixpkgs/archive/ae5b96f3ab6aabb60809ab78c2c99f8dd51ee678.tar.gz";
  sha256 = "11fpdcj5xrmmngq0z8gsc3axambqzvyqkfk23jn3qkx9a5x56xxk";
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
