# QnAnalysis
[![Build Status](https://travis-ci.com/HeavyIonAnalysis/QnAnalysis.svg?branch=master)](https://travis-ci.com/HeavyIonAnalysis/QnAnalysis)

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
 * AnalysisTree (>= 1.0.11, >= 2.2.2)
 * yaml-cpp

### Building from the sources
     git clone https://github.com/HeavyIonAnalysis/QnAnalysis.git
     cd QnAnalysis
     mkdir build install
     cd build
     cmake -DCMAKE_INSTALL_PREFIX=../install ../
     make -j install
     
In case of problems please [submit an issue](https://github.com/HeavyIonAnalysis/QnAnalysis/issues/new/choose).

### Using bundled `yaml-cpp`
For some environments yaml-cpp is not available (e.g Kronos or lxplus).
To build bundled version of `yaml-cpp`, use CMake option `-Dyaml-cpp_BUNDLED=ON` 
## Running example

Coming soon...

## Correlation Task

### QnAnalysisCorrelate --help
```
Correlation options:
  --configuration-file arg         Path to the YAML configuration
  --configuration-name arg         Name of YAML node
  -i [ --input-file ] arg          Name of the input ROOT file (or .list file)
  -i [ --input-tree ] arg (=tree)  Name of the input tree
  -o [ --output-file ] arg         Name of the output ROOT file
```

`--input-file` is either a ROOT file (.root) or list of ROOT files (*.list). 
If list is provided ROOT files are chained.

`--configuration-file` is a path to configuration file. 
If configuration path is relative, runner scans `$(pwd)` or `${projectRoot}/setups`.

`--configuration-name` is a name of YAML node where runner scans correlation tasks.
Example:
```
# --configuration-name=_tasks
# config.yml:
_tasks:
  - task1...
  - task2...
  - task3
```


### Setup
Correction task is configured via YAML file.
Few examples of configuration could be found in `${projectRoot}/setups/correlation`.

Typical setup consists of
- Q-vector definitions
- axes definition
- correlation task(-s)

Exact structure is not fixed (yet) and limited only by YAML syntax and capabilities.

#### Q-vector definitions
Q-vector is the smallest building block of any correlation task. 
Q-vector entity consists of `name` and `tags`.
`name` is an identifier of Q-vector. Corresponding Q-vector MUST exist in the input tree.

Example of the detector definition:
```
name: detectorA
tags: [observable]
```

`tags` is a list of strings of arbitary size. 
With `tags` user specifies groups which given Q-vector belongs to.

In the end user must provide a list of Q-vectors:
```
_detectors: &detectors
  - name: detectorA
    tags: [observable]
  - name: detectorB
    tags: [reference]
  - name: detectorC
    tags: []
```  
Here three Q-vectors are defined. 
`detectorA` belongs to the group `observable`, `detectorB` belongs to the group `reference`, `detectorC` has no groups.

#### Axes definition

In the correlation task a list of event axes is required.
Usually we build our observable against centrality, but for particular experiment it may also be run number or anything else.
Maximum number of axis is defined during compilation from `CorrelationTaskRunner::MAX_AXES`. 
Setting large number to `MAX_AXES` may cause long compilation time and runtime performance degradation.

Example:
```
_axes:
  - &ax0
    name: Centrality
    nb: 10
    lo: 0
    hi: 100
  - &ax1
    name: Centrality
    bin-edges: [0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
```
In the example two identical axes are defined with bin edges and range specifier.


#### Tasks definition

User provides list of tasks to evaluate. 
A task consists of:
- Task arguments
- Task action 
- Number of samples for bootstrapping
- Event axes
- Folder in the output file

Amount of task arguments determines `arity` of the action. 
Maximum arity is defined in `CorrelationTaskRunner::MAX_ARITY`.
Be careful with setting large value of `MAX_ARITY`.

Each task argument is a **list** of Q-vectors or, more precisely, a `query` to the defined list of Q-vectors.
Few examples of task arguments:
```
- query: { tags: { any-in: [ wall ] } }
  query-list: *detectors
  # what are the Q-vector components participating in the correlation
  components: [x1,y1]
  # what corrections steps are participating in the correlation
  correction-steps: [ recentered ]
- query: { tags: { any-in: [ wall ] } }
  query-list: *detectors
  components: *2d1h
  correction-steps: [ recentered ]
```

`query` consists of the list of predicates. 
Predicate has a target field (name or `tags`), type of the predicate and predicate parameters.
```
tags: { any-in: [ wall ] }
```

This is the predicate on field `tags`. It selects Q-vectors with any of `tags` to by in the list: `[wall]`.

Few examples of predicates:
```
# Name is equals to 'detectorA'
- name: { equals: detectorA }
# Name matches regular expression
- name: { regex-match: "detector.*" }
# Any of tags exists in the list (set intersection)
- tags: { any-in: [ wall ] }
# ALL tags exist in the list (A in B)
- tags: { all-in: [ wall ] }
```
Query selects Q-vectors with all predicates passed (AND).
```
# Composite query with two predicates applied to different fields
- tags: { all-in: [ wall ] }
  name: { regex-match: "detector.*" }
```

User must specify components of the Q-vector that shall be used in correlation (e.g `components: [x1, y1]`).
Components are defined according to the regular expression `^(x|y|cos|sin)(\d)$` (case insensitive), where `x` and `y` are 'x' and 'y' components of the Q-vector.
`cos` and `sin` denote `x` and `y` components divided by Q-vector magnitude (event plane analysis).
Number after component denotes Q-vector harmonic.

Field `correction-steps` denote list of correction steps that will be used in the correlation.
List of the allowed values is given by the corresponding enumerator (`PLAIN`,`RECENTERED`,`TWIST`,`RESCALED`).

`weight` represents weight evaluated from given task argument (Q-vector). 
Supported values are `sumw` and `ones`.

##### Task argument combinations

Task argument definition contains query to Q-vector list, list of correction steps and list of components to evaluate.
This set of parameters is being flatten, giving `N_i = N(Q-v) x N(steps) x N(components)` combinations for `i-th` task argument.

For `m` arguments one correlation task produces `N = N_1 x N_2 x ... N_m` combinations of arguments.

##### Task action

Action is a function `(const QVector&, ...) -> float`.
This function is constructed from argument' components via **product** operation.
Similarly is constructed weight function from weight function of individual arguments.

##### Example of task definition
```
args:
  - query: { tags: { any-in: [ wall ] } }
    query-list: *detectors
    # what are the Q-vector components participating in the correlation
    components: [x1,y1,cos1,sin1]
    # what corrections steps are participating in the correlation
    correction-steps: [ recentered ]
  - query: { tags: { any-in: [ wall ] } }
    query-list: *detectors
    components: [x1,y1,cos1,sin1]
    correction-steps: [ recentered ]
n-samples: 50
weights-type: reference
folder: "/QQ/test"
axes: [ *centrality ]
```

If `weights-type` is `reference`, weight function is ignored.






