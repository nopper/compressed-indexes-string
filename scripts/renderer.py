import sys
from itertools import groupby
from optparse import OptionParser
from tabulate import tabulate
from matplotlib import rc


def mean(items):
    return sum(items) * 1.0 / len(items)


def variance(items):
    avg = mean(items)
    var = map(lambda x: (x - avg) ** 2, items)
    return mean(var)


class Renderer(object):
    def __init__(self, options):
        self.opts = options
        self.query_len = options.query_len
        self.skip_first = options.skip

    def compare_mean_times(self, label_file_strings):
        rows = [
            ["Method",
             "Mean. PS time",
             "Var. PS time",
             "Mean time",
             "var. time"]
        ]

        for lbl_file in label_file_strings:
            label, filenames = lbl_file.split('=')
            row = [label] + list(self.mean_time_per_method(filenames))
            rows.append(row)

        self.print_table(rows)

    def compare_sizes(self, label_file_strings):
        rows = [
            ["Method",
             "Documents",
             "Elements",
             "Docs bytes",
             "Postings bytes",
             "Elements/doc",
             "Bits/doc",
             "Bits/element",
             "Ctime"]
        ]

        for lbl_file in label_file_strings:
            label, filename = lbl_file.split('=')
            row = [label] + list(self.size_per_method(filename))
            rows.append(row)

        self.print_table(rows)

    def print_table(self, rows):
        if self.opts.format == "tsv":
            for row in rows[1:]:
                output = []
                for cell in row:
                    if isinstance(cell, float):
                        output.append("%.2f" % cell)
                    else:
                        output.append(str(cell))
                print '\t'.join(output)
        elif self.opts.format == 'pdf':
            self.plot_graph(rows)
        else:
            if self.opts.format == 'latex':
                print "%% Run with: %s" % ' '.join(sys.argv)

            print tabulate(rows, headers="firstrow", floatfmt=".3f",
                           tablefmt=self.opts.format)

    def plot_graph(self, rows, point=3):
        # We skip the headers
        headers, rows = rows[1], rows[1:]

        rows.sort(key=lambda x: x[0])

        import matplotlib.pyplot as plt
        fig = plt.figure(1, figsize=(8, 5))
        ax = fig.add_subplot(111)

        rc('font', **{'family': 'serif', 'serif': ['Palatino']})
        rc('text', usetex=True)

        # ax.set_xscale('log')
        ax.set_yscale('log')

        legend_handles = []
        legend_labels = []

        max_x = 0

        colors = self.opts.colors
        markers = self.opts.markers
        offsets = map(lambda x: map(int, x.split(';')),
                      self.opts.offsets.split(','))

        for idx, (key, lst) in enumerate(groupby(rows, key=lambda x: x[0])):
            label = key
            points = list(lst)

            ys = map(lambda x: x[point], points)
            xs = range(len(ys))
            max_x = max(len(ys), max_x)

            # marker='d'
            h1, = ax.plot(xs, ys, linestyle='--', color=(0.5, 0.5, 0.5))
            h2, = ax.plot(xs, ys, markers[idx] + colors[idx])

            print "Plotting", label, "with index", idx, offsets[idx % len(offsets)]

            for i, lbl in enumerate(ys):
                ax.annotate("%d" % lbl,
                            xy=(xs[i], ys[i]),
                            xytext=offsets[idx % len(offsets)], ha='left',
                            textcoords='offset points', fontsize=9)
                            # arrowprops=dict(arrowstyle='->', shrinkA=0))

            handle = (h1, h2)
            legend_handles.append(handle)
            legend_labels.append(label)

        if self.opts.ylabel:
            ax.set_ylabel(self.opts.ylabel)
        if self.opts.xlabel:
            ax.set_xlabel(self.opts.xlabel)
        if self.opts.title:
            ax.set_title(self.opts.title)
        if self.opts.xticks:
            ax.set_xticks(range(max_x))
            ax.set_xticklabels(self.opts.xticks.split(","), range(max_x), rotation=45)

        for pos in ['top', 'bottom', 'right', 'left']:
            ax.spines[pos].set_edgecolor('gray')

        ax.legend(legend_handles, legend_labels, loc=4)
        fig.savefig(self.opts.output, bbox_inches='tight')

    # Utilities functions

    def iterate_results(self, filenames):
        if ',' in filenames:
            filenames = filenames.rstrip(',').split(',')
        else:
            filenames = [filenames]

        results = []

        for filename in filenames:
            with open(filename) as inputfile:
                for line in inputfile:
                    args = line.strip().split('\t')
                    iteration = int(args[0])
                    user_id = int(args[1])
                    query = args[2]
                    l = int(args[3])
                    r = int(args[4])
                    results_size = int(args[5])
                    ps_time = int(args[6]) / 1000.0
                    inter_time = int(args[7]) / 1000.0

                    if self.skip_first > iteration:
                        continue

                    if self.query_len is not None and len(query) != self.query_len:
                        continue

                    results.append((user_id, query, ps_time, inter_time))


        for user_id, query, ps_time, inter_time in sorted(results, key=lambda x: (x[0], x[1])):
            yield user_id, query, ps_time, inter_time

    def group_results(self, filenames):
        for k, results in groupby(self.iterate_results(filenames),
                                  key=lambda x: (x[0], x[1])):
            results = list(results)
            # print results
            yield k[0], k[1],                                    \
                mean([i[2] for i in results[self.skip_first:]]), \
                mean([i[3] for i in results[self.skip_first:]])

    def mean_time_per_method(self, filenames):
        red_ps_times = []
        red_inter_times = []
        for _, _, red_ps_time, red_inter_time in self.group_results(filenames):
            red_ps_times.append(red_ps_time)
            red_inter_times.append(red_inter_time)

        return mean(red_ps_times), variance(red_ps_times), \
            mean(red_inter_times), variance(red_inter_times)

    def size_per_method(self, filename):
        with open(filename) as inputfile:
            for line in inputfile:
                docs, elements, docs_bytes, postings_bytes, \
                    elements_node, bits_doc, bits_element, ctime = \
                    line.split('\t')

                docs = int(docs)
                elements = int(elements)
                docs_bytes = int(docs_bytes)
                postings_bytes = int(postings_bytes)
                elements_node = float(elements_node)
                bits_doc = float(bits_doc)
                bits_element = float(bits_element)
                ctime = int(ctime)

                return docs, elements, docs_bytes, postings_bytes, \
                    elements_node, bits_doc, bits_element, ctime


