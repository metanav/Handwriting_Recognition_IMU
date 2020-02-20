# Handwriting Recognition

## Required Hardware Component
1. Sparkfun Artemis ATP
2. SparkFun 9DoF IMU Breakout - ICM-20948 (Qwiic)
3. SparkFun Qwiic Cable - 500mm
4. Seeed Grove - Mech Keycap
5. Gameduino 3

## Required Softwares and Libraries
1. [Arduino IDE 1.8.12](https://www.arduino.cc/en/Main/Software)
2. [TensorFlow 2.1](https://www.tensorflow.org/install)
3. [SparkFun_ICM-20948_ArduinoLibrary](https://github.com/metanav/SparkFun_ICM-20948_ArduinoLibrary)
4. [Jupyter](https://jupyter.org/install)
5. [Gameduino 3 library](https://excamera.com/sphinx/gameduino2/code.html)

## Data Collection
Use [data_collection.ino](https://github.com/metanav/Handwriting_Recognition_IMU/tree/master/sketches/data_collection) in the sketches directory to collect data from the IMU sensor.

## Training
Use [train.ipynb](https://github.com/metanav/Handwriting_Recognition_IMU/blob/master/train.ipynb) to train the model.
You can use a GPU or CPU to train.

## Inferencing
Use [handwriting_recognizer.ino](https://github.com/metanav/Handwriting_Recognition_IMU/tree/master/sketches/handwriting_recognizer) to recognize handwritten numerals from the data coming from IMU sensor motion.




