# Private Neural Network Inference 

This is a project for the subject of Comunications Security. In this project we are supposed to train a neural network in the MNIST-dataset, and save the model. Then encrypt the test set and perform inference w/ the encrypted data, then decrypt the results and recover the resulting labels and compare with the actual labels.

## Build instructions

Prerequisites:
- cmake
- make
- g++
- openFHE library
- python3, w/ scikit-learn and numpy installed, to download the dataset



## Homomorphic encryption

Also outputs ExpectedResults file so that the correct operation can be checked

## Mnist-dataset 

You can download the mnist dataset using the script in dataset/download_mnist_dataset.py, more functionalities will later be added
