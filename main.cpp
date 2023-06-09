/*
 *
 * This is meant to be the main source code for our project, after deleting 
 * our tests sources
 *
 * */

// Dependencies

#include "openfhe.h"

// header files needed for serialization
#include "ciphertext-ser.h"
#include "cryptocontext-ser.h"
#include "key/key-ser.h"
#include "scheme/ckksrns/ckksrns-ser.h"

#define PROFILE
using namespace lbcrypto;

const std::string DATAFOLDER = "Data";
const std::string DATASETFOLDER = "../dataset";

#include <iostream>
#include <vector>
#include <math.h>
#include <fstream>
#include <sstream>
#include <string>
#include <random>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



vector<string> split(const string &s, char delim);

void accuracy_calc(const vector <double>&y, const vector< vector <double>>&y_hat, unsigned data_size);

vector<vector<double>> readFile(string filename);

void SerializeCryptoContext(CryptoContext<DCRTPoly> cc, KeyPair<DCRTPoly> keys);	

void packEncrypt(vector<vector<double>> data);

int main(int argc, const char * argv[])
{

	if (argc < 2)
	{
		printErrorMess();
		return 0;
	}
    switch (argv[1][0]) {
        case 'e':
		// Create context and set its params
		
		uint32_t multDepth = 11;	// multiplication depth
		uint32_t scaleModSize = 50;	// scale module
    		uint32_t batchSize = 1<<14;	// batch size or how many slots per pack

		CCParams<CryptoContextCKKSRNS> parameters;
		parameters.SetMultiplicativeDepth(multDepth);
		parameters.SetScalingModSize(scaleModSize);
		parameters.SetBatchSize(batchSize);
		parameters.SetScalingTechnique(FLEXIBLEAUTO);
		parameters.SetSecurityLevel(HEStd_128_classic);

		CryptoContext<DCRTPoly> cc = GenCryptoContext(parameters);

		// Enable the features that you wish to use
		cc->Enable(PKE);
		cc->Enable(KEYSWITCH);
		cc->Enable(LEVELEDSHE);
		cc->Enable(ADVANCEDSHE);

		std::cout << "CKKS scheme is using ring dimension " << cc->GetRingDimension() << std::endl;

		std::cout <<  std::endl << "The cryptocontext has been generated." << std::endl;
		
		// Generate public-secret key pair
		KeyPair<DCRTPoly> keys = cc->KeyGen();
	    	
		// Generate the relinearization key
    		cc->EvalMultKeyGen(keys.secretKey);
	
		SerializeCryptoContext(cc, keys);	
		


		char *args[] = { (char *)"./../dataset/download_mnist.py", (char*)NULL};
	    	int i = execvp(args[0], args);
	    	if(i!=0) perror("Error running downloader script:");
	     
	    	vector<vector<double>> features = readFile("test_features.txt");
	    	cout << "x_train loaded w/ shape: (" << X_train.size()<< ", " << X_train[0].size() << ")\n";
	    
	    	packEncrypt(X_train); 
	    	break;
        case 'd':
            std::cout << "Function Decrypt." << std::endl;
            break;
        case 'i':
            std::cout << "Function Inference." << std::endl;

            break;
         case 't':
	    /*
	     * Add activation function checks
	     * */
	    char *args[] = { (char *)"./../train.py", (char*)"25", (char*) "0.001", (char*) "relu", (char *) NULL};
            execvp(args[0], args);
	    perror("exec error:");
            break;
        default:
	    printErrorMess();
            break;
	
    	}
	return 0;	

}

/*
 * An helper function to explain the program usage
 * */
void printErrorMess()
{
        	std::cout << "Program usage: ./encrypt <mode>" << std::endl;
        	std::cout << "Modes:"<< std::endl;
        	return 1;
}

/*
 * split
 * args: s - the string which will be split in a string vector
 * delim - the char separating each element in a string
 *
 * returns: a vector of strings
 *
 * description: splits a string by every delim char it finds
 * Basically if delim == ',', it separates a csv line
 * An helper function to read the dataset
 *
 * */

vector<string> split(const string &s, char delim) {
    stringstream ss(s);
    string item;
    vector<string> tokens;
    while (getline(ss, item, delim)) {
        tokens.push_back(item);
    }
    return tokens;
}


/*
 * accuracy_calc 
 * args: y - the correct labels
 * y_hat - the results of the model computation on a dataset, should have shape (n_classes, dataset_size) 
 * data_size - dataset size
 *
 * returns: nothing 
 *
 * description: compares computed results with the the given labels and prints the accuracy in percentage
 * 
 * */

