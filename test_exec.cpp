#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <vector>


int main(int argc, const char * argv[]) 
{
	// char *args[] = { (char *)"./../dataset/download_mnist.py", (char*)NULL};
	char *args[] = { (char *)"./../train.py", (char*)"25", (char*) "0.001", (char*) "tanh", (char *) NULL};
        execvp(args[0], args);
	perror("exec error:");
	return 0;
}
