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
          packages = with pkgs; [
            clang-tools # clangd, clang-format, clang-tidy
            curl # tools/fetch.c
            just
            sqlite # tools/fetch.c: Firefox cookie lookup
          ];
          # the justfile owns the flags: the cc wrapper's fortify would
          # inject _FORTIFY_SOURCE into -O0 and sanitized debug builds
          hardeningDisable = [
            "fortify"
            "fortify3"
          ];
        };
      });

      formatter = forAllSystems (pkgs: pkgs.nixfmt-rfc-style);
    };
}
