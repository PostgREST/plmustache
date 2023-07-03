{ lib, stdenv, flex, bison, autoreconfHook, pkg-config, fetchFromGitHub }:

stdenv.mkDerivation rec {
  pname = "mustache-c";
  version = "1.0";

  nativeBuildInputs = [ autoreconfHook pkg-config ];
  buildInputs = [ flex bison ];

  src = fetchFromGitHub {
    owner = "x86-64";
    repo = "mustache-c";
    rev = "7fe52392879d0188c172d94bb4fde7c513d6b929";
    sha256 = "sha256-uDDjG7NOwoHWFhy1XiwsfiHbnPYIrPA/JuRpTBfUch4=";
  };
}
