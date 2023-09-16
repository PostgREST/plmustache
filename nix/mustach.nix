{ lib, stdenv, fetchFromGitLab, pkg-config }:

stdenv.mkDerivation rec {
  pname = "mustach";
  version = "1.2.5";

  buildInputs = [ pkg-config ];

  makeFlags = [ "PREFIX=$(out)" "VERSION=${version}"];

  src = fetchFromGitLab {
    owner = "jobol";
    repo = pname;
    rev    = "bd8a41030099c079aff57fcdfb1f5c0aa08cdbaf";
    sha256 = "sha256-3kdXLp40QZo1+JoQUQNHOxuh/9cHhWvpz/ZFQ2MFXW8=";
  };
}
