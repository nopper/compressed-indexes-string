#!/usr/bin/env python

from utils import *


def main(tmpdir):
    graph_file = os.path.join(tmpdir, "simple-graph.tsv.gz")

    name_file = os.path.join(tmpdir, "simple-names.txt.gz")
    id_mapping_file = os.path.join(tmpdir, "id-mapping.gz")
    sorted_graph_file = os.path.join(tmpdir, "sorted-graph.tsv.gz")

    dummy_index_file = os.path.join(tmpdir, "dummy_index")
    expanded_graph_file = os.path.join(tmpdir, "graph-k2.tsv.gz")

    universe = 1000
    generate_random_graph(graph_file, nodes=universe, edges=5)

    generate_names(name_file, universe)
    generate_id_mapping(name_file, id_mapping_file)
    reshape_graph(graph_file, id_mapping_file, sorted_graph_file)

    build_index(sorted_graph_file, dummy_index_file, DUMMY_INDEX, universe)
    expand_graph(sorted_graph_file, expanded_graph_file, dummy_index_file, DUMMY_INDEX)

    for seq in SEQUENCES:
        n1_index = os.path.join(tmpdir, "%s_n1" % seq)
        n2_index = os.path.join(tmpdir, "%s_n2" % seq)

        build_index(sorted_graph_file, n1_index, "%s_simple" % seq, universe, True)
        build_index(expanded_graph_file, n2_index, "%s_simple" % seq, universe, True)

        tbl = []
        tbl.append(["n1"] + get_stats(n1_index, "%s_simple" % seq))
        tbl.append(["n2"] + get_stats(n2_index, "%s_simple" % seq))

        headers = (
            "Scheme", "docs", "elements", "docs-bytes", "posting-bytes",
            "elements/node", "bits/doc", "bits/element", "construction-time"
        )

        tbl.sort(key=lambda x: x[4])

        assert tbl[0][0] == 'n1'
        assert tbl[1][0] == 'n2'

        print
        print "Testing: %s" % seq
        print tabulate.tabulate(tbl, headers)


if __name__ == "__main__":
    tmpdir = tempfile.mkdtemp()
    print "Testing inside %s" % tmpdir

    try:
        main(tmpdir)
    finally:
        print "Deleting %s" % tmpdir
        executor.execute("ls -l %(tmpdir)s" % locals())
        executor.execute("rm -r %(tmpdir)s" % locals())
