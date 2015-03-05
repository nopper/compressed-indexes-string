import sys
import gzip
import random
from optparse import OptionParser


def get_all_vertices():
    vertices = {}
    for line in sys.stdin:
        src, dst = map(int, line.strip().split('\t'))
        vertices[src] = ''
        vertices[dst] = ''
    return vertices


def main(attribute_file, seed):
    vertices = get_all_vertices()
    attributes = []

    print >> sys.stderr, "The graph has %d" % len(vertices)

    with gzip.open(attribute_file) as inputfile:
        for line in inputfile:
            src, attribute = line.rstrip('\n').split('\t')
            src = int(src)
            if src in vertices:
                vertices[src] = attribute
                attributes.append(attribute)

    missing = 0
    random.seed(seed)
    random.shuffle(attributes)

    for src, attribute in sorted(vertices.items()):
        if not attribute:
            attribute = attributes[missing % len(attributes)]
            missing += 1

        print "%d\t%s" % (src, attribute)

    print >> sys.stderr, "Number of missing attributes %d" % missing


if __name__ == "__main__":
    op = OptionParser("Read vertices from stdin and create attributes file")
    op.add_option("--attribute-file", dest="attribute_file", metavar="FILE",
                  help="Attribute file in TSV format")
    op.add_option("--seed", dest="seed", default=42, type="int",
                  help="Seed for random generation")

    (opts, args) = op.parse_args()
    main(opts.attribute_file, opts.seed)
