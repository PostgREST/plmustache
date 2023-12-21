{ lib, stdenv, pkg-config }:

stdenv.mkDerivation rec {
  pname = "mustach";
  version = "1.2.5";

  buildInputs = [ pkg-config ];

  makeFlags = [ "PREFIX=$(out)" "VERSION=${version}"];

  src = ../mustach;
}
