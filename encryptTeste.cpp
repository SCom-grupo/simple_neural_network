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
        cout << "       Accuracy = " << (count/BATCH_SIZE)*100 << "% " << counter << " right answers" << "\n";
}

vector<vector<double>> readFileNormalized (string filename){
	ifstream input (DATASETFOLDER + filename);
	float calc;

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
				calc=strtof((line_v[i]).c_str(),0);
				temp.push_back(calc/255.0);
			}
			data.push_back(temp);
		}
	}
	else cout << "Unable to open file" << '\n';
	input.close();

	return data;
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
	else cout << "Unable to open file" << '\n';
	input.close();

	return data;
}

int main(int argc, const char * argv[])
{
    
	// Create context and set its params
		
	uint32_t multDepth = 11;	// multiplication depth
	uint32_t scaleModSize = 50;	// scale module
    uint32_t batchSize = 1<<11;	// batch size or how many slots per pack

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

	std::cout << "CKKS scheme is using ring dimension " << cc->GetRingDimension() << std::endl << std::endl;

	std::cout << "\nThe cryptocontext has been generated." << std::endl;

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

	// Serialize the public key
	if (!Serial::SerializeToFile(DATAFOLDER + "/key-public.txt", keys.publicKey, SerType::BINARY)) {
		std::cerr << "Error writing serialization of public key to key-public.txt" << std::endl;
		return 1;
	}
	std::cout << "The public key has been serialized." << std::endl;

	cc->EvalMultKeyGen(keys.secretKey);

	cout << "Loading data ...\n";

	// Reads the features and puts them in X_train
	vector<vector<double>> X_train=readFileNormalized("/test_features.txt");
	cout << "x_train loaded w/ shape: (" << X_train.size()<< ", " << X_train[0].size() << ")\n";

    // Reads the weight file W1
	std::vector<std::vector<double>> W1=readFile("/w1t.txt");
	cout << "W1 loaded w/ shape: (" << W1.size()<< ", " << W1[0].size() << ")\n";

	// Reads the weight file W2
	std::vector<std::vector<double>> W2=readFile("/w2t.txt");
	cout << "W2 loaded w/ shape: (" << W2.size()<< ", " << W2[0].size() << ")\n";

	// Reads the weight file W3
	std::vector<std::vector<double>> W3=readFile("/w3t.txt");
	cout << "W3 loaded w/ shape: (" << W3.size()<< ", " << W3[0].size() << ")\n";

	unsigned size2 = static_cast<int>(X_train[0].size());
	
	cout << "Starting inference\n";
 	cout << size2 << "\n";
	// Encrypts the transposed matrix (X_train_T) in lines to ciphertextVecx
	std::vector<ConstCiphertext<DCRTPoly>> ciphertextVecx;
	for (unsigned i = 0; i < X_train.size(); ++i)
	{
        Plaintext plaintext = cc->MakeCKKSPackedPlaintext(X_train[i]);
        ciphertextVecx.push_back(cc->Encrypt(keys.publicKey, plaintext));
		X_train[i].clear();
		X_train[i].shrink_to_fit();
	}
	cout << "X_train packed and Encrypted\n";
	X_train.clear();
	X_train.shrink_to_fit();

	// Reads encrypted data from file /Encrypteddata.txt
	std::vector<ConstCiphertext<DCRTPoly>> ciphertextVecy;
	ofstream output3 (DATAFOLDER + "/ResultW1.txt");
	ofstream output4 (DATAFOLDER + "/ResultReluW1.txt");

	// Performs the LinearWSum of ciphertextVecx with W1 and outputs the results to output4
	for(unsigned i = 0; i < 128; ++i){
		auto result = cc->EvalLinearWSum(ciphertextVecx, W1[i]);
		auto approx = cc->EvalChebyshevFunction([](double x) -> double { return (x>0) ? x : 0; }, result, -2, 15, 5);
		ciphertextVecy.push_back(approx);
		std::cout << "Decrypted and calculated line i="<< i << std::endl;
		Plaintext resultDecrypted;
		Plaintext resultDecryptedRelu;
		cc->Decrypt(keys.secretKey, result, &resultDecrypted);
		output3 << resultDecrypted;
		cc->Decrypt(keys.secretKey, approx, &resultDecryptedRelu);
		output4 << resultDecryptedRelu;
	}
	ciphertextVecx = ciphertextVecy;
	ciphertextVecy.clear();
	ciphertextVecy.shrink_to_fit();
	ciphertextVecx.shrink_to_fit();
	cout << "First Layer Completed\n";	

	ofstream output5 (DATAFOLDER + "/ResultW2.txt");
	ofstream output6 (DATAFOLDER + "/ResultReluW2.txt");
	for(unsigned i = 0; i < 64; ++i){
		auto result = cc->EvalLinearWSum(ciphertextVecx, W2[i]);
		auto approx = cc->EvalChebyshevFunction([](double x) -> double { return (x>0) ? x : 0; }, result, -40, 36, 5);
		Plaintext resultDecrypted;
		cc->Decrypt(keys.secretKey, result, &resultDecrypted);
		output5 << resultDecrypted;
		ciphertextVecy.push_back(approx);
		std::cout << "Decrypted and calculated line i="<< i << std::endl;
		Plaintext resultDecryptedRelu;
		cc->Decrypt(keys.secretKey, approx, &resultDecryptedRelu);
		output6 << resultDecryptedRelu;
	}
	ciphertextVecx = ciphertextVecy;
	ciphertextVecy.clear();
	ciphertextVecy.shrink_to_fit();
	ciphertextVecx.shrink_to_fit();

	std::cout << "Second layer completed" << std::endl;

	std::vector<std::vector<double>> classes;
	ofstream output7 (DATAFOLDER + "/ResultW3.txt");
	for(unsigned i = 0; i < 10; ++i){
		auto result = cc->EvalLinearWSum(ciphertextVecx, W3[i]);
		Plaintext resultDecrypted;
		cc->Decrypt(keys.secretKey, result, &resultDecrypted);
		std::cout << "LinearWSum completed" << std::endl;
		std::vector<std::complex<double>> finalResult = resultDecrypted->GetCKKSPackedValue();
		std::vector<double> tempy;
		for(unsigned j = 0; j < finalResult.size(); j++)
		{
			tempy.push_back(finalResult[j].real());
			output7 << finalResult[j].real() << "\t";
		}
	output7 << "\n";
	classes.push_back(tempy);
		std::cout << "Decrypted and calculated line i="<< i << std::endl;
	}
	cout << "out has shape: (" << classes.size() << ", " << classes[0].size() << ")\n";
	ciphertextVecy.clear();
	ciphertextVecx.clear();
	ciphertextVecx.shrink_to_fit();

	std::cout << "Third layer completed" << std::endl;
    
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
		cout << "y_train loaded w/ shape: " << y_train.size() << "\n";
	}
	else cout << "Unable to open file" << '\n';
    input_labels.close();
	accuracy_calc(y_train, classes, size2);
}