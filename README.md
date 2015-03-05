# Requirements

You need a fully working Python distribution. The dependencies are listed in the `requirements.txt` file. You can obtain a fully working Python distribution using [pyenv](https://github.com/yyuu/pyenv) by typing the following commands:

    $ pyenv virtualenv 2.7.9 2.7.9-prefix
    $ pyenv local 2.7.9-prefix
    $ pip install -r requirements.txt

You also need to install a set of third party dependencies in order to successfully compile the sources. On Mac OS you can install them using [homebrew](http://brew.sh/).

    $ brew install cmake boost pigz

On Ubuntu something the following 

In case you have a linux based system you can use the [bpt](https://pypi.python.org/pypi/bpt/0.5.2) tool and scripts that are already shipped with the source package. Check out the `bpt/` directory for further instructions.

    $ mkdir -p cpp/build && cd cpp/build
    $ cmake -DCMAKE_BUILD_TYPE=Release ..
    $ make

Testing requires a python version:

    $ make test

# Obtaining the datasets

Please refer to the paper for information on how to obtain those datasets. You should have in the `datasets/raw` directory the following files:

    90084fc73aaf9228805de6fae02db172  datasets/raw/DBLPnew/dblp-17-Oct-2014.nodes
    ee5ef196e9749647438009963e3f8d98  datasets/raw/DBLPnew/dblp-17-Oct-2014.edges
    acb915c7fb58447081d0975603ed6e45  datasets/raw/soc-LiveJournal1/name2id
    94495d91700282fe75b4a99b6b9ffefa  datasets/raw/soc-LiveJournal1/out.soc-LiveJournal1
    9c0f7983a523edd1b753af68c5acc4bd  datasets/raw/Twitter/twitter_rv.net
    35ac716e5de410a9b1f7bd65a748713a  datasets/raw/Twitter/numeric2screen

## Cleanup datasets

A set of BASH scripts are provided to reshape/cleanup the original dataset in a common format. Please note that the cleaning up of twitter dataset will take hours. Better launch the script before going to bed ;)

    $ bash ./scripts/cleanup/dblp.sh
    $ bash ./scripts/cleanup/livejournal.sh
    $ base ./scripts/cleanup/twitter.sh

# Running experiments

The first experiment reported in the paper compare all the different encodings against Friend queries:

    $ bash scripts/report/encodings.sh

The second experiment compare the two best encodings against FoF (Friends of Friends) queries:

    $ bash ./scripts/report/fof.sh

The last experiment is about top-k FoF queries:

    $ bash ./scripts/report/topk.sh

# Authors

- Paolo Ferragina <ferragina@di.unipi.it>
- Francesco Piccinno <piccinno@di.unipi.it>
- Rossano Venturini <rossano@di.unipi.it>