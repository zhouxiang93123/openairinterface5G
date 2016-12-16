import subprocess

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
                               stderr=None)
    (retout, reterr) = process.communicate()
    if (process.returncode != 0):
        print "Error: shell command failed: " + command
        exit(1)
    return retout
