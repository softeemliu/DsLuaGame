#!/bin/bash

for proto_file in *.proto; do 
    protoc --proto_path=./ --descriptor_set_out="${proto_file%.*}.pb" "$proto_file"
done 