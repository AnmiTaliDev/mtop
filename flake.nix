{
  description = "mtop - Minimalistic Modern Top for Linux";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        packages = {
          mtop = pkgs.stdenv.mkDerivation {
            pname = "mtop";
            version = "1.0.0";

            src = ./.;

            nativeBuildInputs = with pkgs; [
              meson
              ninja
              pkg-config
            ];

            meta = with pkgs.lib; {
              description = "Minimalistic Modern Top - A beautiful system monitor for Linux";
              homepage = "https://github.com/AnmiTaliDev/mtop";
              license = licenses.asl20;
              platforms = platforms.linux;
              mainProgram = "mtop";
            };
          };

          default = self.packages.${system}.mtop;
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ self.packages.${system}.mtop ];

          packages = with pkgs; [
            # Development tools
            gdb
            valgrind
            clang-tools
          ];

          shellHook = ''
            echo "mtop development environment"
            echo "Build: meson setup build && meson compile -C build"
            echo "Run: ./build/mtop"
          '';
        };

        apps.default = {
          type = "app";
          program = "${self.packages.${system}.mtop}/bin/mtop";
        };
      }
    );
}
