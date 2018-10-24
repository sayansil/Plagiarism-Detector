# Plagiarism-Detector
Uses naive methods to detect observable plagiarism in text files. 


## Getting started
**1.**   Place all the reference text files in the **database** directory.

**2.**   Place all the text files required to be checked in the **target** directory.

**3.**   *(Optional)* Edit the `stopwords.txt` text file as per requirement, to add words which are to be ignored in the analysis.

**4.**   Compile `run.cpp` in c++17, with the command `g++ run.cpp -std=c++17 -lstdc++fs`.

**5.**   Run the generated executable with the command `./a.out` *(in Linux environment).*
