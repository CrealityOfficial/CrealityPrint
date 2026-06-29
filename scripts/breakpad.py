import subprocess
from pathlib import Path
import shutil

def process_sym_files():
    try:
        # 检查当前目录是否有 PDB 文件
        pdb_file = Path("CrealityPrint_Slicer.pdb")
        if not pdb_file.exists():
            print(f"ERROR: PDB file not found in current directory: {Path.cwd()}")
            print(f"Expected path: {pdb_file.absolute()}")
            print("")
            print("Please run this script from the Release directory containing the PDB file.")
            print("Example:")
            print("  cd build\\src\\Release")
            print("  python ..\\..\\scripts\\breakpad.py")
            return
        
        print(f"Processing PDB: {pdb_file.absolute()}")
        
        # 执行 dump_syms 命令
        result = subprocess.run(
            [r'C:\breakpad\breakpad\dump_syms.exe', str(pdb_file)],
            capture_output=True,
            text=True,
            shell=False
        )
        
        if result.returncode != 0:
            error_msg = result.stderr.decode() if isinstance(result.stderr, bytes) else result.stderr
            print(f"Command failed: {error_msg}")
            return
        
        # 将输出写入 .sym 文件
        sym_file = Path('CrealityPrint_Slicer.sym')
        with open(sym_file, 'w', encoding='utf-8') as f:
            f.write(result.stdout)
        
        print(f"Generated SYM file: {sym_file}")

        # 验证文件生成
        if not sym_file.exists():
            print("ERROR: Symbol file was not created")
            return

        # 读取第一行（自动处理编码）
        with sym_file.open('r', encoding='utf-8') as f:
            first_line = f.readline().strip()
            print(f"First line: {first_line}")
            
        if first_line.startswith('MODULE'):
            parts = first_line.split()
            if len(parts) >= 5:
                info = {
                    "os": parts[1],
                    "arch": parts[2],
                    "module_id": parts[3],
                    "pdb_name": parts[4]
                }
                target_dir = Path("symbols") / Path(info["pdb_name"]) / info["module_id"]
                target_dir.mkdir(parents=True, exist_ok=True)
                shutil.move(str(sym_file), str(target_dir / sym_file.name))
                print("SUCCESS: Symbol generation completed")
            else:
                print("ERROR: Invalid MODULE line format")
        else:
            print("ERROR: First line does not start with MODULE")

    except Exception as e:
        print(f"ERROR: {str(e)}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    process_sym_files()
    print("finish")
