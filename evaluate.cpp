//
//  nn.cpp
//  
//  To compile: g++ -o nn nn.cpp -std=c++11
//  To run: ./nn
//  Created by Sergei Bugrov on 4/20/18.
//  Copyright © 2017 Sergei Bugrov. All rights reserved.
//  Download dataset from: https://drive.google.com/file/d/1OdtwXHf_-2T0aS9HLBnxU3o-72mklCZY/view?usp=sharing

#include <iostream>
#include <vector>
#include <math.h>
#include <fstream>
#include <sstream>
#include <string>
#include <random>
#include <algorithm>

using namespace std;

void print ( const vector <float>& m, int n_rows, int n_columns ) {
    
    /*  "Couts" the input vector as n_rows x n_columns matrix.
     Inputs:
     m: vector, matrix of size n_rows x n_columns
     n_rows: int, number of rows in the left matrix m1
     n_columns: int, number of columns in the left matrix m1
     */
    
    for( int i = 0; i != n_rows; ++i ) {
        for( int j = 0; j != n_columns; ++j ) {
            cout << m[ i * n_columns + j ] << " ";
        }
        cout << '\n';
    }
    cout << endl;
}

int argmax ( const vector <float>& m ) {

    return distance(m.begin(), max_element(m.begin(), m.end()));
}

vector <float> relu(const vector <float>& z){
    int size = z.size();
    vector <float> output;
    for( int i = 0; i < size; ++i ) {
        if (z[i] < 0){
            output.push_back(0.0);
        }
        else output.push_back(z[i]);
    }
    return output;
}

vector <float> reluPrime (const vector <float>& z) {
    int size = z.size();
    vector <float> output;
    for( int i = 0; i < size; ++i ) {
        if (z[i] <= 0){
            output.push_back(0.0);
        }
        else output.push_back(1.0);
    }
    return output;
} 

static vector<float> random_vector(const int size)
{
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> distribution(0.0, 0.05);
    static default_random_engine generator;

    vector<float> data(size);
    generate(data.begin(), data.end(), [&]() { return distribution(generator); });
    return data;
}

vector <float> softmax (const vector <float>& z, const int dim) {
    
    const int zsize = static_cast<int>(z.size());
    vector <float> out;
    
    for (unsigned i = 0; i != zsize; i += dim) {
        vector <float> foo;
        for (unsigned j = 0; j != dim; ++j) {
            foo.push_back(z[i + j]);
        }
        
        float max_foo = *max_element(foo.begin(), foo.end());

        for (unsigned j = 0; j != dim; ++j) {
            foo[j] = exp(foo[j] - max_foo);
        }      

        float sum_of_elems = 0.0;
        for (unsigned j = 0; j != dim; ++j) {
            sum_of_elems = sum_of_elems + foo[j];
        }
        
        for (unsigned j = 0; j != dim; ++j) {
            out.push_back(foo[j]/sum_of_elems);
        }
    }
    return out;
}

vector <float> sigmoid_d (const vector <float>& m1) {
    
    /*  Returns the value of the sigmoid function derivative f'(x) = f(x)(1 - f(x)),
     where f(x) is sigmoid function.
     Input: m1, a vector.
     Output: x(1 - x) for every element of the input matrix m1.
     */
    
    const unsigned long VECTOR_SIZE = m1.size();
    vector <float> output (VECTOR_SIZE);
    
    
    for( unsigned i = 0; i != VECTOR_SIZE; ++i ) {
        output[ i ] = m1[ i ] * (1 - m1[ i ]);
    }
    
    return output;
}

vector <float> sigmoid (const vector <float>& m1) {
    
    /*  Returns the value of the sigmoid function f(x) = 1/(1 + e^-x).
     Input: m1, a vector.
     Output: 1/(1 + e^-x) for every element of the input matrix m1.
     */
    
    const unsigned long VECTOR_SIZE = m1.size();
    vector <float> output (VECTOR_SIZE);
    
    
    for( unsigned i = 0; i != VECTOR_SIZE; ++i ) {
        output[ i ] = 1 / (1 + exp(-m1[ i ]));
    }
    
    return output;
}