void accuracy_calc(const vector <double>&y, const vector< vector <double>>&y_hat, unsigned data_size){

        unsigned BATCH_SIZE = data_size;
        int counter=0, index=0;
        float max=0.;
        vector<float> max_save;

	cout << "y hat has shape: (" << y_hat.size() << ", " << y_hat[0].size() << ")\n";

        for (unsigned i=0; i<BATCH_SIZE; i++)
        {
                index=0;
                max=0.;
                for(unsigned j=0; j < 10; j++)
                {
                        if(y_hat[j][i]>max)
                        {
                                max = y_hat[j][i];
                                index=j;
                        };
                };
                if(y[i]==index) counter++;
                max_save.push_back(index);
        };
        float count = counter;
        cout << "       Accuracy = " << (count/BATCH_SIZE)*100 << "% " << counter << " right answers" << "\n";
}

/*
 * readFile 
 * args: filename - string with the path to the dataset_file 
 *
 * returns: a matrix(vector of vectors) with shape (dataset_size, n_features)
 * the features will already scaled doubles between 0 and 1 
 * 
 * description: reads the data from file and returns it to perform computations on it 
 * 
 * */

vector<vector<double>> readFile(string filename){
	ifstream input (DATASETFOLDER + filename);

	vector<vector<double>> data;
	if(input.is_open())
	{
		string line;
		vector<string> line_v;
		while (getline (input,line) )
	 	{
			line_v = split(line, '\t');
			vector<double>  temp;
			unsigned size = static_cast<int>(line_v.size());
			for (unsigned i = 0; i < size; ++i) {
				temp.push_back(strtof((line_v[i]).c_str(),0));
			}
			data.push_back(temp);
		}
	}
	else cout << "Unable to open file" << '\n';
	input.close();

	return data;
}

/*
 * packEncrypt 
 * args: data - matrix saving the various data elements, for optimal
 * computation should have shape(n_elements, n_features)
 *
 * returns: a vector with the encrypted ciphertexts 
 * 
 * description: Packs data in ckks vectors, the encrypts it
 * 
 * */

void packEncrypt(vector<vector<double>> data)
{	
	std::vector<ConstCiphertext<DCRTPoly>> ciphertextVecx;
	for (unsigned i = 0; i < data.size(); ++i)
	{
        	Plaintext plaintext = cc->MakeCKKSPackedPlaintext(data[i]);
        	ciphertextVecx.push_back(cc->Encrypt(keys.publicKey, plaintext));
		data[i].clear();
		data[i].shrink_to_fit();
	}
	
	if (!Serial::SerializeToFile(DATAFOLDER + "/" + "encryptedfeatures.txt", ciphertextVecx, SerType::BINARY))
       	{
        	std::cerr << "Error writing serialization of ciphertextVecx to encryptedfeatures.txt" << std::endl;
        	exit(1);
	}

	cout << "X_train packed, encrypted and serialized to " + DATAFOLDER+"/encryptedfeatures.txt\n";
	return;	
}


void SerializeCryptoContext(CryptoContext<DCRTPoly> cc, KeyPair<DCRTPoly> keys)
{

		
		// Serialize cryptocontext
		if (!Serial::SerializeToFile(DATAFOLDER + "/cryptocontext.txt", cc, SerType::BINARY))
	       	{
			std::cerr << "Error writing serialization of the crypto context to "
					"cryptocontext.txt"
				<< std::endl;
			return 1;
		}
		std::cout << "The cryptocontext has been serialized." << std::endl;


		std::cout << "The key pair has been generated." << std::endl;

		// Serialize the private key
		if (!Serial::SerializeToFile(DATAFOLDER + "/key-private.txt", keys.secretKey, SerType::BINARY)) {
			std::cerr << "Error writing serialization of public key to key-public.txt" << std::endl;
			return 1;
		}
		std::cout << "The private key has been serialized." << std::endl;


		// Serialize the relinearization (evaluation) key for homomorphic
    		// multiplication
    		std::ofstream emkeyfile(DATAFOLDER + "/" + "key-eval-mult.txt", std::ios::out | std::ios::binary);
    		if (emkeyfile.is_open())
	       	{
        		if (cc->SerializeEvalMultKey(emkeyfile, SerType::BINARY) == false) {
            		std::cerr << "Error writing serialization of the eval mult keys to "
                         "key-eval-mult.txt"
                      	<< std::endl;
            		return 1;
        	}
        	std::cout << "The eval mult keys have been serialized." << std::endl;

        	emkeyfile.close();
    		}
    		else {
        		std::cerr << "Error serializing eval mult keys" << std::endl;
        		return 1;
    		}
}
