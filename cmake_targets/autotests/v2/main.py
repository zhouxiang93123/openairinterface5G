#!/usr/bin/python

import os, re
import xml.etree.ElementTree as ET
from ssh import connection
from utils import test_in_list

#p = connection('EPC ALU HSS', 'mozart', 'roux', 'linuxA')

#try:
#    z = os.read(p.fd, 1024)
#except Exception, e:
#    print "nothing returned"
#    z = ""

oai_user         = os.environ.get('OAI_USER')
oai_password     = os.environ.get('OAI_PASS')
requested_tests  = os.environ.get('OAI_TEST_CASE_GROUP')
machines         = os.environ.get('MACHINELIST')
generic_machines = os.environ.get('MACHINELISTGENERIC')
result_dir       = os.environ.get('RESULT_DIR')
nruns_softmodem  = os.environ.get('NRUNS_LTE_SOFTMODEM')
openair_dir      = os.environ.get('OPENAIR_DIR')

some_undef = False
if (oai_user         == None) :
        print "variable OAI_USER is not defined"
        some_undef = True
if (oai_password     == None) :
        print "variable OAI_PASS is not defined"
        some_undef = True
if (requested_tests  == None) :
        print "variable OAI_TEST_CASE_GROUP is not defined"
        some_undef = True
if (machines         == None) :
        print "variable MACHINELIST is not defined"
        some_undef = True
if (generic_machines == None) :
        print "variable MACHINELISTGENERIC is not defined"
        some_undef = True
if (result_dir       == None) :
        print "variable RESULT_DIR is not defined"
        some_undef = True
if (nruns_softmodem  == None) :
        print "variable NRUNS_LTE_SOFTMODEM is not defined"
        some_undef = True
if (openair_dir      == None) :
        print "variable OPENAIR_DIR is not defined"
        some_undef = True
if (some_undef == True):
    exit(1)

xml_test_file = os.environ.get('OPENAIR_DIR') + \
                "/cmake_targets/autotests/test_case_list.xml"

xmlTree = ET.parse(xml_test_file)
xmlRoot = xmlTree.getroot()

exclusion_tests=xmlRoot.findtext('TestCaseExclusionList',default='')
all_tests=xmlRoot.findall('testCase')

exclusion_tests=exclusion_tests.split()
requested_tests=requested_tests.split()

#check that exclusion tests are well formatted
#(6 digits or less than 6 digits followed by +)
for test in exclusion_tests:
    if     (not re.match('^[0-9]{6}$', test) and
            not re.match('^[0-9]{1,5}\+$', test)):
        print "ERROR: exclusion test is invalidly formatted: " + test
        exit(1)

#check that requested tests are well formatted
#(6 digits or less than 6 digits followed by +)
#be verbose
for test in requested_tests:
    if     (re.match('^[0-9]{6}$', test) or
            re.match('^[0-9]{1,5}\+$', test)):
        print "INFO: test group/case requested: " + test
    else:
        print "ERROR: requested test is invalidly formatted: " + test
        exit(1)

#get the list of tests to be done
todo_tests=[]
for test in all_tests:
    if     (test_in_list(test.get('id'), requested_tests) and
            not test_in_list(test.get('id'), exclusion_tests)):
        print "INFO: test will be run: " + test.get('id')
        todo_tests.append(test)
    else:
        print "INFO: test will be skipped: " + test.get('id')

#def ssh_connection(host, user, password):
#    process = subprocess.Popen(['sshpass', '-p', password,
#        'ssh', user + '@' + host],
#        stdin=subprocess.PIPE,
#        stdout=subprocess.PIPE,
#        stderr=subprocess.STDOUT)
#    fcntl.fcntl(process.stdout.fileno(), fcntl.F_SETFL,
#        fcntl.fcntl(process.stdout.fileno(), fcntl.F_GETFL) | os.O_NONBLOCK)
#    process.stdin.write('export PS1=OPENAIR_PS1\n')
#    return process
#
#def main():
#    oai_user     = os.environ.get('OAI_USER')
#    oai_password = os.environ.get('OAI_PASS')
#    oai_user = 'roux'
#    oai_password = 'linuxA'
#    machine1 = ssh_connection('mozart', oai_user, oai_password)
#    time.sleep(2)
#    machine1.terminate()
#    machine1.kill()
#    machine1.wait()
#
#main()
