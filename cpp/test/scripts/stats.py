#!/usr/bin/env python

from utils import *


def main(tmpdir):
    elements = []

    for encoding in SIMPLE_INDICES:
        output = os.path.join(tmpdir, encoding)
        build_index("./test_data/coverage.tsv.gz", output, encoding, 23)
        elements.append([encoding] + get_stats(output, encoding))

    headers = ("Encoding", "docs", "elements", "docs-bytes", "posting-bytes",
               "elements/node", "bits/doc", "bits/element", "construction-time")

    rows = []

    for element in sorted(elements, key=lambda x: (x[6], x[0])):
        rows.append(element)

    print tabulate.tabulate(rows, headers)

    assert rows[0][0] == "ef_simple", "Most succinct index is not ef_simple"


if __name__ == "__main__":
    tmpdir = tempfile.mkdtemp()
    print "Testing inside %s" % tmpdir

    try:
        main(tmpdir)
    finally:
        print "Deleting %s" % tmpdir
        executor.execute("rm -r %(tmpdir)s" % locals())
