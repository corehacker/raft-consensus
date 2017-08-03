#!/bin/bash

github="https://github.com/google/protobuf/releases/download/v3.3.0/protobuf-cpp-3.3.0.tar.gz";
destination="protobuf-cpp-3.3.0.tar.gz";

wget $github &&\
  tar xvf $destination &&\
  cd protobuf-3.3.0 &&\
  ./autogen.sh &&\
  ./configure &&\
  make
