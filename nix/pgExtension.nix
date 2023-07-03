{ stdenv, postgresql, mustache-c, extensionName }:

stdenv.mkDerivation {
  name = extensionName;

  buildInputs = [ postgresql mustache-c ];

  src = ../.;

  installPhase = ''
    mkdir -p $out/bin
    install -D ${extensionName}.so -t $out/lib

    install -D -t $out/share/postgresql/extension sql/*.sql
    install -D -t $out/share/postgresql/extension ${extensionName}.control
  '';
}
