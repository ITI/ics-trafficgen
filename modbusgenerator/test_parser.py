import unittest
from master_station import master_station
from outstation import outstation
from cfg_json_parser import json_parser

'''
Example Test Counting the total nodes allocated by the parser
'''
test_cfg = '''
{
    "Master Nodes":
    [
        {"Name":"M1",
        "Outstations":["S1", "S2"]
    },
    {"Name":"M2",
    "Outstations":["S1"]
    }
    ],
    "S1":{"IP Address":"192.13.13.12", "Data Count":17, "Unit ID":1},
    "S2":{"IP Address":"192.13.13.12", "Data Count":15, "Unit ID":2},
    "S3":{}
}
'''
class ParserTest(unittest.TestCase):
    def runTest(self):
        parser = json_parser(test_cfg, test=True)

        self.assertEqual(len(parser.masters), (2), 'Incorrect Master Nodes')
        self.assertEqual(len(parser.masters)+len(parser.outstations), (3), 'Incorrect Number of Nodes')

if __name__== '__main__':
    unittest.main()
