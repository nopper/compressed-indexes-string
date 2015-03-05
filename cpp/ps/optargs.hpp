#pragma once
#include <iostream>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

#define PARSE_ARGUMENTS(ARGUMENTS)                                    \
    po::options_description desc("Allowed options");                  \
    desc.add_options()                                                \
        ARGUMENTS                                                     \
    ;                                                                 \
    po::variables_map vm;                                             \
    try                                                               \
    {                                                                 \
        po::store(po::parse_command_line(argc, argv, desc), vm);      \
        if (vm.count("help"))                                         \
        {                                                             \
            std::cout << desc << std::endl;                           \
            return -1;                                                \
        }                                                             \
        po::notify(vm);                                               \
    }                                                                 \
    catch(boost::program_options::required_option& e)                 \
    {                                                                 \
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl; \
        std::cout << desc << std::endl;                               \
        return -1;                                                    \
    }                                                                 \
    catch(boost::program_options::error& e)                           \
    {                                                                 \
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl; \
        std::cout << desc << std::endl;                               \
        return -1;                                                    \
    }

