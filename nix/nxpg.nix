{ writeShellScriptBin, findutils, entr, callPackage, postgresql_17, postgresql_16, postgresql_15, postgresql_14, postgresql_13, postgresql_12 } :
let
  prefix = "nxpg";
  supportedPgs = [
    postgresql_17
    postgresql_16
    postgresql_15
    postgresql_14
    postgresql_13
    postgresql_12
  ];
  build =
    writeShellScriptBin "${prefix}-build" ''
      set -euo pipefail

      make clean
      make
    '';
  test =
    writeShellScriptBin "${prefix}-test" ''
      set -euo pipefail

      make clean
      make
      make installcheck
    '';

  watch =
    writeShellScriptBin "${prefix}-watch" ''
      set -euo pipefail

      ${findutils}/bin/find . -type f \( -name '*.c' -o -name '*.h' \) | ${entr}/bin/entr -dr "$@"
    '';

  tmpDb =
    writeShellScriptBin "${prefix}-tmp" ''
      set -euo pipefail

      export tmpdir="$(mktemp -d)"

      export PGDATA="$tmpdir"
      export PGHOST="$tmpdir"
      export PGUSER=postgres
      export PGDATABASE=postgres

      trap 'pg_ctl stop -m i && rm -rf "$tmpdir"' sigint sigterm exit

      PGTZ=UTC initdb --no-locale --encoding=UTF8 --nosync -U "$PGUSER"

      # pg versions older than 16 don't support adding "-c" to initdb to add these options
      # so we just modify the resulting postgresql.conf to avoid an error
      echo "dynamic_library_path='\$libdir:$(pwd)'" >> $PGDATA/postgresql.conf
      echo "extension_control_path='\$system:$(pwd)'" >> $PGDATA/postgresql.conf

      default_options="-F -c listen_addresses=\"\" -k $PGDATA"

      pg_ctl start -o "$default_options"

      "$@"
    '';
    allPgPaths = map (pg:
      let
        ver = builtins.head (builtins.splitVersion pg.version);
        patchedPg = pg.overrideAttrs(oldAttrs: {
          patches = oldAttrs.patches ++ [
            ./patches/${ver}-add-extension_control_path-for.patch
          ];
        });
        script = ''
          set -euo pipefail

          export PATH=${patchedPg.dev}/bin:${patchedPg}/bin:"$PATH"

          "$@"
        '';
      in
      writeShellScriptBin "${prefix}-${ver}" script
    ) supportedPgs;
in
[
  build
  test
  watch
  tmpDb
  allPgPaths
]
