from argparse import ArgumentParser
import subprocess, sys

ap = ArgumentParser(description="Wrapper setup script for the project. All arguments not listed here will be passed to Conan, for more details see Conan's documentation.")
#ap.add_argument('-o', '--options', dest='options', default=[], nargs='+', help='Options passed to Conan (See `conanfile` for available options)')
#ap.add_argument('-s', '--settings', dest='settings', default=[], nargs='+', help='Settings passed to Conan (See `conanfile` for available settings)')
ap.add_argument('-p', '--profile', dest='profile', default=None, help='Conan profile to use for the build - By default it looks for the profile in the `./conan/profiles` directory, but you can also specify a path to a profile file. Defaults to `wine.txt` on Linux, `macos-arm64.txt` on macOS and `windows-msvc.txt` elsewhere.')
ap.add_argument('-b', '--build', dest='build', action='store_true', help='Build the project after setup')

def main() -> None:
    args, conan_args = ap.parse_known_args()

    profile = args.profile
    if profile is None:
        if sys.platform == "linux":
            profile = "wine.txt"
        elif sys.platform == "darwin":
            profile = "macos-arm64.txt"
        else:
            profile = "windows-msvc.txt"
    if '/' not in profile and '\\' not in profile:
        profile = f'./conan/profiles/{profile}'

    subprocess.run(["conan", "install", ".", "--build=missing", "--profile", profile, *conan_args], check=True)
    if args.build:
        subprocess.run(["conan", "build", ".", "--profile", profile, *conan_args], check=True)

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"Error: {e}")
        exit(1)
