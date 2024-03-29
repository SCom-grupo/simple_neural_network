// Dependencies

#include "openfhe.h"

// header files needed for serialization
#include "ciphertext-ser.h"
#include "cryptocontext-ser.h"
#include "key/key-ser.h"
#include "scheme/ckksrns/ckksrns-ser.h"

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
#include <string.h>

#define PROFILE
using namespace lbcrypto;

const std::string DATAFOLDER = "Data";
const std::string DATASETFOLDER = "../dataset";

using namespace std;

vector<string> string_split(const string &s, char delim);

void accuracy_calc(const vector <int>&y, const vector< vector <double>>&y_hat);

vector<vector<double>> readFile(string filename);

void printErrorMess();

void SerializeCryptoContext(CryptoContext<DCRTPoly> cc, KeyPair<DCRTPoly> keys);	

void packEncrypt(std::vector<std::vector<double>> data, CryptoContext<DCRTPoly> cc, KeyPair<DCRTPoly> keys);

std::vector<ConstCiphertext<DCRTPoly>> CallFunc(CryptoContext<DCRTPoly> cc, std::vector<ConstCiphertext<DCRTPoly>> ciphertextVecx, std::vector<std::vector<double>> W, unsigned NumWeightLines, int lowerbound, int upperbound, int degree, string func); 

