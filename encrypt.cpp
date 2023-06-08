/* This code reads the weights from the weights files and the encrypted data and does the calculations*/
#include "openfhe.h"

// header files needed for serialization
#include "ciphertext-ser.h"
#include "cryptocontext-ser.h"
#include "key/key-ser.h"
#include "scheme/ckksrns/ckksrns-ser.h"

#define PROFILE
using namespace lbcrypto;

#include <iostream>
#include <vector>
#include <math.h>
#include <fstream>
#include <sstream>
#include <string>
#include <random>
#include <algorithm>

using namespace std;

const std::string DATAFOLDER = "Data";
const std::string DATASETFOLDER = "../dataset";

vector<string> split(const string &s, char delim) {
    stringstream ss(s);
    string item;
    vector<string> tokens;
    while (getline(ss, item, delim)) {
        tokens.push_back(item);
    }
    return tokens;
}

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
        cout << "       Accuracy = " << (count/BATCH_SIZE)*100 << "% " << counter << " right answers" << std::endl;
}

std::vector<std::vector<double>> readFile (string filename){
	ifstream input (DATASETFOLDER + filename);

	std::vector<std::vector<double>> data;
	if(input.is_open())
	{
		string line;
		vector<string> line_v;
		while ( getline (input,line) )
		{
			line_v = split(line, '\t');
			vector<double>  temp;
			unsigned size = static_cast<int>(line_v.size());
			for (unsigned i = 0; i < size; ++i)
			{
				temp.push_back(strtof((line_v[i]).c_str(),0)); 
			}
			data.push_back(temp);
		}
	}
	else cout << "Unable to open file" << std::endl;
	input.close();

	return data;
}

std::vector<ConstCiphertext<DCRTPoly>> chebyFunc(CryptoContext<DCRTPoly> cc, std::vector<ConstCiphertext<DCRTPoly>> ciphertextVecx, std::vector<std::vector<double>> W, unsigned NumWeightLines, int lowerbound, int upperbound, int degree){
	std::vector<ConstCiphertext<DCRTPoly>> ciphertextVecy;

	int progress=NumWeightLines-1;
	// Performs the LinearWSum of ciphertextVecx with W
	for(unsigned i = 0; i < NumWeightLines; ++i){
		auto result = cc->EvalLinearWSum(ciphertextVecx, W[i]);
		auto approx = cc->EvalChebyshevFunction([](double x) -> double { return (x>0) ? x : 0; }, result, lowerbound, upperbound, degree);
		ciphertextVecy.push_back(approx);
		std::cout << "Calculated line i="<< i << "/" << progress << std::endl;
	}

	return ciphertextVecy;
}

