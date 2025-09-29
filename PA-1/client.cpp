/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Anuj Aedavelli
	UIN: 833009695
	Date: 9/28/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <fstream>
#include <iostream>

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	bool p_passed = false;
	bool t_passed = false;
	bool e_passed = false;
	bool f_passed = false;
	bool c_passed = false;
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				p_passed=true;
				break;
			case 't':
				t = atof (optarg);
				t_passed=true;
				break;
			case 'e':
				e = atoi (optarg);
				e_passed=true;
				break;
			case 'f':
				filename = optarg;
				f_passed=true;
				break;
			case 'c': 
				c_passed = true; 
				break;
		}
	}
	pid_t server = fork();
	if(server == 0){
		char* args[] = { (char*)"./server", nullptr };
		execvp(args[0], args);
		perror("execvp failed");
		exit(-1);
	}
    FIFORequestChannel control_chan("control", FIFORequestChannel::CLIENT_SIDE);
	
	auto create_new_channel = [&](FIFORequestChannel &chan) -> FIFORequestChannel {
		MESSAGE_TYPE msg = NEWCHANNEL_MSG;
		chan.cwrite(&msg, sizeof(MESSAGE_TYPE));
		char new_channel_name[MAX_MESSAGE];
		chan.cread(new_channel_name, MAX_MESSAGE);
		FIFORequestChannel new_chan(new_channel_name, FIFORequestChannel::CLIENT_SIDE);
		return new_chan;
	};

	auto get_ecg = [&](FIFORequestChannel &chan,int person, double seconds, int ecgno) -> double {
        char buf[MAX_MESSAGE];
        datamsg msg(person, seconds, ecgno);

        memcpy(buf, &msg, sizeof(datamsg));
        chan.cwrite(buf, sizeof(datamsg));

        double reply;
        chan.cread(&reply, sizeof(double));
        return reply;
    };

	FIFORequestChannel *active_chan = &control_chan;
	FIFORequestChannel *new_chan_ptr = nullptr;
	if(c_passed) {
		new_chan_ptr = new FIFORequestChannel(create_new_channel(control_chan));
		active_chan = new_chan_ptr;
	}

	if(p_passed && t_passed && e_passed){
    	double reply = get_ecg(*active_chan,p,t,e);
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}
	else if(p_passed){
		ofstream outfile("received/x1.csv");

		for (int i = 0; i < 1000; i++) {
			double t = i * 0.004;

			double reply1 = get_ecg(*active_chan,p,t,1);
			double reply2 = get_ecg(*active_chan,p,t,2);
			outfile << t << "," << reply1 << "," << reply2 << "\n";
		}
		outfile.close();
	}

	if(f_passed){
		filemsg fm(0, 0);
		string fname = filename;
	
		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		active_chan->cwrite(buf2, len);  
		__int64_t file_length;
		active_chan->cread(&file_length, sizeof(long));	

		delete[] buf2;

		ofstream outfile("received/" + filename,ios::binary);
		__int64_t offset = 0;
		while (offset < file_length) {
    		long chunk_size = min((int)MAX_MESSAGE, (int)(file_length - offset));

    		filemsg fm(offset, chunk_size);
    		int len = sizeof(filemsg) + filename.size() + 1;
    		char* buf = new char[len];
    		memcpy(buf, &fm, sizeof(filemsg));
    		strcpy(buf + sizeof(filemsg), filename.c_str());

    		active_chan->cwrite(buf, len); 
    		char* chunk = new char[chunk_size];
    		active_chan->cread(chunk, chunk_size); 
    		outfile.write(chunk, chunk_size);

    		offset += chunk_size;
    		delete[] buf;
    		delete[] chunk;
		}
		outfile.close();
	}

	// closing the channel    
    MESSAGE_TYPE m = QUIT_MSG;
	if(c_passed && new_chan_ptr) {
    	new_chan_ptr->cwrite(&m, sizeof(MESSAGE_TYPE));
    	delete new_chan_ptr;
	}
	control_chan.cwrite(&m, sizeof(MESSAGE_TYPE));
	waitpid(server, nullptr, 0);
}