int main(int argc, char* argv[])
{

	if (argc < 2)
	{
		printErrorMess();
		return 0;
	}

    
    switch (argv[1][0]) {
        case 'p':
		{		
			// Create context and set its params
			uint32_t multDepth = 13;	// multiplication depth
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
			
			std::cout << "The key pair has been generated." << std::endl;
				
			SerializeCryptoContext(cc, keys);	
			
			vector<vector<double>> features = readFile("/test_features.txt");
			cout << "x_train loaded w/ shape: (" << features.size()<< ", " << features[0].size() << ")\n";
			
			packEncrypt(features, cc, keys); 
			// Clearing keys and context
			cc->ClearEvalMultKeys();
			lbcrypto::CryptoContextFactory<lbcrypto::DCRTPoly>::ReleaseAllContexts();
			std::cout << "Context cleared!" << std::endl;
				
			break;
		}
	case 'd':
		{
			// execute downloader script
			char *args[] = {(char*) "python3", (char *)"../dataset/download_mnist.py", (char*)NULL};
			int i = execvp(args[0], args);
			if(i!=0) perror("Error running downloader script:");
			break;
		}
    case 'e':
		{
			std::cout << "Function Decrypt." << std::endl;
				
			std::vector<ConstCiphertext<DCRTPoly>> ciphertextVecy;
			
			// Deserialize the features
			if (Serial::DeserializeFromFile(DATAFOLDER + "/" + "encryptedlabels.txt", ciphertextVecy, SerType::BINARY) == false) {
					std::cerr << "Could not read ciphertextVec" << std::endl;
					return 1;
				}
				std::cout << "ciphertextVecy has been deserialized." << std::endl;
			
			CryptoContext<DCRTPoly> cc;
			
			// Deserialize the crypto context
				if (!Serial::DeserializeFromFile(DATAFOLDER + "/cryptocontext.txt", cc, SerType::BINARY)) {
					std::cerr << "I cannot read serialization from " << DATAFOLDER + "/cryptocontext.txt" << std::endl;
					return 1;
				}
				cout << "The cryptocontext has been deserialized." << std::endl;
			
			// Deserialize secret key
			PrivateKey<DCRTPoly> sk;
			if (Serial::DeserializeFromFile(DATAFOLDER + "/key-private.txt", sk, SerType::BINARY) == false) {
				std::cerr << "Could not read secret key" << std::endl;
				return 1;
			}
			cout << "The secret key has been deserialized." << std::endl;
			

			// Decrypt results
			std::vector<std::vector<double>> classes;
			for(unsigned i = 0; i < 10; ++i){
				Plaintext resultDecrypted;
				cc->Decrypt(sk, ciphertextVecy[i], &resultDecrypted);
				std::vector<std::complex<double>> finalResult = resultDecrypted->GetCKKSPackedValue();
				std::vector<double> tempy;
				for(unsigned j = 0; j < finalResult.size(); j++)
				{
					tempy.push_back(finalResult[j].real());
				}
				classes.push_back(tempy);
				std::cout << "Decrypted line i="<< i << "/9" << std::endl;
			}
			cout << "out has shape: (" << classes.size() << ", " << classes[0].size() << ")"<< std::endl;
			ciphertextVecy.clear();
			ciphertextVecy.shrink_to_fit();
		
			// Check accuracy
			ifstream input_labels (DATASETFOLDER + "/test_labels.txt");
			vector<int> y_train;
		
			if (input_labels.is_open())
			{
				string line;
				vector<string> line_v;
				while (getline (input_labels,line) )
				{
					line_v = string_split(line, '\t');

					unsigned digit = strtof((line_v[0]).c_str(),0);
					y_train.push_back(digit);
				}
				cout << "y_train loaded w/ shape: " << y_train.size() << std::endl;
			}
			else cout << "Unable to open file" << std::endl;
			input_labels.close();
		
			accuracy_calc(y_train, classes);

			break;
		}
    case 'i':
		{    
				std::cout << "Function Inference." << std::endl;
			
			
			// Reads encrypted data from file /Encrypteddata.txt
			std::vector<ConstCiphertext<DCRTPoly>> ciphertextVecx;
			std::vector<ConstCiphertext<DCRTPoly>> ciphertextVecy;
			
			CryptoContext<DCRTPoly> cc;
			
			// Deserialize the crypto context
				if (!Serial::DeserializeFromFile(DATAFOLDER + "/cryptocontext.txt", cc, SerType::BINARY)) {
					std::cerr << "I cannot read serialization from " << DATAFOLDER + "/cryptocontext.txt" << std::endl;
					return 1;
				}
				std::cout << "The cryptocontext has been deserialized." << std::endl;

			// Deserialize the mult key
			std::ifstream emkeys(DATAFOLDER + "/key-eval-mult.txt", std::ios::in | std::ios::binary);
				if (!emkeys.is_open()) {
					std::cerr << "I cannot read serialization from " << DATAFOLDER + "/key-eval-mult.txt" << std::endl;
					return 1;
				}
				if (cc->DeserializeEvalMultKey(emkeys, SerType::BINARY) == false) {
					std::cerr << "Could not deserialize the eval mult key file" << std::endl;
					return 1;
				}
				std::cout << "Deserialized the eval mult keys." << std::endl;

			// Deserialize the features
			if (Serial::DeserializeFromFile(DATAFOLDER + "/" + "encryptedfeatures.txt", ciphertextVecx, SerType::BINARY) == false) {
				std::cerr << "Could not read ciphertextVec" << std::endl;
				return 1;
			}
			std::cout << "ciphertextVecx has been deserialized." << std::endl;

			// Reads the weight file W1
			std::vector<std::vector<double>> W1=readFile("/w1.txt");
			cout << "W1 loaded w/ shape: (" << W1.size() << ", " << W1[0].size() << ")" << std::endl;;

			// Reads the weight file W2
			std::vector<std::vector<double>> W2=readFile("/w2.txt");
			cout << "W2 loaded w/ shape: (" << W2.size() << ", " << W2[0].size() << ")" << std::endl;;

			// Reads the weight file W3
			std::vector<std::vector<double>> W3=readFile("/w3.txt");
			cout << "W3 loaded w/ shape: (" << W3.size() << ", " << W3[0].size() << ")" << std::endl;;
		
			cout << "Starting inference" << std::endl;
			
			// Perform first layer
			if (strcmp(argv[2], "relu")==0) ciphertextVecx=CallFunc(cc, ciphertextVecx, W1, 128, -7, 7, 5, "relu");
			else if (strcmp(argv[2], "tanh")==0) ciphertextVecx=CallFunc(cc, ciphertextVecx, W1, 128, -8, 10, 12, "tanh");
			else if (strcmp(argv[2], "sigmoid")==0) ciphertextVecx=CallFunc(cc, ciphertextVecx, W1, 128, -17, 16, 12, "sigmoid");
			else if (strcmp(argv[2], "square")==0) ciphertextVecx=CallFunc(cc, ciphertextVecx, W1, 128, -2, 15, 5, "square");
			else if (strcmp(argv[2], "linear")==0) ciphertextVecx=CallFunc(cc, ciphertextVecx, W1, 128, -2, 15, 5, "linear");
			else printErrorMess();	

			ciphertextVecx.shrink_to_fit();
			cout << "First Layer Completed" << std::endl;	

			// Perform second layer
			if (strcmp(argv[2], "relu")==0) ciphertextVecx=CallFunc(cc, ciphertextVecx, W2, 64, -15, 15, 5, "relu");
			else if (strcmp(argv[2], "tanh")==0) ciphertextVecx=CallFunc(cc, ciphertextVecx, W2, 64, -8, 9, 12, "tanh");
			else if (strcmp(argv[2], "sigmoid")==0) ciphertextVecx=CallFunc(cc, ciphertextVecx, W2, 64, -7, 8, 12, "sigmoid");
			else if (strcmp(argv[2], "square")==0) ciphertextVecx=CallFunc(cc, ciphertextVecx, W2, 64, -2, 15, 5, "square");
			else if (strcmp(argv[2], "linear")==0) ciphertextVecx=CallFunc(cc, ciphertextVecx, W2, 64, -2, 15, 5, "linear");
			else printErrorMess();	
			ciphertextVecx.shrink_to_fit();
			std::cout << "Second layer completed" << std::endl;

			// Perform Third layer
			for(unsigned i = 0; i < 10; ++i){
				auto result = cc->EvalLinearWSum(ciphertextVecx, W3[i]);
				ciphertextVecy.push_back(result);
			
				std::cout << "Calculated line i="<< i << "/9" << std::endl;
			}
			std::cout << "Third layer completed" << std::endl;

			if (!Serial::SerializeToFile(DATAFOLDER + "/" + "encryptedlabels.txt", ciphertextVecy, SerType::BINARY))
				{
					std::cerr << "Error writing serialization of ciphertextVecx to encryptedlabels.txt" << std::endl;
					exit(1);
			}
		
			cout << "Y serialized to " + DATAFOLDER+"/encryptedlabels.txt\n";
			break;
		}
    case 't':
		{   
			char *args[] = {(char *) "python3", (char *)"./../train.py", (char*)"25", (char*) "0.001", (char *)argv[2], (char *) NULL};
			execvp(args[0], args);
			perror("exec error:");
			break;
		}
    default:
		{    
			printErrorMess();
			break;
		}
    }
	return 0;	
}


