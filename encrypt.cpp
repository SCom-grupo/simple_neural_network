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

vector<string> split(const string &s, char delim) {
    stringstream ss(s);
    string item;
    vector<string> tokens;
    while (getline(ss, item, delim)) {
        tokens.push_back(item);
    }
    return tokens;
}

int main(int argc, const char * argv[]) {
    uint32_t multDepth = 1;
    uint32_t scaleModSize = 50;
    uint32_t batchSize = 1<<13;

    CCParams<CryptoContextCKKSRNS> parameters;
    parameters.SetMultiplicativeDepth(multDepth);
    parameters.SetScalingModSize(scaleModSize);
    parameters.SetBatchSize(batchSize);

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

    // Serialize the secret key
    if (!Serial::SerializeToFile(DATAFOLDER + "/key-private.txt", keys.secretKey, SerType::BINARY)) {
        std::cerr << "Error writing serialization of private key to key-private.txt" << std::endl;
        return 1;
    }
    std::cout << "The secret key has been serialized." << std::endl;

    cc->EvalMultKeyGen(keys.secretKey);

    string line;
    vector<string> line_v;

    cout << "Loading data ...\n";
    ifstream input (DATAFOLDER + "/test.txt");
    ifstream inputW1 (DATAFOLDER + "/w1.txt");
    ifstream inputW2 (DATAFOLDER + "/w2.txt");
    ifstream inputW3 (DATAFOLDER + "/w3.txt");
    ofstream output1 (DATAFOLDER + "/Data.txt");
    ofstream output2 (DATAFOLDER + "/Label.txt");
    ofstream output3 (DATAFOLDER + "/DataRec.txt");
    ofstream output4 (DATAFOLDER + "/Result.txt");

    // Initializing the vector of vectors
    vector<vector<double>> X_train;
    vector<vector<double>> y_train;
    vector<vector<double>> X_train_T;
    vector<vector<double>> y_train_T;

    std::vector<std::vector<double>> W1;
    std::vector<std::vector<double>> W2;
    std::vector<std::vector<double>> W3;

    float calc;
    
    // Reads the data and separates in labels (y_train) and bits (X_train)
    if (input.is_open())
    {
        while ( getline (input,line) )
        {
            line_v = split(line, '\t');

            vector<double>  tempy;
            unsigned digit = strtof((line_v[0]).c_str(),0);
            for (unsigned i = 0; i < 10; ++i) {
                if (i == digit)
                {
                    tempy.push_back(1.);
                }
                else tempy.push_back(0.);
            }
            
            vector<double>  tempx;
            unsigned size = static_cast<int>(line_v.size());
            for (unsigned i = 1; i < size; ++i) {
                calc=strtof((line_v[i]).c_str(),0);
                tempx.push_back(calc/255.0);
            }

            y_train.push_back(tempy);
            X_train.push_back(tempx);
        }
        input.close();

        // Reads the weight file W1
        while ( getline (inputW1,line) )
        {
            line_v = split(line, '\t');
            
            vector<double>  tempw;
            unsigned size = static_cast<int>(line_v.size());
            for (unsigned i = 0; i < size; ++i) {
                tempw.push_back(strtof((line_v[i]).c_str(),0)); 
            }
            W1.push_back(tempw);
        }
        inputW1.close();

        // Reads the weight file W2
        while ( getline (inputW2,line) )
        {
            line_v = split(line, '\t');
            
            vector<double>  tempw;
            unsigned size = static_cast<int>(line_v.size());
            for (unsigned i = 0; i < size; ++i) {
                tempw.push_back(strtof((line_v[i]).c_str(),0));
            }
            W2.push_back(tempw);
        }
        inputW2.close();

        // Reads the weight file W3
        while ( getline (inputW3,line) )
        {
            line_v = split(line, '\t');
            
            vector<double>  tempw;
            unsigned size = static_cast<int>(line_v.size());
            for (unsigned i = 0; i < size; ++i) {
                tempw.push_back(strtof((line_v[i]).c_str(),0));
            }
            W3.push_back(tempw);
        }
        inputW3.close();
        
        // Transposes the bits matrix (X_train) so that in can be encrypted in columns
        // Also outputs the transposed matrix to output1 for checking the correct operation
        unsigned size1 = static_cast<int>(X_train.size());
        unsigned size2 = static_cast<int>(X_train[0].size());
        for (unsigned i2 = 0; i2 < size2; ++i2) {
            vector<double>  tempx_T;
            for (unsigned i1 = 0; i1 < size1; ++i1) {
                output1 << X_train[i1][i2] << "\t";
                tempx_T.push_back(X_train[i1][i2]);
            }
            output1.seekp(-1 ,ios::cur);
            output1 << "\n";
            X_train_T.push_back(tempx_T);
        }

        // Transposes the bits matrix (y_train) so that in can be encrypted in columns
        // Also outputs the transposed matrix to output2 for checking the correct operation
        unsigned size3 = static_cast<int>(y_train.size());
        unsigned size4 = static_cast<int>(y_train[0].size());
        for (unsigned i2 = 0; i2 < size4; ++i2) {
            vector<double>  tempy_T;
            for (unsigned i1 = 0; i1 < size3; ++i1) {
                output2 << y_train[i1][i2] << "\t";
                tempy_T.push_back(y_train[i1][i2]);
            }
            output2.seekp(-1 ,ios::cur);
            output2 << "\n";
            y_train_T.push_back(tempy_T);
        }

        // Encrypts the transposed matrix (X_train_T) in lines to ciphertextVecx
        std::vector<ConstCiphertext<DCRTPoly>> ciphertextVecx;
        for (unsigned i = 0; i < size2; ++i) {
            Plaintext plaintext = cc->MakeCKKSPackedPlaintext(X_train_T[i]);
            ciphertextVecx.push_back(cc->Encrypt(keys.publicKey, plaintext));
        }

        // Outputs the encrypted data to file /Encrypteddata.txt
        if (!Serial::SerializeToFile(DATAFOLDER + "/Encrypteddata.txt", ciphertextVecx, SerType::BINARY)) {
            std::cerr << "Error writing serialization of the encrypteddata to "
                     "Encrypteddata.txt"
            << std::endl;
            return 1;
        }
        std::cout << "The encrypteddata has been serialized." << std::endl;

        // Encrypts the transposed matrix (y_train_T) in lines to ciphertextVecy
        std::vector<ConstCiphertext<DCRTPoly>> ciphertextVecy;
        for (unsigned i = 0; i < size4; ++i) {
            Plaintext plaintext = cc->MakeCKKSPackedPlaintext(y_train_T[i]);
            ciphertextVecy.push_back(cc->Encrypt(keys.publicKey, plaintext));
        }

        // Outputs the encrypted labels to file /Encryptedlabel.txt
        if (!Serial::SerializeToFile(DATAFOLDER + "/Encryptedlabel.txt", ciphertextVecy, SerType::BINARY)) {
            std::cerr << "Error writing serialization of the encryptedlabel to "
                     "Encryptedlabel.txt"
            << std::endl;
            return 1;
        }
        std::cout << "The encryptedlabel has been serialized." << std::endl;

        // Clears cryptocontext to check correct functioning
        cc->ClearEvalMultKeys();
        lbcrypto::CryptoContextFactory<lbcrypto::DCRTPoly>::ReleaseAllContexts();

        // Read the crypto context from file /cryptocontext.txt
        CryptoContext<DCRTPoly> cc;
        if (!Serial::DeserializeFromFile(DATAFOLDER + "/cryptocontext.txt", cc, SerType::BINARY)) {
            std::cerr << "I cannot read serialization from " << "cryptocontext.txt" << std::endl;
            return 1;
        }
        std::cout << "The cryptocontext has been deserialized." << std::endl;

        // Reads public key from file /key-public.txt
        PublicKey<DCRTPoly> pk;
        if (Serial::DeserializeFromFile(DATAFOLDER + "/key-public.txt", pk, SerType::BINARY) == false) {
            std::cerr << "Could not read public key" << std::endl;
            return 1;
        }
        std::cout << "The public key has been deserialized." << std::endl;

        // Reads private key from file /key-private.txt
        PrivateKey<DCRTPoly> sk;
        if (Serial::DeserializeFromFile(DATAFOLDER + "/key-private.txt", sk, SerType::BINARY) == false) {
            std::cerr << "Could not read secret key" << std::endl;
            return 1;
        }
        std::cout << "The secret key has been deserialized." << std::endl;

        // Reads encrypted data from file /Encrypteddata.txt
        std::vector<ConstCiphertext<DCRTPoly>> ciphertextVecx_R;
        if (Serial::DeserializeFromFile(DATAFOLDER + "/Encrypteddata.txt", ciphertextVecx_R, SerType::BINARY) == false) {
            std::cerr << "Could not read the ciphertext" << std::endl;
            return 1;
        }
        std::cout << "The first ciphertext has been deserialized." << std::endl;

        // Performs the LinearWSum of ciphertextVecx_R with W1 and outputs the results to output4
        for(unsigned i = 0; i < 128; ++i){
            auto result = cc->EvalLinearWSum(ciphertextVecx_R, W1[i]);
            Plaintext resultDecrypted;
            cc->Decrypt(sk, result, &resultDecrypted);
            output4 << resultDecrypted;
        }

    }
    else cout << "Unable to open file" << '\n';
}