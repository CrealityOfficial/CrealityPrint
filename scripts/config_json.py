import json
import sys
import os
def read_json(file_path):
    """读取 JSON 文件并返回数据"""
    try:
        with open(file_path, 'r', encoding='utf-8') as file:
            data = json.load(file)
            return data
    except FileNotFoundError:
        print(f"文件 {file_path} 未找到")
        return None
    except json.JSONDecodeError:
        print(f"文件 {file_path} 不是有效的 JSON 文件")
        return None

def write_json(file_path, data):
    """将数据写入 JSON 文件"""
    try:
        with open(file_path, 'w', encoding='utf-8') as file:
            json.dump(data, file, ensure_ascii=False, indent=4)
            print(f"数据已成功写入 {file_path}")
    except IOError as e:
        print(f"写入文件 {file_path} 时发生错误: {e}")

if __name__ == "__main__":
    # 示例 JSON 文件路径
    arg1 = sys.argv[1] if len(sys.argv) > 1 else None
    arg2 = sys.argv[2] if len(sys.argv) > 2 else None
    print(f"Argument 1: {arg1}")
    print(f"Argument 2: {arg2}")
    base_dir = arg1
    unit_dir = arg2
    if not os.path.exists(base_dir):
        os.makedirs(base_dir)
        print("dir is create")
    config_path = base_dir + "/config.json"
    # 示例数据
    config_data_to_write = {
        "runStatus": True,
        "testTypeName": "SlicerBaseline",
        "version": "CP2.0"
    }

    # 写入 JSON 文件
    write_json(config_path, config_data_to_write)
    slicerBl_path = base_dir + "/SlicerBaseline.json"
    slicerBl_Data = {
        "version": "cx5.2",
        "type": "Compare",
        "input": {
            "baseline dir": unit_dir + "/BaseLine", 
            "3mf dir": unit_dir + "/Source/3MF",
            "script dir": unit_dir + "/Marco", 
            "script running way": "setup once"
        },
        "output": {
            "result dir": unit_dir + "/Result", #"C:/Users/Administrator/Desktop/UnitTest/Result"
        }
    }
    write_json(slicerBl_path, slicerBl_Data)
    # 读取 JSON 文件
    data_read = read_json(slicerBl_path)
    if data_read is not None:
        print("读取的 JSON 数据:")
        print(data_read)

