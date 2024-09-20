{
  description = "template";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs = { self, nixpkgs }: 
  let
    system = "x86_64-linux";
    pkgs = import nixpkgs {
        inherit system;
    };
    deps = with pkgs; [ 
      clang
      lldb
      cmake
    ];
  in 
  {
    devShells."${system}" =  {
      default = pkgs.mkShell.override
      {
      }
      {
        shellHook= ''fish'';
        packages = deps;
      };
    };
  };  
}
