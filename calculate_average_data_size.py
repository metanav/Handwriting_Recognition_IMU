import os
from  collections import defaultdict

data_size = defaultdict(list)

for file in os.listdir('./data'):
    if file.endswith('.csv'):
        path = os.path.join('./data', file)
        with  open(path) as f:
            lines = f.readlines()
            digit = os.path.splitext(file)[0].split('_')[-1]
            data_size[digit].append(len(lines))

for digit in data_size:
    size = data_size[digit]
    print(sum(size)/len(size))
