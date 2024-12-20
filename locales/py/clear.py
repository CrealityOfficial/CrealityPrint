import os
import shutil

excel_path = "../excel/"
newTs_path = "../ts_new/"

# 确认文件夹存在
if os.path.exists(excel_path):
    # 清空文件夹
    shutil.rmtree(excel_path)
    # 重新创建空文件夹
    os.mkdir(excel_path)
else:
    print("文件夹不存在")

print(f"清空文件夹{excel_path}")

# 确认文件夹存在
if os.path.exists(newTs_path):
    # 清空文件夹
    shutil.rmtree(newTs_path)
    # 重新创建空文件夹
    os.mkdir(newTs_path)
else:
    print("文件夹不存在")

print(f"清空文件夹{newTs_path}")