{ postgresql, writeShellScriptBin, options ? "" } :

let
  ver = builtins.head (builtins.splitVersion postgresql.version);
  script = ''
    export PATH=${postgresql}/bin:"$PATH"

    tmpdir="$(mktemp -d)"

    export PGDATA="$tmpdir"
    export PGHOST="$tmpdir"
    export PGUSER=postgres
    export PGDATABASE=postgres

    trap 'pg_ctl stop -m i && rm -rf "$tmpdir"' sigint sigterm exit

    PGTZ=UTC initdb --no-locale --encoding=UTF8 --nosync -U "$PGUSER"

    default_options="-F -c listen_addresses=\"\" -k $PGDATA"

    pg_ctl start -o "$default_options" -o "${options}"

    "$@"
  '';
in
writeShellScriptBin "with-pg-${ver}" script
