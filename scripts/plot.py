import gzip
from optparse import OptionParser

#from matplotlib import rc
import matplotlib.pyplot as plt


class Grapher(object):
    def __init__(self, inputfile, outputfile, title, xlabel, ylabel):
        self.inputfile = inputfile
        self.outputfile = outputfile

        self.title = title
        self.xlabel = xlabel
        self.ylabel = ylabel

    def read_values(self, inputfile):
        xs = []
        ys = []

        with gzip.open(inputfile, 'r') as inputfile:
            for line in inputfile:
                x, y = map(int, line.strip().split('\t'))
                xs.append(x)
                ys.append(y)

        return xs, ys

    def run(self):
        xs, ys = self.read_values(self.inputfile)

        fig = plt.figure(1, figsize=(8, 8))
        ax = fig.add_subplot(111)
        # rc('font', **{'family': 'serif', 'serif': ['Palatino']})
        # rc('text', usetex=True)

        ax.set_xscale('log')
        ax.set_yscale('log')
        ax.plot(xs, ys, 'rx')

        if self.ylabel:
            ax.set_ylabel(self.ylabel)
        if self.xlabel:
            ax.set_xlabel(self.xlabel)
        if self.title:
            ax.set_title(title)

        fig.savefig(self.outputfile, bbox_inches='tight')
        plt.close()


if __name__ == "__main__":
    op = OptionParser()
    op.add_option("-i", "--input", dest="input", metavar="FILE",
                  help="Input file")
    op.add_option("--ylabel", dest="ylabel", default=None,
                  help="Label on the y-axis")
    op.add_option("--xlabel", dest="xlabel", default=None,
                  help="Label on the x-axis")
    op.add_option("--title", dest="title", default=None,
                  help="Title for the graph")
    op.add_option("-o", "--output", dest="output", metavar="FILE",
                  help="Output file")

    (opts, args) = op.parse_args()

    Grapher(
        opts.input,
        opts.output,
        opts.title,
        opts.xlabel,
        opts.ylabel
    ).run()
