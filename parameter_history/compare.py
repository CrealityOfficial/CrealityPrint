import sys, os

if len(sys.argv) < 3:
    print("Usage: python compare.py [version1] [version2]")
    sys.exit(1)

version1 = sys.argv[1]
version2 = sys.argv[2]

suffix = ".metas.csv"

csv1 = version1 + suffix
csv2 = version2 + suffix

working_directory = os.getcwd()
print(working_directory)
os.chdir('parameter_history')

import csv
try:
    with open(csv1, 'r') as f1, open(csv2, 'r') as f2:
        lines1 = csv.reader(f1)
        lines2 = csv.reader(f2)

        meta1 = []
        sear1 = {}
        meta2 = []
        sear2 = {}

        for line in lines1:
            meta1.append(line)  
            if len(line[3]) > 0 and line[3] != 'Key':    # 3rd column is the key
                sear1[line[3]] = line 

        for line in lines2:
            meta2.append(line)
            if len(line[3]) > 0 and line[3] != 'Key':   # 3rd column is the key
                sear2[line[3]] = line
        
        out_meta = []
        # add delete key
        for (key, meta) in sear2.items():
            if key not in sear1:
                meta[7] = 'Deleted'
                out_meta.append(meta)

        for meta in meta1:
            key = meta[3]
            if len(key) > 0 and key != 'Key':  # real key
                meta[7] = 'Unchanged'
                if key in sear2:
                    if meta[5] != sear2[key][5]:  # 5th column is the type
                        change = 'Changed ({}->{})'.format(sear2[key][5], meta[5])
                        meta[7] = change
                else:
                    meta[7] = 'Added'
                    
                out_meta.append(meta)
            else:
                out_meta.append(meta)

        out_csv = '{}-{}.compare.csv'.format(version1, version2)
        with open(out_csv, 'w', newline='') as outf:
            writer = csv.writer(outf)
            
            for row in out_meta:
                writer.writerow(row)

except Exception as e:
    print("Error:", e)
    sys.exit(1)