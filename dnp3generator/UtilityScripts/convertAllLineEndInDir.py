import os
import string
import argparse

for filename in os.listdir('.'):
    if "TD" not in filename:
        continue
    fileread = open(filename, 'r')
    print(filename)
    out_name = os.path.splitext(filename)[0]+'convert.csv'
    print (out_name)
    with open(out_name, 'w') as fileout:
        for line in fileread:
            new_line = string.replace(line, '\r','')
            fileout.write(new_line)
