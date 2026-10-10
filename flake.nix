{
  description = "GitKop";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    openocd.url = "github:PawelReich/openocd-stmicro-flake";
  };

  outputs =
    {
      self,
      nixpkgs,
      openocd,
    }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};

      armToolchain = pkgs.gcc-arm-embedded-15;

      firmware = pkgs.stdenv.mkDerivation {
        pname = "gitkop";
        version = "0.1.0";

        src = ./gitkop001;

        nativeBuildInputs = [
          armToolchain
          pkgs.cmake
          pkgs.ninja
        ];

        cmakeToolchainFile = "${./gitkop001}/cmake/gcc-arm-none-eabi.cmake";
        dontUseCmakeBuildDir = true;
        cmakeFlags = [ "-DCMAKE_BUILD_TYPE=Release" ];

        installPhase = ''
          runHook preInstall

          install -Dm444 gitkop001.elf $out/gitkop001.elf

          runHook postInstall
        '';

        meta = {
          description = "GitKop firmware";
          platforms = [ "x86_64-linux" ];
        };
      };

      plotterPython = pkgs.python3.withPackages (
        ps: with ps; [
          tkinter
          matplotlib
          numpy
          pyserial
        ]
      );
    in
    {
      packages.${system} = {
        inherit firmware;
        default = firmware;
      };

      devShells.${system}.default = pkgs.mkShell {
        packages = [
          armToolchain
          plotterPython
          pkgs.cmake
          pkgs.gdb
          pkgs.ninja
          pkgs.stlink-tool
          pkgs.usbutils

          # STMicroelectronics' OpenOCD 
          openocd.packages.${system}.openocd-st
        ];

        shellHook = ''
          export ARM_SWD_CONFIG="${
            pkgs.runCommand "gitkop-openocd-config" { } ''
              mkdir -p $out
              cat > $out/gitkop.cfg <<'EOF'
              source [find interface/stlink-dap.cfg]
              source [find target/stm32h5x.cfg]
              adapter speed 4000
              reset_config srst_only
              EOF
            ''
          }/gitkop.cfg"
          echo "GitKop dev shell"
          echo "  cd gitkop001 && cmake --preset Release && cmake --build --preset Release"
          echo "  flash with: openocd -f \$ARM_SWD_CONFIG -c 'program build/Release/gitkop001.elf verify reset exit'"
          echo "  plotter with: python plotter/main.py"
        '';
      };
    };
}
