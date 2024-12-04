# My Mini - Version Control System

## Assumptions:
My version control system ignores the files given below as they are part of my mini version control system, for eg it will not add files:  if(currFile == ".." || currFile == "." || currFile == ".mygit" || currFile == "add.h" || currFile == "mygit" || currFile == ".git" || currFile == ".vscode" || currFile == "main.cpp" || currFile == "README.md"|| currFile == "Makefile")

## Steps of execution:

1) g++ main.cpp -o mygit -lssl -lcrypto
./mygit init

2) 
echo -n "hello world" > test.txt
./mygit hash-object [-w] test.txt

3)
./mygit cat-file -p 4b6fcb2d521ef0fd442a5301e7932d16cc9f375a
./mygit cat-file -s 4b6fcb2d521ef0fd442a5301e7932d16cc9f375a

4)
./mygit write-tree

5)
./mygit ls-tree 073ad3f61890ba86e1c1c425ef5a4add5f2eadbe

6)
./mygit add .
./mygit add test.txt

7)
./mygit commit -m "Commit message"
./mygit commit

8)
./mygit log

9)
./mygit checkout 84264c74639a7b5539607f505335d28c699c942e
