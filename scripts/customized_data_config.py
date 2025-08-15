import json
import os
from pathlib import Path
#定制将json数据写入项目目录的 /resource/data/customized_config.json
def write_config(data):
    # 获取项目根目录（假设脚本在./scripts/目录下）
    project_root = Path(__file__).parent.parent  # 上溯两级到项目根目录
    target_path = project_root / "resources" / "data" / "customized_config.json"
    
    # 创建目录（若不存在）
    target_path.parent.mkdir(parents=True, exist_ok=True)
    
    # 写入JSON文件（带格式化）
    try:
        with open(target_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=4, ensure_ascii=False)
        print(f"配置文件已写入: {target_path}")
    except Exception as e:
        print(f"写入失败: {str(e)}")

# 示例调用
if __name__ == "__main__":
    config_data = {"language": "es_ES"}  # 替换为实际需要写入的内容
    write_config(config_data)