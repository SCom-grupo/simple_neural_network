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

vector<vector<double>> readFileNormalized (string filename);

int main(int argc, const char * argv[])
{

	 if (argc < 2) {
        std::cout << "No extra Command Line Argument passed" << std::endl;
        return 1;
    }

    switch (argv[1][0]) {
        case 'e':
            std::cout << "Function Encrypt." << std::endl;
            break;
        case 'd':
            std::cout << "Function Descrypt." << std::endl;
            break;
        case 'i':
            std::cout << "Function Inference." << std::endl;
            break;
         case 't':
            std::cout << "Function Training." << std::endl;
            break;
        default:
            std::cout << "Don't exist this Function." << std::endl;
            break;
    }

    for (int i = 0; i < argc; i++) {
            cout << "argv[" << i << "]: " << argv[i]
                 << '\n';
        }
	
	return 0;	

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

vector<vector<double>> readaFile(string filename){
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
