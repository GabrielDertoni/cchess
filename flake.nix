{
  description = "Minimalistic Chess Machmaking written in C";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        name = "cchess";
        version = "0.1";
        pkgs = import nixpkgs { inherit system; };

        binaries = [ "engine_chess" "ui" ];

        nativeBuildInputs = [
          pkgs.xxd
        ];
       in rec {
        packages.engine_chess = pkgs.stdenv.mkDerivation {
          inherit version system nativeBuildInputs;
          pname = "engine_chess";
          src = self;
          buildPhase = "make build/engine_chess";
          installPhase = "mkdir -p $out/bin; install -t $out/bin build/engine_chess";
        };

        packages.ui = pkgs.stdenv.mkDerivation {
          inherit version system nativeBuildInputs;
          pname = "ui";
          src = self;
          buildPhase = "make build/ui";
          installPhase = "mkdir -p $out/bin; install -t $out/bin build/ui";
        };

        packages.container = pkgs.dockerTools.buildImage {
          inherit name;
          tag = version;
          created = "now";
          copyToRoot = map (pname: packages.${pname}) binaries ++ [ pkgs.coreutils pkgs.bash ];
          config.Cmd = [ "${packages.engine_chess}/bin/engine_chess" ];
        };

        apps.engine_chess = flake-utils.lib.mkApp {
          name = "engine_chess";
          drv = packages.engine_chess;
        };

        apps.ui = flake-utils.lib.mkApp {
          name = "ui";
          drv = packages.ui;
        };

        devShell = pkgs.mkShell {
          inherit nativeBuildInputs;
        };
      }
    );
}
