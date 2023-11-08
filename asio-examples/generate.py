from os import walk
import os.path

def check_has_main(cpp):
    with open(cpp) as f:
        return 'int main(' in f.read()
        

for (path, dirs, files) in walk("./"):
    for f in files:
        if f.endswith(".cpp"):
            cpp_file = os.path.join(path, f)
            if check_has_main(cpp_file):
                print(cpp_file)

