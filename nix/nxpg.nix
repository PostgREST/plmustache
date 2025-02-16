{ writeShellScriptBin, findutils, entr, callPackage, lcov, clang-tools, git, postgresql_17, postgresql_16, postgresql_15, postgresql_14, postgresql_13, postgresql_12 } :
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
  cov =
    writeShellScriptBin "${prefix}-coverage" ''
      set -euo pipefail

      info_file="coverage.info"
      out_dir="coverage_html"

      make clean
      make COVERAGE=1
      make installcheck
      ${lcov}/bin/lcov --capture --directory . --output-file "$info_file"

      # remove postgres headers on the nix store, otherwise they show on the output
      ${lcov}/bin/lcov --remove "$info_file" '/nix/*' --output-file "$info_file" || true

      ${lcov}/bin/lcov --list coverage.info
      ${lcov}/bin/genhtml "$info_file" --output-directory "$out_dir"

      echo "${prefix}-coverage: To see the results, visit file://$(pwd)/$out_dir/index.html on your browser"
    '';
  style =
    writeShellScriptBin "${prefix}-style" ''
      ${clang-tools}/bin/clang-format -i src/*
    '';
  styleCheck =
    writeShellScriptBin "${prefix}-style-check" ''
      ${clang-tools}/bin/clang-format -i src/*

      ${git}/bin/git diff-index --exit-code HEAD -- '*.c'
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
  cov
  style
  styleCheck
  watch
  tmpDb
  allPgPaths
]