/*
 * string_split
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

vector<string> string_split(const string &s, char delim) {
    stringstream ss(s);
    string item;
    vector<string> tokens;
    while (getline(ss, item, delim)) {
        tokens.push_back(item);
    }
    return tokens;
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

std::vector<std::vector<double>> readFile(std::string filename)
{
	std::ifstream input (DATASETFOLDER + filename);

	std::vector<std::vector<double>> data;
	if(input.is_open())
	{
		std::string line;
		std::vector<std::string> line_v;
		while (getline (input,line) )
	 	{
			line_v = string_split(line, '\t');
			std::vector<double>  temp;
			unsigned size = static_cast<int>(line_v.size());
			for (unsigned i = 0; i < size; ++i) {
				temp.push_back(strtof((line_v[i]).c_str(),0));
			}
			data.push_back(temp);
		}
	}
	else std::cout << "Unable to open file" << '\n';
	input.close();

	return data;
}

/*
 * An helper function to explain the program usage
 * */
void printErrorMess()
{
        std::cout << "Program usage: ./encrypt <mode>" << std::endl;
		std::cout << "Modes:"<< std::endl;
        exit(1);
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

void accuracy_calc(const std::vector <int>&y, const std::vector< std::vector <double>>&y_hat){

        unsigned BATCH_SIZE = y.size();
        int counter=0, index=0;
        float max=0.;
	// std::vector<int> max_save;

	std::cout << "y hat has shape: (" << y_hat.size() << ", " << y_hat[0].size() << ")\n";

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
        };
        float count = counter;
	std::cout << "       Accuracy = " << (count/BATCH_SIZE)*100 << "% " << counter << " right answers" << "\n";
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

void packEncrypt(std::vector<std::vector<double>> data, CryptoContext<DCRTPoly> cc, KeyPair<DCRTPoly> keys)
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

	std::cout << "X_train packed, encrypted and serialized to " + DATAFOLDER+"/encryptedfeatures.txt\n";
	return;	
}

