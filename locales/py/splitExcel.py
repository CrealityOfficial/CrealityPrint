import pandas as pd
import os

def split_excel(input_file, output_prefix):
    # 读取Excel文件
    df = pd.read_excel(input_file)
    
    # 获取第一列和其他列的列名
    first_column = df.columns[0]
    second_column = df.columns[1]
    other_columns = df.columns[1:]

    if not os.path.exists(output_prefix):
        os.makedirs(output_prefix)
    
    for col in other_columns:
        # 创建新DataFrame，组合第一列和当前循环列
        new_df = df[[first_column, second_column, col]]
        
        # 生成新的输出文件名
        parts = col.split('-')
        newCol = parts[1] if len(parts) > 1 else col
        output_file = f"{output_prefix}{newCol}.xlsx"
        
        # 保存结果到新的Excel文件
        new_df.to_excel(output_file, index=False)
        print(f"生成{output_file}")

# 示例用法：
print(f"\n--------开始切割excel_orca--------")
input_file = 'a_total.xlsx'
input_file_orca = 'a_total_orca.xlsx'

output_prefix = '../excel/'
output_prefix_orca = '../excel/orca/'
split_excel(input_file, output_prefix)
split_excel(input_file_orca, output_prefix_orca)
print(f"--------结束切割excel_orca--------")
