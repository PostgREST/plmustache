{ fetchFromGitHub, lib } :
let
  dep = fetchFromGitHub {
    owner  = "supabase";
    repo   = "xpg";
    rev    = "v1.2.1";
    sha256 = "sha256-wmS2FdfA0FvjeuIZ2fWs3sByFIDYCJTBi/gmswVaY+A=";
  };
  xpg = (import dep).xpg;
in
xpg
