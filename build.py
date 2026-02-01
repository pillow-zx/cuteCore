import os
import glob

BIN_DIR = "./user/build/bin"
OUTPUT_S = "./os/src/link_apps.S"

def generate():
    bin = sorted(glob.glob(os.path.join(BIN_DIR, "*.bin")))
    app_names = [os.path.splitext(os.path.basename(b))[0] for b in bin]
    
    with open(OUTPUT_S, "w") as f:
        f.write(".align 3\n")
        f.write(".section .data\n")
        f.write(".global _num_app\n")

        f.write(f"_num_app:\n    .quad {len(app_names)}\n")

        for i in range(len(app_names)):
            f.write(f"    .quad app_{i}_start\n")
        if app_names:
            f.write(f"    .quad app_{len(app_names)-1}_end\n")


        for i, name in enumerate(app_names):
            bin_path = os.path.join(BIN_DIR, f"{name}.bin")
            f.write(f"\n    .section .data\n")
            f.write(f"    .global app_{i}_start\n")
            f.write(f"    .global app_{i}_end\n")
            f.write(f"app_{i}_start:\n")
            f.write(f'    .incbin "{bin_path}"\n')
            f.write(f"app_{i}_end:\n")


if __name__ == "__main__":
    generate()

