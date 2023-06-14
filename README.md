# Private Neural Network Inference 

This is a project for the subject of Comunications Security. In this project we are supposed to train a neural network in the MNIST-dataset, and save the model. Then encrypt the test set and perform inference w/ the encrypted data, then decrypt the results and recover the resulting labels and compare with the actual labels.

## Build instructions

Prerequisites:
- cmake
- make
- g++
- OpenFHE library
- python3, w/ scikit-learn and numpy installed, to download the dataset

To install the OpenFHE library refer to [OpenFHE installation guide](https://openfhe-development.readthedocs.io/en/latest/sphinx_rsts/intro/installation/installation.html). No addition libraries are needed

After installing all the prerequisites:
1. clone this repository
2. create a build directory where the binaries will be built. Example:
```
mkdir -p build/Data
cd build
cmake ..
```
3. compile the binary: ```make```


## Running the binary

Example usage:
```
# Download dataset
./main d

# pack and encrypt the dataset
./main p

# train the model
./main t <activation function>

# Supported activation functions include: "relu", "tanh", "sigmoid" and "linear"
# perform encrypted inference
./main i <activation_function>

# Decrypt the data and measure accuracy
./main e
```
We reccomend using the various modes/commands in the order presented

## Homomorphic encryption

Since our goal is to perform private inference, we have to be able to keep data data private, while being able to perform computations over it, we use FHE(Fully Homomorphic encryption). 

There are multiple libraries implementing FHE, to choose among them we had to inderstand what we needed from them.
- float support: CKKS scheme implementation
- Not so high-level that we don't have to understand how arrays/vectors are packed and don't have to set parameters related to noise flooding(this is a college assignment)

Only 2 were left and from those 2 OpenFHE had the best documentation, so here we are...

If you want an explanation on FHE and the CKKS scheme, just do like us and read this fantastic article: [CKKS explained](https://blog.openmined.org/ckks-explained-part-1-simple-encoding-and-decoding/)

## Mnist-dataset 

The Mnist Dataset is a dataset with 70000 hand-written digits saved in a 28x28 pixels image. Since the original dataset's website had an miscofiguration it is now unaccessible so we provide a script to download it
