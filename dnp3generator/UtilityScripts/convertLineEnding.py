import os
import string
import argparse

parser = argparse.ArgumentParser(description='Convert line ending to line ending recognized by lua.')
parser.add_argument('-i', '--file', default = "Fault 4_24s_1ms.csv", help='Input csv File')
parser.add_argument('-w', '--out', default = "DNP3Data.csv", help='Output csv File')
args = parser.parse_args()

if not os.path.isfile(args.file):
    print("Invalid path for input File")
    exit()
if args.file == args.out:
    print("Use a different name for the output file. It cannot be the same name as the input file")
    exit()
with open(args.file, 'r') as fileread:
    with open(args.out, 'w') as fileout:
        for line in fileread:
            new_line = string.replace(line, '\r','')
            fileout.write(new_line)
