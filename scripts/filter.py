import sys
import gzip
import codecs
from nameparser import HumanName
from optparse import OptionParser


def main(graph_file):
    valid = set()
    with gzip.open(graph_file) as inputfile:
        for line in inputfile:
            valid.add(int(line.split('\t')[0]))

    for line in sys.stdin:
        node_id = int(line.split('\t')[0])
        if node_id in valid:
            sys.stdout.write(line)


if __name__ == "__main__":
    op = OptionParser()
    op.add_option("--graph", dest="graph_file", metavar="FILE",
                  help="Graph file")

    (opts, args) = op.parse_args()
    main(opts.graph_file)