vector <float> operator+(const vector <float>& m1, const vector <float>& m2){
    
    /*  Returns the elementwise sum of two vectors.
     Inputs:
     m1: a vector
     m2: a vector
     Output: a vector, sum of the vectors m1 and m2.
     */
    
    const unsigned long VECTOR_SIZE = m1.size();
    vector <float> sum (VECTOR_SIZE);
    
    for (unsigned i = 0; i != VECTOR_SIZE; ++i){
        sum[i] = m1[i] + m2[i];
    };
    
    return sum;
}

vector <float> operator-(const vector <float>& m1, const vector <float>& m2){
    
    /*  Returns the difference between two vectors.
     Inputs:
     m1: vector
     m2: vector
     Output: vector, m1 - m2, difference between two vectors m1 and m2.
     */
    
    const unsigned long VECTOR_SIZE = m1.size();
    vector <float> difference (VECTOR_SIZE);
    
    for (unsigned i = 0; i != VECTOR_SIZE; ++i){
        difference[i] = m1[i] - m2[i];
    };
    
    return difference;
}

vector <float> operator*(const vector <float>& m1, const vector <float>& m2){
    
    /*  Returns the product of two vectors (elementwise multiplication).
     Inputs:
     m1: vector
     m2: vector
     Output: vector, m1 * m2, product of two vectors m1 and m2
     */
    
    const unsigned long VECTOR_SIZE = m1.size();
    vector <float> product (VECTOR_SIZE);
    
    for (unsigned i = 0; i != VECTOR_SIZE; ++i){
        product[i] = m1[i] * m2[i];
    };
    
    return product;
}

vector <float> operator*(const float m1, const vector <float>& m2){
    
    /*  Returns the product of a float and a vectors (elementwise multiplication).
     Inputs:
     m1: float
     m2: vector
     Output: vector, m1 * m2, product of two vectors m1 and m2
     */
    
    const unsigned long VECTOR_SIZE = m2.size();
    vector <float> product (VECTOR_SIZE);
    
    for (unsigned i = 0; i != VECTOR_SIZE; ++i){
        product[i] = m1 * m2[i];
    };
    
    return product;
}

vector <float> operator/(const vector <float>& m2, const float m1){
    
    /*  Returns the product of a float and a vectors (elementwise multiplication).
     Inputs:
     m1: float
     m2: vector
     Output: vector, m1 * m2, product of two vectors m1 and m2
     */
    
    const unsigned long VECTOR_SIZE = m2.size();
    vector <float> product (VECTOR_SIZE);
    
    for (unsigned i = 0; i != VECTOR_SIZE; ++i){
        product[i] = m2[i] / m1;
    };
    
    return product;
}

vector <float> transpose (float *m, const int C, const int R) {
    
    /*  Returns a transpose matrix of input matrix.
     Inputs:
     m: vector, input matrix
     C: int, number of columns in the input matrix
     R: int, number of rows in the input matrix
     Output: vector, transpose matrix mT of input matrix m
     */
    
    vector <float> mT (C*R);
    
    for(unsigned n = 0; n != C*R; n++) {
        unsigned i = n/C;
        unsigned j = n%C;
        mT[n] = m[R*j + i];
    }
    
    return mT;
}

vector <float> dot (const vector <float>& m1, const vector <float>& m2, const int m1_rows, const int m1_columns, const int m2_columns) {
    
    /*  Returns the product of two matrices: m1 x m2.
     Inputs:
     m1: vector, left matrix of size m1_rows x m1_columns
     m2: vector, right matrix of size m1_columns x m2_columns (the number of rows in the right matrix
     must be equal to the number of the columns in the left one)
     m1_rows: int, number of rows in the left matrix m1
     m1_columns: int, number of columns in the left matrix m1
     m2_columns: int, number of columns in the right matrix m2
     Output: vector, m1 * m2, product of two vectors m1 and m2, a matrix of size m1_rows x m2_columns
     */
    
    vector <float> output (m1_rows*m2_columns);
   
    for( int row = 0; row != m1_rows; ++row ) {
        // cout << "row=" << row << "\n";
	for( int col = 0; col != m2_columns; ++col ) {
            output[ row * m2_columns + col ] = 0.f;
	    for( int k = 0; k != m1_columns; ++k ) {
                output[ row * m2_columns + col ] += m1[ row * m1_columns + k ] * m2[ k * m2_columns + col ];
	    }
        }
    }
    
    return output;
}