if __name__ == "__main__":
    op = OptionParser()
    op.add_option("--query-len", dest="query_len", default=None, type="int",
                  help="Only consider queries with defined length")
    op.add_option("--compare", dest="compare_by", default="time",
                  help="Comparison method (default: time)")
    op.add_option("--format", dest="format", default="latex",
                  help="Format of output")
    op.add_option("--skip", dest="skip", default=0, type="int",
                  help="Number of measurements to skip")
    op.add_option("--output", dest="output", metavar="FILE",
                  help="Output file")
    op.add_option("--ylabel", dest="ylabel", default=None,
                  help="Label on the y-axis")
    op.add_option("--xlabel", dest="xlabel", default=None,
                  help="Label on the x-axis")
    op.add_option("--title", dest="title", default=None,
                  help="Title for the graph")
    op.add_option("--xticks", dest="xticks", default=None,
                  help="X ticks")
    op.add_option("--colors", dest="colors", default="rb",
                  help="Colors")
    op.add_option("--markers", dest="markers", default="od",
                  help="Markers")
    op.add_option("--offsets", dest="offsets", default="-5;5",
                  help="Offsets")

    (opts, args) = op.parse_args()

    app = Renderer(opts)

    if opts.compare_by == 'time':
        app.compare_mean_times(args)
    elif opts.compare_by == 'size':
        app.compare_sizes(args)
    else:
        raise Exception("Unknown compare method: %s" % opts.compare_by)
