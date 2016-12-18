import subprocess, os

#check if given test is in list
#it is in list if one of the strings in 'list' is at the beginning of 'test'
def test_in_list(test, list):
    for check in list:
        check=check.replace('+','')
        if (test.startswith(check)):
            return True
    return False

#run a local command in a shell
def quickshell(command):
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT)
    (retout, reterr) = process.communicate()
    if (process.returncode != 0):
        print "ERROR: shell command failed: " + command
        if len(retout):
            print "ERROR: command says: "
            for l in retout.splitlines():
                print "ERROR:     " + l
        os._exit(1)
    return retout

RED="\x1b[31m"
GREEN="\x1b[32m"
YELLOW="\x1b[33m"
RESET="\x1b[m"
