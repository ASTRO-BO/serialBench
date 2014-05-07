# See README.txt.

.PHONY: all cpp java python clean

all: serialBench

clean:
	rm -f serialBench
	rm -f packet.pb.h packet.pb.cc
	rm -f protoc_middleman

protoc_middleman: packet.proto
	protoc --cpp_out=. packet.proto
	@touch protoc_middleman

serialBench: serialBench.cpp protoc_middleman
	pkg-config --cflags protobuf  # fails if protobuf is not installed
	c++ serialBench.cpp packet.pb.cc -o serialBench `pkg-config --cflags --libs protobuf`
