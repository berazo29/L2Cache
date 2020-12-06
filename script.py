# Scrip to copy .c .h Makefile in a folder name pa5/first or pa5/second
# This is for python3
import os
import shutil
import re
import subprocess


def main():

    autograter = 'pa5_autograter'
    projectFolder = 'pa5'
    path = os.getcwd()
    print("The current working directory is %s", path)


    action = input("Enter 1 for Pa5/first or 2 Pa5/second: ")

    print(action)
    if int(action) is 1:
        new_dir = "/pa5/first"
    elif int(action) is 2:
        new_dir = "/pa5/second"
    else:
        return

    # update the path
    new_path = path+new_dir
    print("new path = %s", new_path)


    try:
        os.makedirs(new_path)
    except OSError:
        if os.scandir(new_path):
            print("Directory path already exits", new_path)
        else:
            print ("Creation of the directory %s failed" % path)
    else:
        print ("Successfully created the directory %s" % path)

    with os.scandir('./') as entries:
        for entry in entries:
            src = os.path.basename(entry.name)
            if entry.name.endswith('.c') or entry.name.endswith('.h'):
                try:
                    shutil.copy(src, new_path)
                except OSError:
                    print(entry.name,"file copied failed")
                else:
                    print(entry.name,"file copied successfully")

            elif bool(re.match("makefile",entry.name.lower())):
                try:
                    shutil.copy(src, new_path)
                except OSError:
                    print(entry.name,"file copied failed")
                else:
                    print(entry.name,"file copied successfully")
            else:
                pass

    # Run the makefile to compile the c program
    cmd = "cd {0}".format(path)
    p = subprocess.Popen(['make'], cwd=new_path)
    p.wait()

    try:
        os.scandir(autograter)
        shutil.move(projectFolder, autograter)
    except OSError:
        print("Move manually the {}".format(projectFolder))
    else:
        print("Folder {} moved ".format(projectFolder))


main()
print("Done!")
