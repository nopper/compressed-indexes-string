#!/usr/bin/env python

from utils import *


def main(tmpdir):
    for encoding in SIMPLE_INDICES:
        graph_file = "./test_data/simple-graph.tsv.gz"
        name_file = "./test_data/simple-names.tsv.gz"

        executor.execute("cp %(graph_file)s %(tmpdir)s/graph.tsv.gz" % locals())
        executor.execute("mkdir -p %(tmpdir)s/name" % locals())
        executor.execute("cp %(name_file)s %(tmpdir)s/name/attrs.gz" % locals())

        b = Builder(tmpdir, ("name", ))
        b.prepare()

        b.sample_queries(0, "name")

        b.build_index("name", encoding)
        b.build_index("_id", encoding)

        print b.intersection("hopping", "name", "name", encoding)
        print b.intersection("baseline-hopping", "name", "_id", encoding)
        print b.intersection("fast-baseline-hopping", "name", "_id", encoding)

if __name__ == "__main__":
    tmpdir = tempfile.mkdtemp()
    print "Testing inside %s" % tmpdir

    try:
        main(tmpdir)
    finally:
        print "Deleting %s" % tmpdir
        executor.execute("rm -r %(tmpdir)s" % locals())
