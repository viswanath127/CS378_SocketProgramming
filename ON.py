import sys
import numpy as np

config_file = sys.argv[1]

N_VN = 0

class O_Node:
    def read_config_file(config_file):
        with open(config_file) as f:
            r1_found = False

            for l in f:
                ll = l.strip().split()
                if ll[0] == '#':
                    continue
                if not r1_found:
                    r1_found = True
                    N_VN = len(ll) + 1




