import os
import string
import argparse
import re

parser = argparse.ArgumentParser(description='Convert mapping file to json for config file.')
parser.add_argument('-i', '--file', default = "DNP point list_0808.txt", help='Input mapping File')
parser.add_argument('-w', '--out', default = "dataField.json", help='Output json snippet File')
args = parser.parse_args()

pattern = re.compile("(AI|BI|CI)\s*:\s*(\d+)\s*(\w+)\s*([0-9]*[.]?[0-9]+)")
typedict = {'AI':'Analog Input', 'BI':'Binary Input', 'CI':'Counter'}
evtVarDict = {'AI': '"Event Class":2, "sVariation":5, "eVariation":7',
                'BI': '"Event Class":1, "sVariation":1, "eVariation":1'}
if not os.path.isfile(args.file):
    print("Invalid path for input File")
    exit()

jsonStr = '"Data":\n[\n'
with open(args.file, 'r') as fileread:
    for line in fileread:
        line = line.strip()
        if line.startswith("#"):
            continue
        match = re.match(pattern, line)
        if match:
            ftype = match.group(1)
            index = match.group(2)
            fname = match.group(3)
            deadband = match.group(4)
            fline = '\t{"Type":"' + typedict[ftype] + '", ' + evtVarDict[ftype] + ', "Index":' + index
            if ftype == 'AI':
                fline += ', "Deadband":' + deadband
            fline += '},'
            jsonStr += fline + "\n"
    jsonStr = jsonStr[:-2] #last line, we want to remove the trailing ","
    jsonStr += '\n]'
    #print (jsonStr)
with open(args.out, 'w') as fileout:
    fileout.write(jsonStr)
