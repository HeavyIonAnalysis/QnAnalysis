# QnAnalysis
[![Build Status](https://travis-ci.com/HeavyIonAnalysis/QnAnalysis.svg?branch=master)](https://travis-ci.com/HeavyIonAnalysis/AnalysisTreeQA)

Package for any Q-vector-based analysis. Using [QnTools](https://github.com/HeavyIonAnalysis/QnTools/) framework for Q-vector multidimentional corrections and operations. 
Input data are expected to be in [AnalysisTree](https://github.com/HeavyIonAnalysis/AnalysisTree/) format.

## Installation

### Requirements:
 * ROOT 6.20 (with MathMore)
 * C++17 compatible compiler
 * CMake 3.13

### Prerequisites:
   
 * [Boost](https://www.boost.org/) library (for program arguments)
 
 ### External packages (installed automatically)
 * QnTools
 * AnalysisTree
 * yaml-cpp

### Building from the sources
     git clone https://github.com/HeavyIonAnalysis/QnAnalysis.git
     cd QnAnalysis
     mkdir build install
     cd build
     cmake -DCMAKE_INSTALL_PREFIX=../install ../
     make -j install
     
In case of problems please [submit an issue](https://github.com/HeavyIonAnalysis/QnAnalysis/issues/new/choose).

## Running example

Coming soon...