#check if given test is in list
#it is in list if one of the strings in 'list' is at the beginning of 'test'
def test_in_list(test, list):
    for check in list:
        check=check.replace('+','')
        if (test.startswith(check)):
            return True
    return False