int main(int argc, const char * argv[])
{
    
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

	// Serialize cryptocontext
	if (!Serial::SerializeToFile(DATAFOLDER + "/cryptocontext.txt", cc, SerType::BINARY)) {
	std::cerr << "Error writing serialization of the crypto context to "
					"cryptocontext.txt"
				<< std::endl;
		return 1;
	}
	std::cout << "The cryptocontext has been serialized." << std::endl;

	auto keys = cc->KeyGen();

	std::cout << "The key pair has been generated." << std::endl;

	// Serialize the private key
	if (!Serial::SerializeToFile(DATAFOLDER + "/key-private.txt", keys.secretKey, SerType::BINARY)) {
		std::cerr << "Error writing serialization of public key to key-public.txt" << std::endl;
		return 1;
	}
	std::cout << "The private key has been serialized." << std::endl;

	// Generate the relinearization key
    cc->EvalMultKeyGen(keys.secretKey);

	// Serialize the relinearization (evaluation) key for homomorphic
    // multiplication
    std::ofstream emkeyfile(DATAFOLDER + "/" + "key-eval-mult.txt", std::ios::out | std::ios::binary);
    if (emkeyfile.is_open()) {
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

	cout << "Loading data ..." << std::endl;

	// Reads the features and puts them in X_train
	vector<vector<double>> X_train=readFile("/test_features.txt");
	cout << "x_train loaded w/ shape: (" << X_train.size()<< ", " << X_train[0].size() << ")" << std::endl;;

    // Reads the weight file W1
	std::vector<std::vector<double>> W1=readFile("/w1.txt");
	cout << "W1 loaded w/ shape: (" << W1.size()<< ", " << W1[0].size() << ")" << std::endl;;

	// Reads the weight file W2
	std::vector<std::vector<double>> W2=readFile("/w2.txt");
	cout << "W2 loaded w/ shape: (" << W2.size()<< ", " << W2[0].size() << ")" << std::endl;;

	// Reads the weight file W3
	std::vector<std::vector<double>> W3=readFile("/w3.txt");
	cout << "W3 loaded w/ shape: (" << W3.size()<< ", " << W3[0].size() << ")" << std::endl;;

	unsigned size2 = static_cast<int>(X_train[0].size());
	
	cout << "Starting inference" << std::endl;
 	cout << size2 << std::endl;
	// Encrypts the transposed matrix (X_train) in lines to ciphertextVecx
	std::vector<ConstCiphertext<DCRTPoly>> ciphertextVecx;
	for (unsigned i = 0; i < X_train.size(); ++i)
	{
        Plaintext plaintext = cc->MakeCKKSPackedPlaintext(X_train[i]);
        ciphertextVecx.push_back(cc->Encrypt(keys.publicKey, plaintext));
		X_train[i].clear();
		X_train[i].shrink_to_fit();
	}
	cout << "X_train packed and Encrypted" << std::endl;
	X_train.clear();
	X_train.shrink_to_fit();

	if (!Serial::SerializeToFile(DATAFOLDER + "/" + "encryptedfeatures.txt", ciphertextVecx, SerType::BINARY)) {
        std::cerr << "Error writing serialization of ciphertextVecx to encryptedfeatures.txt" << std::endl;
        return 1;
    }
    std::cout << "ciphertextVecx has been serialized." << std::endl;
	ciphertextVecx.clear();
	ciphertextVecx.shrink_to_fit();

	// Clearing keys and contex
	cc->ClearEvalMultKeys();
	lbcrypto::CryptoContextFactory<lbcrypto::DCRTPoly>::ReleaseAllContexts();
	std::cout << "Context cleared!" << std::endl;

	// Reads encrypted data from file /Encrypteddata.txt
	std::vector<ConstCiphertext<DCRTPoly>> ciphertextVecy;

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

	// Perform first layer
	ciphertextVecy=chebyFunc(cc, ciphertextVecx, W1, 128, -2, 15, 5);
	ciphertextVecx = ciphertextVecy;
	ciphertextVecy.clear();
	ciphertextVecy.shrink_to_fit();
	ciphertextVecx.shrink_to_fit();
	cout << "First Layer Completed" << std::endl;	

	// Perform second layer
	ciphertextVecy=chebyFunc(cc, ciphertextVecx, W2, 64, -40, 36, 5);
	ciphertextVecx = ciphertextVecy;
	ciphertextVecy.clear();
	ciphertextVecy.shrink_to_fit();
	ciphertextVecx.shrink_to_fit();
	std::cout << "Second layer completed" << std::endl;

	// Perform Third layer
	for(unsigned i = 0; i < 10; ++i){
		auto result = cc->EvalLinearWSum(ciphertextVecx, W3[i]);
		ciphertextVecy.push_back(result);
		
		std::cout << "Calculated line i="<< i << "/9" << std::endl;
	}
	ciphertextVecx = ciphertextVecy;
	ciphertextVecy.clear();
	ciphertextVecy.shrink_to_fit();
	ciphertextVecx.shrink_to_fit();
	std::cout << "Third layer completed" << std::endl;

	// Deserialize secret key
	PrivateKey<DCRTPoly> sk;
    if (Serial::DeserializeFromFile(DATAFOLDER + "/key-private.txt", sk, SerType::BINARY) == false) {
        std::cerr << "Could not read secret key" << std::endl;
        return 1;
    }
    std::cout << "The secret key has been deserialized." << std::endl;

	// Decrypt results
	std::vector<std::vector<double>> classes;
	for(unsigned i = 0; i < 10; ++i){
		Plaintext resultDecrypted;
		cc->Decrypt(sk, ciphertextVecx[i], &resultDecrypted);
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
	ciphertextVecx.clear();
	ciphertextVecx.shrink_to_fit();
    
	// Check accuracy
	ifstream input_labels (DATASETFOLDER + "/test_labels.txt");
	vector<double> y_train;
    
	if (input_labels.is_open())
	{
		string line;
		vector<string> line_v;
		while (getline (input_labels,line) )
		{
				line_v = split(line, '\t');

			unsigned digit = strtof((line_v[0]).c_str(),0);
			y_train.push_back(digit);
		}
		cout << "y_train loaded w/ shape: " << y_train.size() << std::endl;
	}
	else cout << "Unable to open file" << std::endl;
    input_labels.close();
	accuracy_calc(y_train, classes, size2);
}
