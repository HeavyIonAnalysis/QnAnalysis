FROM kreisl/rootcpp17

RUN yum install -y boost-devel

WORKDIR /opt/yaml-cpp
RUN git clone https://github.com/jbeder/yaml-cpp.git &&\
    mkdir -vp yaml-cpp-build && cd yaml-cpp-build &&\
    cmake -DYAML_BUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release ../yaml-cpp &&\
    make install