vector<string> split(const string &s, char delim) {
    stringstream ss(s);
    string item;
    vector<string> tokens;
    while (getline(ss, item, delim)) {
        tokens.push_back(item);
    }
    return tokens;
}

void accuracy_calc(const vector <float>&y, const vector <float>&y_hat){

	int counter=0, digit, index=0;
	float max=0.;
	vector <float> max_save;
	//for (unsigned i=0; i<y_hat.size(); i++)
	for (unsigned i=0; i<y.size(); i++)
	{
		index=0;
		max=0.;
		for(unsigned j=0; j < 10; j++)
		{
			if(y_hat[(10*i)+j]>max)
			{
				max = y_hat[(10*i)+j];
				index=j;
			}
		}
		if(y[i]==index) counter++;
	}
	float count = counter;
	cout << "	Accuracy = " << (count/y.size())*100 << "% " << counter << " right answers" << "\n";
}

int main(int argc, const char * argv[]) {

    string line;
    vector<string> line_v;

    cout << "Loading data ...\n";
    vector<float> X_train;
    vector<float> y_train;
    vector<float> X_test;
    vector<float> y_test;
    vector<float> W1;
    vector<float> W2;
    vector<float> W3;
    
    ifstream myfile ("dataset/train.txt");
    if (myfile.is_open())
    {
        while ( getline (myfile,line) )
        {
            line_v = split(line, '\t');
            int digit = strtof((line_v[0]).c_str(),0);
            y_train.push_back(digit);
            int size = static_cast<int>(line_v.size());
            for (unsigned i = 1; i < size; ++i) {
                X_train.push_back(strtof((line_v[i]).c_str(),0));
            }
        }
        X_train = X_train/255.0;
        myfile.close();
    }
    
    else cout << "Unable to open file" << '\n';
	
    ifstream test_file ("dataset/test.txt");
    if (test_file.is_open())
    {
	while (getline(test_file, line))	
	{
            line_v = split(line, '\t');
            int digit = strtof((line_v[0]).c_str(),0);
            y_test.push_back(digit);
            int size = static_cast<int>(line_v.size());
            for (unsigned i = 1; i < size; ++i) {
                X_test.push_back(strtof((line_v[i]).c_str(),0));
            }
        }
        X_test = X_test/255.0;
        test_file.close();
    }
    
    else cout << "Unable to open file" << '\n';

    ifstream weights_file("trained_model.txt");		// todo:add ./dataset
    if (weights_file.is_open())
    {
	getline(weights_file, line);
        line_v = split(line, ',');
        int size = static_cast<int>(line_v.size());
        for (unsigned i = 0; i < size; ++i) {
        	W1.push_back(strtof((line_v[i]).c_str(),0));
        }
	getline(weights_file, line);
        line_v = split(line, ',');
        size = static_cast<int>(line_v.size());
        for (unsigned i = 0; i < size; ++i) {
        	W2.push_back(strtof((line_v[i]).c_str(),0));
        }
	getline(weights_file, line);
        line_v = split(line, ',');
        size = static_cast<int>(line_v.size());
        for (unsigned i = 0; i < size; ++i) {
        	W3.push_back(strtof((line_v[i]).c_str(),0));
        }
        weights_file.close();
    }
   	 
    else cout << "Unable to open file" << '\n';
    

    int train_size = static_cast<int>(X_train.size()/784);
    int test_size = static_cast<int>(X_test.size()/784);
  	
	 cout << "Testing the model\n";

    if(1)
    { 	
    	vector<float> a1 = relu(dot( X_train, W1, train_size, 784, 128 ));
	vector<float> a2 = relu(dot( a1, W2, train_size, 128, 64 ));
    	vector<float> yhat = softmax(dot( a2, W3, train_size, 64, 10 ), 10);
    	accuracy_calc(y_train, yhat);	
    }
    if(1)
    { 
    	vector<float> a1 = relu(dot( X_test, W1, test_size, 784, 128 ));
    	vector<float> a2 = relu(dot( a1, W2, test_size, 128, 64 ));
    	vector<float> yhat = softmax(dot( a2, W3, test_size, 64, 10 ), 10);
    	accuracy_calc(y_test, yhat);	
    }
    
    return 0;
}
