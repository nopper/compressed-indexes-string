import sys
import random
from optparse import OptionParser
from utils import *


class SampleUsers(object):
    def __init__(self, opts):
        self.opts = opts
        random.seed(opts.seed)

    def run(self):
        if self.opts.sample == 'uniform':
            self.sample_uniform()
        elif self.opts.sample == 'bucket':
            self.sample_bucket()
        elif self.opts.sample == 'stats':
            self.bucket_stats()
        else:
            raise Exception("Unknown sampling method")

    def bucket_stats(self):
        assert self.opts.buckets > 0

        with streamed(self.opts.input) as stream:
            users, counts, cdf = attrs_cdf_from_stream(stream, cdf=False)

            prev_integral = 0.0
            curr_integral = 0.0
            target_integral = 1.0 / self.opts.buckets

            i = 0

            for bucket_id in xrange(self.opts.buckets):
                bucket = []

                while i < len(cdf) and curr_integral - prev_integral < target_integral:
                    bucket.append((users[i], counts[i]))
                    curr_integral += cdf[i]
                    i += 1

                print bucket_id, bucket[0][1], bucket[-1][1]

                prev_integral = curr_integral

    def sample_bucket(self):
        assert self.opts.buckets > 0

        with streamed(self.opts.input) as stream:
            users, counts, cdf = attrs_cdf_from_stream(stream, cdf=False)

            prev_integral = 0.0
            curr_integral = 0.0
            target_integral = 1.0 / self.opts.buckets

            i = 0

            for bucket_id in xrange(self.opts.buckets):
                bucket = []

                while i < len(cdf) and curr_integral - prev_integral < target_integral:
                    bucket.append((users[i], counts[i]))
                    curr_integral += cdf[i]
                    i += 1

                k = min(self.opts.k, len(bucket))

                for user_count in random.sample(bucket, k):
                    print "%d\t%s\t%d" % \
                        (bucket_id, user_count[0], user_count[1])

                prev_integral = curr_integral

    def sample_uniform(self):
        with streamed(self.opts.input) as stream:
            users, counts, cdf = attrs_cdf_from_stream(stream)

            k = min(self.opts.k, len(users))
            selected = set()

            for x in xrange(k):
                while True:
                    idx = random_sample_index(cdf)

                    if idx not in selected:
                        print "0\t%s\t%d" % (users[idx], counts[idx])
                        selected.add(idx)
                        break

if __name__ == "__main__":
    op = OptionParser()
    op.add_option("--input", dest="input", default=sys.stdin, metavar="FILE",
                  help="TSV file containing the attributes and their counts")
    op.add_option("--seed", dest="seed", default=42, type="int",
                  help="Random seed to initialize the RNG")
    op.add_option("--sample", dest="sample", default="uniform",
                  help="Sampling strategy to use (bucket | uniform)")
    op.add_option("-k", dest="k", default=1000, type="int",
                  help="Number of users to sample")
    op.add_option("--buckets", "-b", dest="buckets", default=10, type="int",
                  help="Number of buckets to use")

    (opts, args) = op.parse_args()

    app = SampleUsers(opts)
    app.run()
