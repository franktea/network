from os import walk
import os.path

def check_has_main(cpp):
    with open(cpp) as f:
        return 'int main(' in f.read()

def get_exe_name(cpp):
    return cpp.replace("./", "").replace(".cpp", "").replace("/", "-")        

def get_file_name(cpp):
    return cpp.replace("./", "")

for (path, dirs, files) in walk("./"):
    for f in files:
        if f.endswith(".cpp"):
            cpp_file = os.path.join(path, f)
            if check_has_main(cpp_file):
                #print(cpp_file)
                #print(get_exe_name(cpp_file))
                print("add_executable({} {})".format(get_exe_name(cpp_file), get_file_name(cpp_file)))
