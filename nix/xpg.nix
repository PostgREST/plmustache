{ fetchFromGitHub, lib } :
let
  dep = fetchFromGitHub {
    owner  = "supabase";
    repo   = "xpg";
    rev    = "v2.6.0";
    sha256 = "sha256-SMv4EKUaGmfUyXyeGHW3AiX1vbef7El56RKTIxio8jQ=";
  };
  xpg = (import dep).xpg;
in
xpg
