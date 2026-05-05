import subprocess
import sys

def build_and_run(source_file):
    print(f"Compiling {source_file}...")
    compile_proc = subprocess.run(["g++", "-std=c++20", "-Wall", "-Werror", "-Wno-narrowing", source_file], capture_output=True, text=True)
    if compile_proc.returncode != 0:
        print("Compilation failed!")
        print(compile_proc.stderr)
        return False
    
    print("Compilation successful. Running...")
    run_proc = subprocess.run(["./a.exe"], capture_output=True, text=True)
    if run_proc.returncode != 0:
        print("Run failed! Exit code:", run_proc.returncode)
        print("Stderr:", run_proc.stderr)
        print("Stdout:", run_proc.stdout)
        return False
        
    print("Run successful!")
    print(run_proc.stdout)
    return True

if __name__ == "__main__":
    build_and_run(sys.argv[1])
