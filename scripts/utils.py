import bz2
import gzip
import bisect
import random
from contextlib import contextmanager


@contextmanager
def streamed(filename):
    try:
        if hasattr(filename, 'read'):
            handle = filename
        elif filename.endswith('.bz2'):
            handle = bz2.BZ2File(filename, 'r')
        elif filename.endswith('.gz'):
            handle = gzip.open(filename, 'r')
        else:
            handle = open(filename, 'r')

        yield handle
    finally:
        handle.close()


def attrs_cdf_from_stream(stream, cb=None, cdf=True):
    attributes = []
    counts = []
    probabilities = []
    total = 0

    prev_count = 0

    for line in stream:
        if cb:
            attr, count = cb(line.strip())
        else:
            attr, count = line.strip().split('\t')
            count = int(count)

        attributes.append(attr)
        counts.append(count)
        probabilities.append(count)

        assert prev_count <= count, "Input is not sorted"
        prev_count = count

        total += count

    total = float(total)
    for i in xrange(len(probabilities)):
        probabilities[i] /= total

    if cdf:
        # Convert to CDF
        for i in xrange(1, len(probabilities)):
            probabilities[i] += probabilities[i - 1]

    return attributes, counts, probabilities


def random_sample_index(cdf):
    x = random.random()
    idx = bisect.bisect(cdf, x)
    return idx


def random_sample(attrs, cdf):
    return attrs[random_sample_index(cdf)]
