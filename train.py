#!/usr/bin/python3

import numpy as np
from dataset.download_mnist import fetch_classification_data
from sys import argv

def configure_seed(seed):
    np.random.seed(seed)

# sigmoid activation function
def sigmoid(x):
  return 1/(1 + np.exp(-x))

# Derivative of logistc activation function
def sigmoid_derivative(x):
  sig = sigmoid(x)
  return sig*(1-sig)

# Derivative of tanh activation function
def tanh_derivative(x):
  return 1 - np.tanh(x)**2

# ReLU activation function
def relu(x):
  return np.maximum(0, x)

# Derivative of activation function
def relu_derivative(x):
  return x > 0

# Multinomial logistic loss function
def cross_entropy(y, y_pred):
  #y_pred -= np.max(y_pred)
  epsilon = 1e-5
  return -np.sum(y * np.log(y_pred+epsilon))

# Derivative of loss function
def cross_entropy_derivative(y, y_pred):
  y = one_hot_encode(10, y)
  return y_pred - y

def one_hot_encode(n_classes, y):
  encode = np.zeros((1, n_classes))
  encode[0][y] = 1
  return encode

class Neural_Net(object):
    def __init__(self, n_classes, n_features, hidden_size, hidden2, act, derivative):
        self.n_classes = n_classes
        self.hidden_size = hidden_size
        self.hidden2 = hidden2
        self.n_features = n_features
        # if act == relu:
        #     self.weights_1 = np.random.uniform(0., 0.05, size=(n_features, hidden_size)) # 0.1 0.1
        #     self.weights_2 = np.random.uniform(0., 0.05, size=(hidden_size, hidden2))
        #     self.weights_3 = np.random.uniform(0., 0.05, size=(hidden2, n_classes))
        self.weights_1 = np.random.normal(loc= 0, scale=np.sqrt(3.6/n_features), size=(n_features, hidden_size)) # 0.1 0.1
        self.weights_2 = np.random.normal(loc= 0, scale=np.sqrt(3.6/hidden_size), size=(hidden_size, hidden2))
        self.weights_3 = np.random.normal(loc= 0, scale=np.sqrt(3.6/hidden2), size=(hidden2, n_classes))
        self.act = act
        self.derivative = derivative

    def train_epoch(self, X, y, **kwargs):
        for x_i, y_i in zip(X, y):
            self.update_weight(x_i, y_i, **kwargs)

    def predict(self, X):
        """X (n_examples x n_features)"""
        # Compute the forward pass of the network. At prediction time, there is
        # no need to save the values of hidden nodes, whereas this is required
        # at training time.
        # Forward pass
        z1 = np.dot(X, self.weights_1)
        a1 = self.act(z1)
        z2 = np.dot(a1, self.weights_2)
        a2 = self.act(z2)
        y_pred = np.dot(a2, self.weights_3)
        # z3 -= np.max(z3)
        # y_pred = np.exp(z3) / np.sum(np.exp(z3)) # softmax normalization
        return np.argmax(y_pred, axis=1)

    def evaluate(self, X, y):
        """
        X (n_examples x n_features):
        y (n_examples): gold labels
        """
        y_hat = self.predict(X)
        n_correct = (y == y_hat).sum()
        n_possible = y.shape[0]
        return n_correct / n_possible

    def train_epoch(self, X, y, learning_rate=0.001):
        '''
        Performs forward and backward pass
        ''' 
        for x,y in zip(X,y):
          x = np.expand_dims(x, 1)
          # forward pass
          z1 = np.dot(x.T, self.weights_1)
          a1 = self.act(z1)
          z2 = np.dot(a1, self.weights_2) 
          a2 = self.act(z2)
          z3 = np.dot(a2, self.weights_3)
          z3 -= np.max(z3)
          y_pred = np.exp(z3) / np.sum(np.exp(z3))  # softmax_normalization 

          # Backward pass
          dz3 = cross_entropy_derivative(y, y_pred) 
          da2 = np.dot(dz3, self.weights_3.T)
          dz2 = da2 * self.derivative(z2)
          dw3 = np.dot(a2.T, dz3)
          da1 = np.dot(dz2, self.weights_2.T)
          dz1 = da1 * self.derivative(z1)
          dw2 = np.dot(a1.T, dz2)
          dw1 = np.dot(x, dz1)

          # Update weights and biases
          self.weights_3 -= learning_rate * dw3
          self.weights_2 -= learning_rate * dw2
          self.weights_1 -= learning_rate * dw1

def main(epochs=25, learning_rate=0.001, act='relu'):
    hidden_size = 128 # 200
    hidden2 = 64
    layers = 2

    configure_seed(seed=42)
    train_X, ev_X, train_y, ev_y = fetch_classification_data()
    epochs = np.arange(1, epochs + 1)

    if act == 'relu':
        func = relu
        derivative = relu_derivative
    elif act == 'sigmoid':
        func = sigmoid
        derivative = sigmoid_derivative
    elif act == 'tanh':
        func = np.tanh
        derivative = tanh_derivative

    n_classes = np.unique(train_y).size  # 10
    n_feats = train_X.shape[1]
    print(n_classes, "classes")
    print(n_feats, "features")
    print(train_X.shape[0], "Training Samples;", ev_X.shape[0], "Training Samples;")


    # initialize the model
    model = Neural_Net(n_classes, n_feats, hidden_size, hidden2, func, derivative)

    valid_accs = []
    test_accs = []
    for i in epochs:
        print('Training epoch {}'.format(i))
        train_order = np.random.permutation(train_X.shape[0])
        train_X = train_X[train_order]
        train_y = train_y[train_order]
        model.train_epoch(
            train_X,
            train_y,
            learning_rate=learning_rate
        )
        # Commented for the time being, might be used later if necessaty for accuracy optimization
        print("test accuracy:",model.evaluate(ev_X, ev_y))

    np.savetxt('../dataset/w1.txt', np.transpose(model.weights_1), fmt='%f', delimiter='\t')
    np.savetxt('../dataset/w2.txt', np.transpose(model.weights_2), fmt='%f', delimiter='\t')
    np.savetxt('../dataset/w3.txt', np.transpose(model.weights_3), fmt='%f', delimiter='\t')

if __name__=='__main__':
    if len(argv)!=4:
        print("Script usage: ./train.py <epochs> <learning rate> <activation function>")
        print("Possible activation functions: 'relu' 'sigmoid' 'tanh'")
        print("Using default Values: 25 epochs, lr = 0.001, 'relu'")
        main()
    else:  
        main(int(argv[1]), float(argv[2]), act=argv[3])
