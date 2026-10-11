{
  description = "Advent of Code in C";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

  outputs =
    { nixpkgs, ... }:
    let
      systems = [
        "aarch64-darwin"
        "x86_64-darwin"
        "aarch64-linux"
        "x86_64-linux"
      ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f nixpkgs.legacyPackages.${system});
    in
    {
      devShells = forAllSystems (pkgs: {
        # clangStdenv: the same compiler (and warning set) on Linux and macOS
        default = pkgs.mkShell.override { stdenv = pkgs.clangStdenv; } {
          packages =
            with pkgs;
            [
              clang-tools # clangd, clang-format, clang-tidy
              cppcheck # third static analyzer in `just lint`
              nixfmt # formats flake.nix in `just fmt`
              curl # tools/fetch.c
              just
              sqlite # tools/fetch.c: Firefox cookie lookup
            ]
            # Linux only: gcc is the second strict compiler for `just gcc`
            # (on Darwin it would just be another clang); perf counts
            # instructions for `just bench` (Darwin has no perf)
            ++ lib.optionals stdenv.hostPlatform.isLinux [
              gcc
              perf
              util-linux # taskset pins `just bench` to one CPU
            ];
          # the justfile owns the flags: the cc wrapper's fortify would
          # inject _FORTIFY_SOURCE into -O0 and sanitized debug builds
          hardeningDisable = [
            "fortify"
            "fortify3"
          ];
        };
      });

      formatter = forAllSystems (pkgs: pkgs.nixfmt);
    };
}
