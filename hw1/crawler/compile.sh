#!/bin/bash

g++ crawler.cpp -std=c++0x -L/usr/lib/x86_64-linux-gnu/ -lcurl -lboost_system -lboost_filesystem -lboost_regex -lpthread -O2 -o crawler
