#!/bin/bash

gcc test.c http.c httpserver_c.c ../../build/unlinked/mem* ../../build/unlinked/string* -g
