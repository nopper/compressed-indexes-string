import os
import sys
import gzip
import math
import executor
import tabulate
import tempfile
import networkx as nx

SEQUENCES = (
    "block_varint",
    "block_optpfor",
    "block_interpolative",
    "ef",
    "single",
    "uniform",
    "opt",
    "fixed"
)

DUMMY_INDEX = "fixed_simple"

SIMPLE_INDICES = (
    "block_varint_simple",
    "block_optpfor_simple",
    "block_interpolative_simple",
    "ef_simple",
    "single_simple",
    "uniform_simple",
    "opt_simple",
    "fixed_simple",
)

COVERAGE_INDICES = (
    "block_varint_coverage",
    "block_optpfor_coverage",
    "block_interpolative_coverage",
    "ef_coverage",
    "single_coverage",
    "uniform_coverage",
    "opt_coverage",
    "fixed_coverage",
)

TOPK_INDICES = (
    "block_varint_topk",
    "block_optpfor_topk",
    "block_interpolative_topk",
    "ef_topk",
    "single_topk",
    "uniform_topk",
    "opt_topk",
    "fixed_topk",
)


def generate_names(filename, k):
    width = math.ceil(math.log(k) / math.log(2))
    fmt = '{:0%db}' % width

    with gzip.open(filename, 'w') as output:
        for i in range(k):
            encoded = fmt.format(i)
            encoded = encoded.replace("0", ".").replace("1", ":")
            entry = "%d\t%s\n" % (i, encoded)
            output.write(entry)


def generate_id_mapping(filename, outputfname):
    executor.execute(
        "gunzip -c %(filename)s | "
        "sort -f -k2 | awk '{print $1}' | "
        "gzip -c > %(outputfname)s" % locals())


def reshape_graph(graph_file, id_mapping_file, output_graph_file):
    executor.execute(
        "gunzip -c %(graph_file)s | ../reassign_ids "
        "--translation=UserIdToSortId --input-ids=%(id_mapping_file)s |"
        "sort -k1n -k2n | gzip -c > %(output_graph_file)s" % locals())


def build_index(graph_file, index_file, encoding, universe, verification=False):
    executor.execute(
        "gunzip -c %(graph_file)s | ../create_index "
        "-u %(universe)s -t %(encoding)s -o %(index_file)s" % locals())

    if verification:
        executor.execute(
            "gunzip -c %(graph_file)s | ../verify_index "
            "-i %(index_file)s -t %(encoding)s" % locals()
        )


def build_dictionary(name_file, dict_file, dict_type="permuterm"):
    executor.execute(
        "gunzip -c %(name_file)s |"
        "sort -f -k2 |"
        "awk '{print $2}' | ../create_dictionary "
        "--type %(dict_type)s --output %(dict_file)s" % locals())


def generate_queries(user_id, queries_file,
                     index_file, encoding, id_mapping_file, name_file, k=1):
    executor.execute(
        "echo %(user_id)s | ../sample_queries "
        "--k %(k)s --index-type %(encoding)s --input %(index_file)s "
        "--id-mapping %(id_mapping_file)s --str-mapping %(name_file)s "
        " > %(queries_file)s" % locals())


def execute_queries(scheme, queries_file, index_file, encoding,
                    id_mapping_file, dict_file, dict_type="permuterm",
                    dict_remapping="", iterations=1):
    return executor.execute(
        "../simple_scheme "
        "--query-file %(queries_file)s "
        "--dict-type %(dict_type)s --dictionary %(dict_file)s "
        "--index-type %(encoding)s --input %(index_file)s "
        "--id-mapping %(id_mapping_file)s --scheme %(scheme)s "
        "--dict-remapping %(dict_remapping)s "
        "--iterations %(iterations)s" % locals(), capture=True)


def expand_graph(graph_file, output_graph_file, index_file, encoding, k=2):
    executor.execute(
        "gunzip -c %(graph_file)s | ../friends_at_k "
        "--input %(index_file)s --index-type %(encoding)s |"
        "gzip -c > %(output_graph_file)s" % locals())


def get_stats(index_file, encoding):
    element = []
    output = executor.execute(
        "../index_stats --input %(index_file)s "
        "--index-type %(encoding)s" % locals(), capture=True)
    for el in output.split(' '):
        var, val = el.split('=')
        element.append(('.' in val) and (float(val)) or(int(val)))
    return element


