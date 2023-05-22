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
    ifstream input (DATAFOLDER + "/train.txt");
    ofstream output1 (DATAFOLDER + "/Data.txt");
    ofstream output2 (DATAFOLDER + "/Label.txt");
    ofstream output3 (DATAFOLDER + "/DataRec.txt");

    // Initializing the vector of vectors
    vector<vector<double> > X_train;
    vector<vector<double> > y_train;
    vector<vector<double> > X_train_T;
    vector<vector<double> > y_train_T;

    float calc;
    
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

        std::vector<ConstCiphertext<DCRTPoly>> ciphertextVecx;
        for (unsigned i = 0; i < size2; ++i) {
            Plaintext plaintext = cc->MakeCKKSPackedPlaintext(X_train_T[i]);
            ciphertextVecx.push_back(cc->Encrypt(keys.publicKey, plaintext));
        }

        if (!Serial::SerializeToFile(DATAFOLDER + "/Encrypteddata.txt", ciphertextVecx, SerType::BINARY)) {
            std::cerr << "Error writing serialization of the encrypteddata to "
                     "Encrypteddata.txt"
            << std::endl;
            return 1;
        }
        std::cout << "The encrypteddata has been serialized." << std::endl;

        std::vector<ConstCiphertext<DCRTPoly>> ciphertextVecy;
        for (unsigned i = 0; i < size4; ++i) {
            Plaintext plaintext = cc->MakeCKKSPackedPlaintext(y_train_T[i]);
            ciphertextVecy.push_back(cc->Encrypt(keys.publicKey, plaintext));
        }

        if (!Serial::SerializeToFile(DATAFOLDER + "/Encryptedlabel.txt", ciphertextVecy, SerType::BINARY)) {
            std::cerr << "Error writing serialization of the encryptedlabel to "
                     "Encryptedlabel.txt"
            << std::endl;
            return 1;
        }
        std::cout << "The encryptedlabel has been serialized." << std::endl;

        cc->ClearEvalMultKeys();
        lbcrypto::CryptoContextFactory<lbcrypto::DCRTPoly>::ReleaseAllContexts();

        // Deserialize the crypto context
        CryptoContext<DCRTPoly> cc;
        if (!Serial::DeserializeFromFile(DATAFOLDER + "/cryptocontext.txt", cc, SerType::BINARY)) {
            std::cerr << "I cannot read serialization from " << "cryptocontext.txt" << std::endl;
            return 1;
        }
        std::cout << "The cryptocontext has been deserialized." << std::endl;

        PublicKey<DCRTPoly> pk;
        if (Serial::DeserializeFromFile(DATAFOLDER + "/key-public.txt", pk, SerType::BINARY) == false) {
            std::cerr << "Could not read public key" << std::endl;
            return 1;
        }
        std::cout << "The public key has been deserialized." << std::endl;

        PrivateKey<DCRTPoly> sk;
        if (Serial::DeserializeFromFile(DATAFOLDER + "/key-private.txt", sk, SerType::BINARY) == false) {
            std::cerr << "Could not read secret key" << std::endl;
            return 1;
        }
        std::cout << "The secret key has been deserialized." << std::endl;

        std::vector<ConstCiphertext<DCRTPoly>> ciphertextVecx_R;
        if (Serial::DeserializeFromFile(DATAFOLDER + "/Encrypteddata.txt", ciphertextVecx_R, SerType::BINARY) == false) {
            std::cerr << "Could not read the ciphertext" << std::endl;
            return 1;
        }
        std::cout << "The first ciphertext has been deserialized." << std::endl;

        for (unsigned i = 0; i < size2; ++i) {
            Plaintext result;
            cc->Decrypt(sk, ciphertextVecx_R[i], &result);
            output3 << result;
        }

    }
    else cout << "Unable to open file" << '\n';
}