/*
 * SerializeCryptoContext 
 * args: 	cc - crypto contex
 * 			keys - encryption keys
 *
 * returns: void
 * 
 * description: Serializes the crypto context, private key and multkey.
 * 
 * */
void SerializeCryptoContext(CryptoContext<DCRTPoly> cc, KeyPair<DCRTPoly> keys)
{
	// Serialize cryptocontext
	if (!Serial::SerializeToFile(DATAFOLDER + "/cryptocontext.txt", cc, SerType::BINARY))
		{
		std::cerr << "Error writing serialization of the crypto context to "
				"cryptocontext.txt"
			<< std::endl;
		exit(1);
	}
	std::cout << "The cryptocontext has been serialized." << std::endl;

	// Serialize the private key
	if (!Serial::SerializeToFile(DATAFOLDER + "/key-private.txt", keys.secretKey, SerType::BINARY)) {
		std::cerr << "Error writing serialization of public key to key-public.txt" << std::endl;
		exit(1);
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
				exit(1);
		}
		std::cout << "The eval mult keys have been serialized." << std::endl;

		emkeyfile.close();
		}
		else {
			std::cerr << "Error serializing eval mult keys" << std::endl;
			exit(1);
		}
	return;
}

/*
 * CallFunc
 * args: 	cc - crypto contex
 * 			ciphertextVecx - cipher text vector
 *			W - weights vector
 *			NumWeightLines - Number of lines of the weight vector
 *			lowerbound - Lower bound of the function
 *			upperbound - Upper bound of the function
 *			degree - Polynomial degree
 *			act - Function to be executed
 *
 * returns: a vector with the encrypted ciphertexts 
 * 
 * description: Executes the EvalLinearWSum followed by one of the following functions: Chebyshev, Logistic, Mult and returns the result.
 * 
 * */
std::vector<ConstCiphertext<DCRTPoly>> CallFunc(CryptoContext<DCRTPoly> cc, std::vector<ConstCiphertext<DCRTPoly>> ciphertextVecx, std::vector<std::vector<double>> W, unsigned NumWeightLines, int lowerbound, int upperbound, int degree, string act) 
{
	std::vector<ConstCiphertext<DCRTPoly>> ciphertextVecy;
	int progress=NumWeightLines-1;
	// Performs the LinearWSum of ciphertextVecx with W
	for(unsigned i = 0; i < NumWeightLines; ++i){
		auto result = cc->EvalLinearWSum(ciphertextVecx, W[i]);
		if (act.compare("relu")==0) result = cc->EvalChebyshevFunction([](double x) -> double { return (x>0) ? x : 0; }, result, lowerbound, upperbound, degree);
		else if (act.compare("tanh")==0) result = cc->EvalChebyshevFunction([](double x) -> double { return tanh(x); }, result, lowerbound, upperbound, degree);
		else if (act.compare("sigmoid")==0) result = cc->EvalLogistic(result, lowerbound, upperbound, degree);
		else if (act.compare("linear")!=0) printErrorMess();
		
		ciphertextVecy.push_back(result);
		std::cout << "Calculated line i="<< i << "/" << progress << std::endl;
	}

	return ciphertextVecy;
}