def generate_random_graph(output_graph_file, nodes=100, edges=10, seed=42):
    # graph = nx.barabasi_albert_graph(nodes, edges, seed)
    graph = nx.powerlaw_cluster_graph(nodes, edges, 0.05, seed)

    with gzip.open(output_graph_file, 'w') as output:
        for src, dst in graph.to_directed().edges():
            output.write("%d\t%d\n" % (src, dst))


class Builder(object):
    def __init__(self, datasetpath, attributes):
        self.datasetpath = datasetpath
        self.attributes = attributes
        self.dictionary_types = ("permuterm", "strarray")
        self.indexpath = os.path.join(self.datasetpath, "indices")

        if not os.path.exists(self.indexpath):
            os.mkdir(self.indexpath)

    def prepare(self):
        self.collect_statistics()

        for attribute in self.attributes:
            self.generate_attribute_graph(attribute)

        self.compact_graph()

    def collect_statistics(self):
        datasetpath = self.datasetpath

        print "Gathering degree statistics ..."
        executor.execute(
            "gunzip -c %(datasetpath)s/graph.tsv.gz | "
            "awk '{print $1}' | uniq -c | awk '{print $2 \"\\t\" $1}' | "
            "gzip -c > %(datasetpath)s/degree.stats.gz" % locals())
        executor.execute(
            "gunzip -c %(datasetpath)s/degree.stats.gz | "
            "awk '{print $2}' | LC_ALL=C sort -k1n | uniq -c | "
            "awk '{print $2 \"\\t\" $1}' |"
            "gzip -c > %(datasetpath)s/degree-cdf.stats.gz" % locals())

        print "Generating random ranking ..."
        executor.execute(
            "gunzip -c %(datasetpath)s/graph.tsv.gz | "
            "awk '{print $1 \"\\n\" $2}' | "
            "LC_ALL=C sort -k1n -u | LC_ALL=C sort --random-sort |"
            "awk '{print $1 \"\\t\" NR-1}' | sort -k1n -k2n | "
            "gzip -c > %(datasetpath)s/ranking.tsv.gz" % locals())

        print "Generating universe ..." % locals()
        executor.execute(
            "gunzip -c %(datasetpath)s/degree.stats.gz | "
            "tail -n1 | awk '{print $1}' | "
            "awk '{ sum += $1; } END { print sum + 1; }' \"$@\" "
            "> %(datasetpath)s/universe" % locals())

    def generate_attribute_graph(self, attribute):
        datasetpath = self.datasetpath
        attribute_path = os.path.join(self.datasetpath, attribute)

        if not os.path.exists(attribute_path):
            os.mkdir(attribute_path)

        print "Sorting IDs lexicographically (%(attribute)s)" % locals()
        executor.execute(
            "gunzip -c %(attribute_path)s/attrs.gz | "
            "LC_ALL=C sort -k2 -f | awk '{print $1}' | "
            "gzip -c > %(attribute_path)s/ids-by-attr.gz" % locals())

        print "Generating universe for %(attribute)s" % locals()
        executor.execute(
            "gunzip -c %(attribute_path)s/ids-by-attr.gz | "
            "wc -l > %(attribute_path)s/universe" % locals())

        print "Reshaping graph according to %(attribute)s "\
              "lexicographic order ..." % locals()
        executor.execute(
            "gunzip -c %(datasetpath)s/graph.tsv.gz |"
            "../reassign_ids -i %(attribute_path)s/ids-by-attr.gz"
            " --translation UserIdToSortId | "
            "LC_ALL=C sort -k1n -k2n | "
            "gzip -c > %(attribute_path)s/graph.tsv.gz" % locals())

        print "Creating %(attribute)s dictionary ..." % locals()
        executor.execute(
            "gunzip -c %(attribute_path)s/attrs.gz |"
            "awk -F'\\t' '{print $2}' |"
            "LC_ALL=C sort -f | "
            "gzip -c > %(attribute_path)s/dict.gz" % locals())

        for dictionary in self.dictionary_types:
            print "Creating %(dictionary)s for %(attribute)s ..." % locals()
            executor.execute(
                "gunzip -c %(attribute_path)s/dict.gz | "
                "../create_dictionary --type %(dictionary)s"
                " -o %(attribute_path)s/dict.%(dictionary)s" % locals())

        print "Reshaping ranking ..."
        executor.execute(
            "gunzip -c %(datasetpath)s/ranking.tsv.gz | "
            "../reassign_ids -i %(attribute_path)s/ids-by-attr.gz"
            " --translation UserIdToSortId -c 0 | "
            "LC_ALL=C sort -k1n | "
            "gzip -c > %(attribute_path)s/ranking.tsv.gz" % locals())

        print "Creating WAND data ..."
        executor.execute(
            "gunzip -c %(attribute_path)s/graph.tsv.gz | "
            "../reassign_ids -i %(attribute_path)s/ranking.tsv.gz"
            " --translation UserIdToString -c 1 | "
            "LC_ALL=C sort -k1n -k2nr | awk '!seen[$1]++' | "
            "gzip -c > %(attribute_path)s/wand.tsv.gz" % locals())

    def compact_graph(self):
        attribute = "_id"
        attributes = self.attributes
        datasetpath = self.datasetpath
        attribute_path = os.path.join(self.datasetpath, attribute)

        if not os.path.exists(attribute_path):
            os.mkdir(attribute_path)

        print "Compacting IDs of the orignal graph ..."
        executor.execute(
            "gunzip -c %(datasetpath)s/graph.tsv | "
            "awk '{print $1 \"\\n\" $2}' | "
            "sort -k1n -u | "
            "gzip -c > %(attribute_path)s/ids-by-attr.gz" % locals())

        print "Generating universe for %(attribute)s" % locals()
        executor.execute(
            "gunzip -c %(attribute_path)s/ids-by-attr.gz | "
            "wc -l > %(attribute_path)s/universe" % locals())

        print "Reshaping graph according to %(attribute)s "\
              "lexicographic order ..." % locals()
        executor.execute(
            "gunzip -c %(datasetpath)s/graph.tsv.gz |"
            "../reassign_ids -i %(attribute_path)s/ids-by-attr.gz"
            " --translation UserIdToSortId | "
            "LC_ALL=C sort -k1n -k2n | "
            "gzip -c > %(attribute_path)s/graph.tsv.gz" % locals())

        for attr in attributes:
            print "Generating dict-remapping for %(attr)s ..." % locals()
            executor.execute(
                "gunzip -c %(datasetpath)s/%(attr)s/ids-by-attr.gz | "
                "../reassign_ids -i %(attribute_path)s/ids-by-attr.gz"
                " --translation UserIdToSortId -c 0 | "
                "gzip -c >"
                " %(attribute_path)s/dict-remapping.%(attr)s" % locals())
            executor.execute(
                "gunzip -c %(attribute_path)s/ids-by-attr.gz | "
                "../reassign_ids -i %(datasetpath)s/%(attr)s/ids-by-attr.gz"
                " --translation UserIdToSortId -c 0 | "
                "gzip -c >"
                " %(attribute_path)s/fast-dict-remapping.%(attr)s" % locals())

        print "Reshaping ranking ..."
        executor.execute(
            "gunzip -c %(datasetpath)s/ranking.tsv.gz | "
            "../reassign_ids -i %(attribute_path)s/ids-by-attr.gz"
            " --translation UserIdToSortId -c 0 | "
            "LC_ALL=C sort -k1n | "
            "gzip -c > %(attribute_path)s/ranking.tsv.gz" % locals())

        print "Creating WAND data ..."
        executor.execute(
            "gunzip -c %(attribute_path)s/graph.tsv.gz | "
            "../reassign_ids -i %(attribute_path)s/ranking.tsv.gz"
            " --translation UserIdToString -c 1 | "
            "LC_ALL=C sort -k1n -k2nr | awk '!seen[$1]++' | "
            "gzip -c > %(attribute_path)s/wand.tsv.gz" % locals())


    def build_index(self, attribute, encoding,
                    ranking=False, verification=True):

        attribute_path = os.path.join(self.datasetpath, attribute)
        index_file = os.path.join(self.indexpath,
                                  "%(attribute)s-%(encoding)s" % locals())

        universe = int(open(os.path.join(attribute_path, "universe")).read())

        print "Creating %(attribute)s index with %(encoding)s ..." % locals()

        command = "gunzip -c %(attribute_path)s/graph.tsv.gz | " \
            "../create_index" \
            " -u %(universe)d" \
            " -t %(encoding)s" \
            " -o %(index_file)s" % locals()

        if ranking:
            command += " --ranking %(attribute_path)s/ranking.tsv.gz" % locals()

        executor.execute(command)

        if verification:
            self.verify_index(attribute, encoding)

    def verify_index(self, attribute, encoding):
        attribute_path = os.path.join(self.datasetpath, attribute)
        index_file = os.path.join(self.indexpath,
                                  "%(attribute)s-%(encoding)s" % locals())

        print "Verifying %(attribute)s index with %(encoding)s ..." % locals()

        executor.execute(
            "gunzip -c %(attribute_path)s/graph.tsv.gz | ../verify_index "
            "-t %(encoding)s -i %(index_file)s" % locals())

    def sample_queries(self, user_id, attribute):
        encoding = SIMPLE_INDICES[0]
        attribute_path = os.path.join(self.datasetpath, attribute)
        index_file = os.path.join(self.indexpath, "%(encoding)s" % locals())
        universe = int(open(os.path.join(attribute_path, "universe")).read())

        executor.execute(
            "gunzip -c %(attribute_path)s/graph.tsv.gz | "
            "../create_index"
            " -u %(universe)d"
            " -t %(encoding)s"
            " -o %(index_file)s" % locals())

        print "Verifying %(attribute)s index with %(encoding)s ..." % locals()

        executor.execute(
            "gunzip -c %(attribute_path)s/graph.tsv.gz | ../verify_index "
            "-t %(encoding)s -i %(index_file)s" % locals())

        print "Sampling queries ..."
        executor.execute(
            "echo %(user_id)d | "
            "../sample_queries -t %(encoding)s -i %(index_file)s"
            " --id-mapping %(attribute_path)s/ids-by-attr.gz"
            " --str-mapping %(attribute_path)s/attrs.gz"
            "> %(attribute_path)s/queries.txt" % locals())

    def intersection(self, scheme, query_attribute, attribute, encoding,
                     dictionary="permuterm", iterations='1'):

        datasetpath = self.datasetpath
        attribute_path = os.path.join(self.datasetpath, query_attribute)
        index_file = os.path.join(self.indexpath,
                                  "%(attribute)s-%(encoding)s" % locals())

        if query_attribute != attribute:
            if scheme.startswith('fast-'):
                dict_remapping = '%(datasetpath)s/_id/' \
                                 'fast-dict-remapping.%(query_attribute)s' % locals()
            else:
                dict_remapping = '%(datasetpath)s/_id/' \
                                 'dict-remapping.%(query_attribute)s' % locals()
        else:
            dict_remapping = '\'\''

        print "Executing queries ..."

        return executor.execute(
            "../simple_scheme "
            "--query-file %(attribute_path)s/queries.txt "
            "--dict-type %(dictionary)s "
            "--dictionary %(attribute_path)s/dict.%(dictionary)s "
            "--index-type %(encoding)s --input %(index_file)s "
            "--id-mapping %(datasetpath)s/%(attribute)s/ids-by-attr.gz "
            "--scheme %(scheme)s "
            "--dict-remapping %(dict_remapping)s "
            "--iterations %(iterations)s" % locals(), capture=True)

    def topk(self, scheme, query_attribute, attribute, encoding, topk=10,
             dictionary="permuterm", iterations='2'):

        datasetpath = self.datasetpath
        attribute_path = os.path.join(self.datasetpath, query_attribute)
        index_file = os.path.join(self.indexpath,
                                  "%(attribute)s-%(encoding)s" % locals())

        if query_attribute != attribute:
            if scheme.startswith('fast-'):
                dict_remapping = '%(datasetpath)s/_id/' \
                                 'fast-dict-remapping.%(query_attribute)s' % locals()
            else:
                dict_remapping = '%(datasetpath)s/_id/' \
                                 'dict-remapping.%(query_attribute)s' % locals()
        else:
            dict_remapping = '\'\''

        if 'wand' in scheme:
            wand_data = '%(datasetpath)s/%(attribute)s/wand.tsv.gz' % locals()
        else:
            wand_data = '\'\''

        print "Executing queries ..."

        return executor.execute(
            "../simple_scheme "
            "--query-file %(attribute_path)s/queries.txt "
            "--dict-type %(dictionary)s "
            "--dictionary %(attribute_path)s/dict.%(dictionary)s "
            "--index-type %(encoding)s --input %(index_file)s "
            "--id-mapping %(datasetpath)s/%(attribute)s/ids-by-attr.gz "
            "--scheme %(scheme)s "
            "--dict-remapping %(dict_remapping)s "
            "--ranking %(datasetpath)s/%(attribute)s/ranking.tsv.gz "
            "--topk %(topk)d "
            "--wand %(wand_data)s "
            "--iterations %(iterations)s" % locals(), capture=True)

    def stats(self, attribute, encoding):
        index_file = os.path.join(self.indexpath,
                                  "%(attribute)s-%(encoding)s" % locals())
        return get_stats(index_file, encoding)
