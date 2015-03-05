#!/usr/bin/env python

from utils import *


def main(tmpdir):
    encoding = SIMPLE_INDICES[0]
    topk_encoding = TOPK_INDICES[0]

    graph_file = "./test_data/simple-graph.tsv.gz"
    name_file = "./test_data/simple-names.tsv.gz"

    executor.execute("cp %(graph_file)s %(tmpdir)s/graph.tsv.gz" % locals())
    executor.execute("mkdir %(tmpdir)s/name" % locals())
    executor.execute("cp %(name_file)s %(tmpdir)s/name/attrs.gz" % locals())

    b = Builder(tmpdir, ("name", ))
    b.prepare()

    b.sample_queries(0, "name")

    b.build_index("name", topk_encoding, ranking=True)
    b.build_index("name", encoding)

    # print b.topk("topk-hopping", "name", "name", encoding)
    # print b.topk("topk-hopping-wand", "name", "name", encoding)
    # print b.topk("topk-hopping-rmq", "name", "name", topk_encoding)
    print b.topk("topk-hopping-rmq-wand", "name", "name", topk_encoding)

    print b.stats("name", encoding)
    print b.stats("name", topk_encoding)

    executor.execute("ls -l %(tmpdir)s/indices" % locals())

if __name__ == "__main__":
    tmpdir = tempfile.mkdtemp()
    print "Testing inside %s" % tmpdir

    try:
        main(tmpdir)
    finally:
        print "Deleting %s" % tmpdir
        executor.execute("rm -r %(tmpdir)s" % locals())
