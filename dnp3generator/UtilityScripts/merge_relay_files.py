import pandas as pd
import argparse
'''
    This script combines the Steady State csv files for the 4 relays into one steady state file that can be used at the Orion.
    It expects an input argument that indicates the base index of the steady state file. Typical values would be 1,2,....6 or 11,12,...16
    It writes the SSReplayX.csv file as output. The line endings are not affected by this script.
'''
parser = argparse.ArgumentParser(description='Combine csv files for 4 relays into one for orion')
parser.add_argument('-i', '--ss_index', default = 1, help='Index of Steady State file')
args = parser.parse_args()

relay_files = ["R90SS", "R91SS", "R92SS", "R93SS"]

all_data = pd.DataFrame()
for f in relay_files:
    csv_file = f+args.ss_index+".csv"
    print(csv_file)
    df = pd.read_csv(csv_file)
    if not all_data.empty:
        del df['Time']
    all_data = pd.concat([all_data, df],axis=1)

all_data.to_csv('SSReplay'+args.ss_index+'.csv', index=False)
