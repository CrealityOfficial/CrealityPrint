import subprocess

print(f"\n--------开始执行python--------")
subprocess.run(["python", "clear.py"])
subprocess.run(["python", "clear_orca.py"])
subprocess.run(["python", "splitExcel.py"])
subprocess.run(["python", "trans.py"])
subprocess.run(["python", "trans_orca.py"])
print(f"\n--------结束执行python--------")

input("\nPress Enter to exit...")

