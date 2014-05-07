#include <cstdlib>
#include <ctime>
#include <iostream>
#include "packet.pb.h"

#define NANO 1000000000L
double timediff(struct timespec start, struct timespec stop){
    double secs = (stop.tv_sec - start.tv_sec) + (stop.tv_nsec - start.tv_nsec) / (double)NANO;
    return secs;
}

void setPacket(benchmark::Packet& packet)
{
	int structSize = sizeof(benchmark::Packet);

	benchmark::Packet::Header* h = packet.mutable_header();
	h->set_version(1);
	h->set_type(2);
	h->set_dhdf(3);
	h->set_apid(4);
	h->set_sf(5);
	h->set_sourcesequencecounter(6);
	h->set_packetlength(7);
	h->set_crcflag(8);
	h->set_packettype(9);
	h->set_packetsubtype(10);

	benchmark::Packet::DataFieldHeader* dfh = packet.mutable_datafieldheader();
	dfh->set_timesec(1);
	dfh->set_timens(2);
	dfh->set_arrayid(3);
	dfh->set_runnumber(4);
	dfh->set_eventnumber(5);
	dfh->set_telescopeid(6);
	dfh->set_ntriggered(7);
	dfh->set_telescopecounter(8);

	const int N = 1800;
	const int M = 40;
	benchmark::Packet::SourceDataFieldFixed* sdff = packet.mutable_sourcedatafieldfixed();
	sdff->set_n1(N);
	sdff->set_n2(0);
	sdff->set_nsamples(M);
	sdff->set_nid(0);

	for(int n=0; n<sdff->n1(); n++)
	{
		benchmark::Packet::Pixel* FADC = packet.add_fadcs();

		for(int m=0; m<sdff->nsamples(); m++)
		{
			benchmark::Packet::Pixel::Sample* sample = FADC->add_samples();
			int randNum = std::rand() / (RAND_MAX * 1.0f) * 65535; // [0-65535] (16bit)
			sample->set_value(randNum);
			structSize += sizeof(benchmark::Packet::Pixel::Sample);
		}
		structSize += sizeof(benchmark::Packet::Pixel);
	}

	for(int i=0; i<sdff->nid(); i++)
	{
		benchmark::Packet::ID* id = packet.add_pixelids();
		id->set_value(i);
		structSize += sizeof(benchmark::Packet::ID);
	}

//	std::cout << "Structure size: " << structSize << " bytes" << std::endl;
}

void printPacket(const benchmark::Packet& packet)
{
	std::cout << "Version: " << packet.header().version() << std::endl;
	std::cout << "Type: " << packet.header().type() << std::endl;
	std::cout << "DHDF: " << packet.header().dhdf() << std::endl;
	std::cout << "APID: " << packet.header().apid() << std::endl;
	std::cout << "SF: " << packet.header().sf() << std::endl;
	std::cout << "Source Sequence Counter: " << packet.header().sourcesequencecounter() << std::endl;
	std::cout << "Packet Length: " << packet.header().packetlength() << std::endl;
	std::cout << "CRC Flag: " << packet.header().crcflag() << std::endl;
	std::cout << "Packet Type: " << packet.header().packettype() << std::endl;
	std::cout << "Packet Sub Type: " << packet.header().packetsubtype() << std::endl;
	std::cout << std::endl;

	std::cout << "Time (s): " << packet.datafieldheader().timesec() << std::endl;
	std::cout << "Time (ns): " << packet.datafieldheader().timens() << std::endl;
	std::cout << "Array ID: " << packet.datafieldheader().arrayid() << std::endl;
	std::cout << "Run Number: " << packet.datafieldheader().runnumber() << std::endl;
	std::cout << "Event Number: " << packet.datafieldheader().eventnumber() << std::endl;
	std::cout << "Telescope ID: " << packet.datafieldheader().telescopeid() << std::endl;
	std::cout << "N Triggered: " << packet.datafieldheader().ntriggered() << std::endl;
	std::cout << "Telescope Counter: " << packet.datafieldheader().telescopecounter() << std::endl;
	std::cout << std::endl;

	std::cout << "N1: " << packet.sourcedatafieldfixed().n1() << std::endl;
	std::cout << "N2: " << packet.sourcedatafieldfixed().n2() << std::endl;
	std::cout << "NSamples: " << packet.sourcedatafieldfixed().nsamples() << std::endl;
	std::cout << "NID: " << packet.sourcedatafieldfixed().nid() << std::endl;

	for(int n=0; n<packet.fadcs_size(); n++)
	{
		std::cout << "Pixel " << n << std::endl;
		const benchmark::Packet::Pixel& pixel = packet.fadcs(n);


		std::cout << "Samples: " << std::endl;
		for(int m=0; m<pixel.samples_size(); m++)
			std::cout << pixel.samples(m).value() << " ";
		std::cout << std::endl;
	}

	int size = packet.pixelids().size();
	if(size > 0)
	{
		std::cout << "IDs: " << std::endl;
		for(int i=0; i<size; i++)
			std::cout << packet.pixelids(i).value() << " ";
		std::cout << std::endl;
	}
}

int main(int argc, char* argv[]) {
	// Verify that the version of the library that we linked against is
	// compatible with the version of the headers we compiled against.
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	std::srand(42);

	const int NPACKETS = 528;
	benchmark::Packet packet[NPACKETS];

	std::cout << "Initializing " << NPACKETS << " packets.." << std::endl;
	for(int i=0; i<NPACKETS; i++)
		setPacket(packet[i]);
	std::cout << "Initialization complete." << std::endl;

//	printPacket(packet[0]);

	int buffSize[NPACKETS];
	void *buffers[NPACKETS];

	struct timespec tstart, tend;

	std::cout << "Serializing " << NPACKETS << " packets.." << std::endl;
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tstart);

	for(int i=0; i<NPACKETS; i++)
	{
		buffSize[i] = packet[i].ByteSize();
		buffers[i] = new char[buffSize[i]];
		packet[i].SerializeToArray(buffers[i], buffSize[i]);
	}

	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tend);
	std::cout << "Serialized in: " << timediff(tstart,tend) << " seconds." << std::endl;

	std::cout << "Parsing " << NPACKETS << " serialized packets.." << std::endl;
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tstart);

	for(int i=0; i<NPACKETS; i++)
		packet[i].ParseFromArray(buffers[i], buffSize[i]);

	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tend);
	std::cout << "Parsed in: " << timediff(tstart,tend) << " seconds." << std::endl;

	google::protobuf::ShutdownProtobufLibrary();

	return 0;
}
